#include "MainWindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDir>
#include <QEventLoop>
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

#include "Theme.h"
#include "Programs.h"
#include "screens/BootScreen.h"
#include "screens/CalculatorScreen.h"
#include "screens/ConsoleScreen.h"
#include "screens/GameScreen.h"
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
        setGeometry(parentWidget()->rect());   // first: the blocks are counted from this size
        image = old.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
        const int cols = (width() + 7) / 8, rows = (height() + 7) / 8;
        order.resize(cols * rows);
        for (int i = 0; i < (int)order.size(); i++) order[i] = i;
        std::shuffle(order.begin(), order.end(), std::mt19937(8));
        done = 0;
        steps = 0;
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

// ---------------------------------------------------------------- MainWindow

MainWindow::MainWindow()
{
    setWindowTitle("PIXEL-8");
    setMinimumSize(1024, 680);

    stack = new QStackedWidget(this);
    setCentralWidget(stack);

    boot = new BootScreen(stack);
    add(ScreenId::Boot, boot);
    connect(boot, &BootScreen::finished, this, [this] { go(ScreenId::Home); });
    HomeScreen* home = new HomeScreen(stack);
    homeScreen = home;
    add(ScreenId::Home, home);
    calculatorScreen = new CalculatorScreen(stack);
    add(ScreenId::Calculator, calculatorScreen);
    consoleScreen = new ConsoleScreen(stack);
    add(ScreenId::Console, consoleScreen);
    gameScreen = new GameScreen(stack);
    add(ScreenId::Snake, gameScreen);
    add(ScreenId::Pong, gameScreen);
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

    Screen* first = Prefs::bootScreen ? static_cast<Screen*>(boot) : home;
    stack->setCurrentWidget(first);
    currentId = Prefs::bootScreen ? ScreenId::Boot : ScreenId::Home;
    QTimer::singleShot(0, first, &Screen::enter);
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
    if ((id == ScreenId::Snake || id == ScreenId::Pong) && currentId != ScreenId::Help)   // a fresh game in READY
        gameScreen->setMode(id == ScreenId::Snake ? GameScreen::SNAKE : GameScreen::PONG);

    if (current()) current()->leave();
    const bool wipeIt = Prefs::animations && isVisible();
    const QPixmap old = wipeIt ? stack->grab() : QPixmap();
    stack->setCurrentWidget(next);
    if (wipeIt) wipe->play(old);   // after the switch: showing a page raises it, the wipe must be on top
    currentId = id;
    next->enter();
}

void MainWindow::showHelp()
{
    if (currentId != ScreenId::Help && currentId != ScreenId::Boot) go(ScreenId::Help);
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
    const bool anim = Prefs::animations;
    Prefs::animations = false;
    auto shot = [&](const QString& name) {
        QApplication::processEvents();
        grab().save(QDir(dir).filePath(name + ".png"));
    };
    auto key = [&](QWidget* to, int k, bool down) {
        QKeyEvent e(down ? QEvent::KeyPress : QEvent::KeyRelease, k, Qt::NoModifier);
        QApplication::sendEvent(to, &e);
    };

    go(ScreenId::Boot);
    shot("00-boot");
    go(ScreenId::Home);
    shot("01-home");
    if (kShowConsole)
    {
        homeScreen->runPreview(1, 30);
        shot("01-home-console-preview");
    }
    const int snakeCard = kShowConsole ? 2 : 1;
    homeScreen->runPreview(snakeCard, 60);
    shot("01-home-snake-preview");
    homeScreen->runPreview(snakeCard + 1, 40);
    shot("01-home-pong-preview");
    homeScreen->runPreview(0, 0);

    go(ScreenId::Calculator);
    shot("02-calculator-ready");
    calculatorScreen->setSum(200, '+', 100);
    shot("02-calculator-carry");
    calculatorScreen->setSum(17, '/', 5);
    shot("02-calculator-divide");
    calculatorScreen->setSum(7, '/', 0);
    shot("02-calculator-div0");
    calculatorScreen->setSum(12, '|', 3);
    shot("02-calculator-or");
    calculatorScreen->setSum(15, '*', 20);
    shot("02-calculator-toobig");

    go(ScreenId::Console);   // loads dot.asm
    key(consoleScreen, Qt::Key_Right, true);
    consoleScreen->runFrames(6);
    key(consoleScreen, Qt::Key_Right, false);
    key(consoleScreen, Qt::Key_Down, true);
    key(consoleScreen, Qt::Key_Left, true);
    consoleScreen->runFrames(3);
    consoleScreen->suspend();
    key(consoleScreen, Qt::Key_Down, true);   // held while the picture is taken: the pad LED is lit
    shot("03-console-dot");
    key(consoleScreen, Qt::Key_Down, false);
    key(consoleScreen, Qt::Key_Left, false);
    const QString tests = QString(PIXEL8_GUI_DIR) + "/tests/programs/";
    consoleScreen->load(findProgram("sum.asm"));   // opens in CODE view: one tick = one line
    consoleScreen->runFrames(14);
    shot("04-console-sum-code");
    consoleScreen->runFrames(200);
    shot("04b-console-sum-done");
    consoleScreen->load(findProgram("sum.asm"));   // halts inside one SCREEN frame, then CODE view must still point at HLT
    consoleScreen->setCodeView(false);
    consoleScreen->runFrames(1);
    consoleScreen->setCodeView(true);
    shot("04c-console-sum-halt-in-screen");
    consoleScreen->load(tests + "fault.asm");
    consoleScreen->setCodeView(false);
    consoleScreen->runFrames(1);
    shot("05-console-fault");
    consoleScreen->load(tests + "bad.asm");
    shot("06-console-errors");
    consoleScreen->load(findProgram("sum.asm"));   // from the Error state: the code listing must not come up blank
    consoleScreen->runFrames(3);
    shot("06b-console-code-after-error");
    consoleScreen->load(tests + "hello.asm");
    consoleScreen->setCodeView(false);
    consoleScreen->runFrames(1);
    shot("07-console-output");
    consoleScreen->load(findProgram("dot.asm"));
    consoleScreen->suspend();

    go(ScreenId::Snake);
    shot("10-snake-ready");
    gameScreen->playTicks(6);
    key(gameScreen, Qt::Key_Down, true);
    gameScreen->playTicks(4);
    key(gameScreen, Qt::Key_Left, true);
    gameScreen->playTicks(3);
    shot("11-snake-playing");
    gameScreen->pauseForShots();
    shot("12-snake-paused");
    key(gameScreen, Qt::Key_Escape, true);     // resume
    gameScreen->playTicks(40);                 // into the wall
    QApplication::processEvents();
    shot("13-snake-over");

    go(ScreenId::Home);
    go(ScreenId::Pong);
    gameScreen->playTicks(25);
    shot("14-pong");
    gameScreen->pauseForShots();
    go(ScreenId::Home);

    go(ScreenId::Help);
    shot("20-help");
    go(ScreenId::Home);

    // the pixel wipe, caught half way (about 3 of its 6 steps)
    Prefs::animations = true;
    go(ScreenId::Help);
    {
        QEventLoop wait;
        QTimer::singleShot(75, &wait, &QEventLoop::quit);
        wait.exec();
    }
    shot("24-wipe");
    {
        QEventLoop wait;
        QTimer::singleShot(250, &wait, &QEventLoop::quit);   // let it finish
        wait.exec();
    }
    Prefs::animations = false;
    QApplication::processEvents();
    go(ScreenId::Home);

    // dialogs: take the picture from inside their event loop, then close them with Esc
    auto dialogShot = [&](const QString& name) {
        QTimer::singleShot(150, this, [this, dir, name] {
            grab().save(QDir(dir).filePath(name + ".png"));
            if (QWidget* d = findChild<PixelDialog*>())
            {
                QKeyEvent esc(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
                QApplication::sendEvent(d, &esc);
            }
        });
    };
    dialogShot("21-exit-dialog");
    PixelDialog::confirmQuit(this, false);
    dialogShot("22-about");
    showAbout();
    dialogShot("23-settings");
    showSettings();

    // the smallest window size: nothing may be cut off
    resize(minimumSize());
    for (ScreenId id : {ScreenId::Home, ScreenId::Calculator, ScreenId::Console, ScreenId::Snake, ScreenId::Help})
    {
        go(id);
        if (id == ScreenId::Calculator) calculatorScreen->setSum(200, '+', 100);
        shot(QString("30-min-%1").arg((int)id));
        if (id == ScreenId::Console)   // CODE view too: source, caption and register names must fit
        {
            consoleScreen->load(findProgram("sum.asm"));
            consoleScreen->runFrames(14);
            consoleScreen->suspend();
            shot("30-min-3-code");
        }
    }
    consoleScreen->suspend();
    go(ScreenId::Home);
    resize(1280, 800);
    Prefs::animations = anim;
}
