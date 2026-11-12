#include "MainWindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDir>
#include <QHBoxLayout>
#include <QImage>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QSettings>
#include <QShortcut>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <random>

#include "CalculatorTab.h"
#include "GameTab.h"
#include "Theme.h"
#include "screens/HelpScreen.h"
#include "screens/HomeScreen.h"
#include "widgets/PixelDialog.h"

// ---------------------------------------------------------------- PixelWipe
// The old screen (a grabbed pixmap) disappears in 8 x 8 blocks over 6 steps, 180 ms (plan 5.8).
class PixelWipe : public QWidget
{
public:
    explicit PixelWipe(QWidget* parent) : QWidget(parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        hide();
        timer.setInterval(30);
        connect(&timer, &QTimer::timeout, this, [this] { step(); });
    }

    void play(const QPixmap& old)
    {
        image = old.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
        const int cols = (width() + 7) / 8, rows = (height() + 7) / 8;
        order.resize(cols * rows);
        for (int i = 0; i < (int)order.size(); i++) order[i] = i;
        std::shuffle(order.begin(), order.end(), std::mt19937(8));
        done = 0;
        steps = 0;
        setGeometry(parentWidget()->rect());
        raise();
        show();
        timer.start();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.drawImage(0, 0, image);
    }

private:
    void step()
    {
        const int cols = (width() + 7) / 8;
        const int target = (int)order.size() * ++steps / 6;
        QPainter p(&image);
        p.setCompositionMode(QPainter::CompositionMode_Clear);
        for (; done < target; done++) p.fillRect((order[done] % cols) * 8, (order[done] / cols) * 8, 8, 8, Qt::transparent);
        p.end();
        update();
        if (steps >= 6) { timer.stop(); hide(); image = QImage(); }
    }

    QImage image;
    std::vector<int> order;
    int done = 0, steps = 0;
    QTimer timer;
};

// ---------------------------------------------------------------- TabScreen
// shortcut: wraps Ashmit's two tabs unchanged until phases 4 and 5 replace them with real screens.
class TabScreen : public Screen
{
public:
    TabScreen(const QString& title, QWidget* tab, QWidget* parent) : Screen(parent), tab(tab)
    {
        QVBoxLayout* v = new QVBoxLayout(this);
        v->setContentsMargins(0, 0, 0, 0);
        ScreenHeader* header = new ScreenHeader(title, this);
        connect(header, &ScreenHeader::back, this, &Screen::back);
        v->addWidget(header);
        v->addWidget(tab, 1);
        tab->setParent(this);
    }
    void enter() override { tab->setFocus(); }

protected:
    void keyPressEvent(QKeyEvent* e) override
    {
        if (e->key() == Qt::Key_Escape) emit back(); else Screen::keyPressEvent(e);
    }

private:
    QWidget* tab;
};

// ---------------------------------------------------------------- MainWindow

MainWindow::MainWindow()
{
    setWindowTitle("PIXEL-8");
    setMinimumSize(1024, 680);

    stack = new QStackedWidget(this);
    setCentralWidget(stack);

    HomeScreen* home = new HomeScreen(stack);
    add(ScreenId::Home, home);
    add(ScreenId::Calculator, new TabScreen("CALCULATOR", new CalculatorTab(), stack));
    GameTab* games = new GameTab();
    TabScreen* arcade = new TabScreen("ARCADE", games, stack);
    add(ScreenId::Snake, arcade);
    add(ScreenId::Pong, arcade);
    help = new HelpScreen(stack);
    add(ScreenId::Help, help);

    connect(home, &HomeScreen::helpRequested, this, &MainWindow::showHelp);
    connect(home, &HomeScreen::aboutRequested, this, &MainWindow::showAbout);
    connect(home, &HomeScreen::settingsRequested, this, &MainWindow::showSettings);
    connect(home, &HomeScreen::exitRequested, this, &QWidget::close);

    wipe = new PixelWipe(stack);

    QShortcut* f1 = new QShortcut(QKeySequence(Qt::Key_F1), this);
    f1->setContext(Qt::ApplicationShortcut);
    connect(f1, &QShortcut::activated, this, &MainWindow::showHelp);
    QShortcut* f11 = new QShortcut(QKeySequence(Qt::Key_F11), this);
    f11->setContext(Qt::ApplicationShortcut);
    connect(f11, &QShortcut::activated, this, &MainWindow::toggleFullScreen);

    resize(1280, 800);
    restoreGeometry(QSettings().value("geometry").toByteArray());

    stack->setCurrentWidget(home);
    currentId = ScreenId::Home;
    QTimer::singleShot(0, home, &Screen::enter);
}

void MainWindow::add(ScreenId id, Screen* s)
{
    screens.insert((int)id, s);
    if (stack->indexOf(s) >= 0) return;   // Snake and Pong share one screen
    stack->addWidget(s);
    connect(s, &Screen::back, this, [this, id] { go(id == ScreenId::Help ? beforeHelp : ScreenId::Home); });
    connect(s, &Screen::navigate, this, &MainWindow::go);
}

Screen* MainWindow::current() const
{
    return qobject_cast<Screen*>(stack->currentWidget());
}

void MainWindow::go(ScreenId id)
{
    if (PixelDialog::isOpen()) return;
    Screen* next = screens.value((int)id);
    if (!next) return;
    if (next == current() && id == currentId) { next->enter(); return; }

    if (id == ScreenId::Help) { beforeHelp = currentId; help->showFor(currentId); }
    if ((id == ScreenId::Snake || id == ScreenId::Pong) && currentId != ScreenId::Help)   // start the chosen game
        QMetaObject::invokeMethod(next->findChild<GameTab*>(), id == ScreenId::Snake ? "startSnake" : "startPong");

    if (current()) current()->leave();
    if (Prefs::animations && isVisible()) wipe->play(stack->grab());
    stack->setCurrentWidget(next);
    currentId = id;
    next->enter();
}

void MainWindow::showHelp()
{
    if (currentId != ScreenId::Help) go(ScreenId::Help);
}

void MainWindow::toggleFullScreen()
{
    if (PixelDialog::isOpen()) return;
    if (isFullScreen()) showNormal(); else showFullScreen();
}

void MainWindow::closeEvent(QCloseEvent* e)
{
    if (PixelDialog::isOpen()) { e->ignore(); return; }
    if (current()) current()->suspend();
    const bool lose = current() && current()->hasProgressToLose();
    if (PixelDialog::confirmQuit(this, lose))
    {
        if (current()) current()->leave();
        QSettings().setValue("geometry", saveGeometry());
        e->accept();
    }
    else
    {
        e->ignore();
        if (current()) current()->setFocus();
    }
}

void MainWindow::showAbout()
{
    if (current()) current()->suspend();
    PixelDialog d(this, "ABOUT PIXEL-8", 560);
    QHBoxLayout* top = new QHBoxLayout();
    top->setSpacing(16);
    top->addWidget(new SpriteView(SpriteId::Logo, 4), 0, Qt::AlignTop);
    QVBoxLayout* words = new QVBoxLayout();
    words->addWidget(Theme::text("PIXEL-8  v" APP_VERSION, Theme::pixel(16), Theme::amber));
    QLabel* one = Theme::text("An 8-bit computer that runs entirely in software: an assembler, a CPU with "
                              "8 registers and 38 instructions, memory, a 32 x 16 screen and 8 buttons.",
                              Theme::mono(14), Theme::textHi);
    one->setWordWrap(true);
    words->addWidget(one);
    top->addLayout(words, 1);
    d.body()->addLayout(top);

    d.body()->addWidget(Theme::text("TEAM NULL POINTERS", Theme::label(), Theme::textMid));
    QLabel* team = Theme::text("Arnav: parser and assembler\n"
                               "Piyush: operand checker and SystemState\n"
                               "Ashmit: Qt window and games\n"
                               "Aashish: CPU, Bus, Console, calculator, debugger",
                               Theme::mono(13), Theme::textHi);
    d.body()->addWidget(team);
    QLabel* small = Theme::text(QString("Graphic Era (Deemed to be University)\nBuilt with Qt %1, C++17.\n"
                                        "Fonts: Press Start 2P and JetBrains Mono (SIL Open Font License 1.1).")
                                    .arg(qVersion()),
                                Theme::mono(12), Theme::textMid);
    d.body()->addWidget(small);
    d.addButton("Close", PixelButton::Primary, 0);
    d.exec();
    if (current()) current()->setFocus();
}

void MainWindow::showSettings()
{
    if (current()) current()->suspend();
    PixelDialog d(this, "SETTINGS");
    QCheckBox* scan = new QCheckBox("CRT scanlines on the screens");
    QCheckBox* anim = new QCheckBox("Animations (screen wipe, dialogs, blinking)");
    QCheckBox* boot = new QCheckBox("Boot screen with the self-test at start-up");
    scan->setChecked(Prefs::scanlines);
    anim->setChecked(Prefs::animations);
    boot->setChecked(Prefs::bootScreen);
    for (QCheckBox* c : {scan, anim, boot}) d.body()->addWidget(c);
    d.addButton("Cancel", PixelButton::Secondary, 0);
    d.addButton("Save", PixelButton::Primary, 1);
    d.setFocusButton(scan);
    if (d.exec() == 1)
    {
        Prefs::scanlines = scan->isChecked();
        Prefs::animations = anim->isChecked();
        Prefs::bootScreen = boot->isChecked();
        Prefs::save();
        stack->update();
    }
    if (current()) current()->setFocus();
}

void MainWindow::saveShots(const QString& dir)
{
    const ScreenId ids[] = {ScreenId::Home, ScreenId::Calculator, ScreenId::Snake, ScreenId::Help};
    const char* names[] = {"home", "calculator", "arcade", "help"};
    const bool anim = Prefs::animations;
    Prefs::animations = false;
    for (int i = 0; i < 4; i++)
    {
        go(ids[i]);
        QApplication::processEvents();
        grab().save(QDir(dir).filePath(QString("%1.png").arg(names[i])));
    }
    Prefs::animations = anim;
}
