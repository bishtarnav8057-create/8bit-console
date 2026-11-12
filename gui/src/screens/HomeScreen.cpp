#include "screens/HomeScreen.h"

#include <QFontMetrics>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <functional>

#include "Theme.h"
#include "art/SpriteCache.h"
#include "widgets/Pixel.h"

namespace
{
    enum class Badge { RunsOnCpu, CppPreview };

    // One row per program: add a row to add a program (plan 7.3).
    struct ProgramEntry
    {
        ScreenId target;
        const char* title;
        const char* blurb;      // card, 2 lines
        const char* about;      // preview pane
        SpriteId icon;
        Badge badge;
        const char* section;    // a section label is shown before the first entry of a new section
        QVector<FooterHints::Hint> keys;
    };

    const QVector<ProgramEntry>& programs()
    {
        static const QVector<ProgramEntry> list = {
            {ScreenId::Calculator, "CALCULATOR",
             "Two bytes, one operation. calc.asm runs it on our CPU.",
             "Pick two 8-bit numbers and an operation. The answer comes from our CPU running calc.asm, "
             "with every register, flag and line it ran.",
             SpriteId::Calc, Badge::RunsOnCpu, "RUNS ON THE PIXEL-8 CPU",
             {{{"+", "-", "*", "/"}, "OPERATION"}, {{QString(QChar(0x23CE))}, "RUN"}}},
            {ScreenId::Console, "CONSOLE",
             "Run any .asm program: screen, keys and CPU, live.",
             "The emulator itself. Load any .asm file and run it on the CPU: its 32 x 16 screen, the "
             "buttons it reads and the registers, frame by frame.",
             SpriteId::ConsoleIcon, Badge::RunsOnCpu, "RUNS ON THE PIXEL-8 CPU",
             {{{QString(QChar(0x2191)), QString(QChar(0x2193)), QString(QChar(0x2190)), QString(QChar(0x2192))}, "PAD"},
              {{"Z"}, "A"}, {{"X"}, "B"}}},
            {ScreenId::Snake, "SNAKE",
             "Eat bytes, grow, don't hit the walls.",
             "Steer the snake to the byte chips. Every byte makes it longer. Hit a wall or yourself and "
             "it's over. Rules in C++ for now; the assembly version will run in the Console.",
             SpriteId::SnakeIcon, Badge::CppPreview, "ARCADE",
             {{{QString(QChar(0x2191)), QString(QChar(0x2193)), QString(QChar(0x2190)), QString(QChar(0x2192))}, "TURN"},
              {{"P"}, "PAUSE"}}},
            {ScreenId::Pong, "PONG",
             "First to 5 against the computer.",
             "Your paddle on the left, the computer (COM) on the right. First to 5 points wins. "
             "Rules in C++ for now.",
             SpriteId::PongIcon, Badge::CppPreview, "ARCADE",
             {{{QString(QChar(0x2191)), QString(QChar(0x2193))}, "PADDLE"}, {{"P"}, "PAUSE"}}},
        };
        return list;
    }

    // ------------------------------------------------------------ header strip with logo and wordmark
    class HomeHeader : public QWidget
    {
    public:
        explicit HomeHeader(QWidget* parent) : QWidget(parent) { setFixedHeight(120); }

    protected:
        void paintEvent(QPaintEvent*) override
        {
            QPainter p(this);
            p.fillRect(rect(), Theme::bgPanel);
            SpriteCache::draw(p, SpriteId::Logo, QPointF(28, 26), 4);

            p.setFont(Theme::pixel(32));
            p.setPen(Theme::amber);
            p.drawText(QPoint(116 + 4, 66 + 4), "PIXEL-8");
            p.setPen(Theme::textHi);
            p.drawText(QPoint(116, 66), "PIXEL-8");

            p.setFont(Theme::label());
            p.setPen(Theme::textMid);
            p.drawText(QPoint(118, 92), QString("8-BIT COMPUTER  %1  TEAM NULL POINTERS").arg(QChar(0x00B7)));

            p.setFont(Theme::mono(12));
            p.setPen(Theme::textDim);
            p.drawText(QRect(0, 14, width() - 24, 20), Qt::AlignRight, "v" APP_VERSION);

            p.fillRect(0, height() - 4, width(), 2, Theme::line);
            const QColor strip[4] = {Theme::green, Theme::amber, Theme::red, Theme::cyan};
            for (int x = 0, i = 0; x < width(); x += 8, i++) p.fillRect(x, height() - 2, 8, 2, strip[i % 4]);
        }
    };

    class SectionLabel : public QWidget
    {
    public:
        SectionLabel(const QString& t, QWidget* parent) : QWidget(parent), text(t) { setFixedHeight(24); }

    protected:
        void paintEvent(QPaintEvent*) override
        {
            QPainter p(this);
            p.fillRect(2, 9, 6, 6, Theme::amber);
            p.setFont(Theme::label());
            p.setPen(Theme::textMid);
            p.drawText(QRect(16, 0, width() - 16, height()), Qt::AlignVCenter | Qt::AlignLeft, text);
        }

    private:
        QString text;
    };

    // key hints as a widget (same painting as the footer)
    class HintsView : public QWidget
    {
    public:
        HintsView(QWidget* parent) : QWidget(parent) { setFixedHeight(Theme::KEYCAP_H); }
        void setHints(const QVector<FooterHints::Hint>& h) { hints = h; update(); }

    protected:
        void paintEvent(QPaintEvent*) override
        {
            QPainter p(this);
            FooterHints::paintHints(p, 0, 0, hints);
        }

    private:
        QVector<FooterHints::Hint> hints;
    };
}

// ---------------------------------------------------------------- ProgramCard
// 280 x 196 card plus room to lift by 2 px and cast a 6 px shadow.
class ProgramCard : public QWidget
{
public:
    ProgramCard(const ProgramEntry& e, int hotkey, QWidget* parent)
        : QWidget(parent), entry(e), key(hotkey)
    {
        setFixedSize(288, 204);
        setFocusPolicy(Qt::StrongFocus);
        setAttribute(Qt::WA_Hover);
        setCursor(Qt::PointingHandCursor);
        idle.setInterval(400);
        QObject::connect(&idle, &QTimer::timeout, this, [this] { frame ^= 1; update(); });
    }

    std::function<void()> onActivate, onFocus;

protected:
    bool active() const { return hasFocus() || underMouse(); }

    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        const bool on = active();
        const QRect card = on ? QRect(0, 0, 280, 196) : QRect(2, 2, 280, 196);
        Theme::drawBox(p, card, Theme::bgPanel, on ? Theme::amber : Theme::line, on ? 6 : 4);

        const int x = card.x() + 16, y = card.y() + 16;
        SpriteCache::draw(p, entry.icon, QPointF(x, y - (on && frame ? 2 : 0)), 4);
        Theme::drawKeyCap(p, card.right() - 16 - Theme::keyCapWidth(QString::number(key)), y,
                          QString::number(key));

        p.setFont(Theme::pixel(16));
        p.setPen(on ? Theme::amber : Theme::textHi);
        p.drawText(QPoint(x, y + 64 + 26), entry.title);

        p.setFont(Theme::mono(13));
        p.setPen(Theme::textMid);
        p.drawText(QRect(x, y + 64 + 34, 248, 40), Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, entry.blurb);

        // badge
        const bool cpu = entry.badge == Badge::RunsOnCpu;
        const QString text = cpu ? "RUNS ON PIXEL-8 CPU" : "C++ PREVIEW";
        const int tw = QFontMetrics(Theme::pixel(8)).horizontalAdvance(text);
        const int bw = tw + 16 + (cpu ? 12 : 0);
        const QRect badge(x, card.bottom() - 16 - 20 + 1, bw, 20);
        p.fillRect(badge, cpu ? Theme::greenBadge : Theme::bgRaised);
        int tx = badge.x() + 8;
        if (cpu) { p.fillRect(tx, badge.y() + 7, 6, 6, Theme::green); tx += 12; }
        p.setFont(Theme::pixel(8));
        p.setPen(cpu ? Theme::green : Theme::textMid);
        p.drawText(QRect(tx, badge.y(), tw + 4, 20), Qt::AlignVCenter | Qt::AlignLeft, text);
    }

    void focusInEvent(QFocusEvent*) override { idle.start(); if (onFocus) onFocus(); update(); }
    void focusOutEvent(QFocusEvent*) override { idle.stop(); frame = 0; update(); }
    void enterEvent(QEnterEvent*) override { update(); }
    void leaveEvent(QEvent*) override { update(); }
    void mousePressEvent(QMouseEvent*) override { setFocus(Qt::MouseFocusReason); }
    void mouseReleaseEvent(QMouseEvent* e) override
    {
        if (rect().contains(e->position().toPoint()) && onActivate) onActivate();
    }
    void keyPressEvent(QKeyEvent* e) override
    {
        if ((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter || e->key() == Qt::Key_Space) && onActivate)
            onActivate();
        else e->ignore();   // arrows, digits and Esc go to HomeScreen
    }

private:
    const ProgramEntry& entry;
    int key;
    int frame = 0;
    QTimer idle;
};

// ---------------------------------------------------------------- PreviewPane

class PreviewPane : public Panel
{
public:
    explicit PreviewPane(QWidget* parent) : Panel("PREVIEW", parent)
    {
        setMinimumWidth(340);
        setMaximumWidth(420);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        title = Theme::text("", Theme::pixel(16), Theme::textHi, this);
        stage = new QStackedWidget(this);
        stage->setMinimumHeight(180);
        for (const ProgramEntry& e : programs()) stage->addWidget(new SpriteView(e.icon, 8, stage));
        about = Theme::text("", Theme::mono(13), Theme::textMid, this);
        about->setWordWrap(true);
        hints = new HintsView(this);
        launch = new PixelButton("Launch", PixelButton::Primary, this);
        launch->setHeightKind(PixelButton::Hero);
        launch->setGlyph(SpriteId::Play);
        launch->setTrailingGlyph(SpriteId::Enter);

        body()->addWidget(stage, 1);
        body()->addWidget(title);
        body()->addWidget(about);
        body()->addSpacing(4);
        body()->addWidget(hints);
        body()->addStretch(0);
        body()->addWidget(launch);
    }

    void show(int i)
    {
        const ProgramEntry& e = programs().at(i);
        title->setText(e.title);
        about->setText(e.about);
        hints->setHints(e.keys);
        stage->setCurrentIndex(i);
    }

    PixelButton* launch;

private:
    QLabel* title;
    QLabel* about;
    HintsView* hints;
    QStackedWidget* stage;
};

// ---------------------------------------------------------------- HomeScreen

HomeScreen::HomeScreen(QWidget* parent) : Screen(parent)
{
    QVBoxLayout* page = new QVBoxLayout(this);
    page->setContentsMargins(0, 0, 0, 0);
    page->setSpacing(0);
    page->addWidget(new HomeHeader(this));

    QHBoxLayout* middle = new QHBoxLayout();
    middle->setContentsMargins(24, 20, 24, 20);
    middle->setSpacing(24);

    QVBoxLayout* left = new QVBoxLayout();
    left->setSpacing(8);
    QString section;
    QGridLayout* grid = nullptr;
    for (int i = 0; i < programs().size(); i++)
    {
        const ProgramEntry& e = programs().at(i);
        if (e.section != section)
        {
            section = e.section;
            left->addWidget(new SectionLabel(section, this));
            grid = new QGridLayout();
            grid->setHorizontalSpacing(16);
            grid->setVerticalSpacing(16);
            grid->setAlignment(Qt::AlignLeft | Qt::AlignTop);
            left->addLayout(grid);
            left->addSpacing(8);
        }
        ProgramCard* card = new ProgramCard(e, i + 1, this);
        card->onActivate = [this, i] { launch(i); };
        card->onFocus = [this, i] { focused = i; preview->show(i); };
        grid->addWidget(card, grid->count() / 2, grid->count() % 2);
        cards.append(card);
    }
    left->addStretch(1);
    middle->addLayout(left, 1);

    preview = new PreviewPane(this);
    connect(preview->launch, &QPushButton::clicked, this, [this] { launch(focused); });
    middle->addWidget(preview, 1);
    page->addLayout(middle, 1);

    FooterHints* footer = new FooterHints(this);
    footer->setHints({{{QString(QChar(0x2191)), QString(QChar(0x2193)), QString(QChar(0x2190)), QString(QChar(0x2192))}, "SELECT"},
                      {{QString(QChar(0x23CE))}, "LAUNCH"}, {{"F1"}, "HELP"}, {{"ESC"}, "EXIT"}});
    PixelButton* help = new PixelButton("Help", PixelButton::Ghost, footer);
    PixelButton* about = new PixelButton("About", PixelButton::Ghost, footer);
    PixelButton* settings = new PixelButton("Settings", PixelButton::Ghost, footer);
    PixelButton* exit = new PixelButton("Exit", PixelButton::Secondary, footer);
    exit->setGlyph(SpriteId::Power);
    for (PixelButton* b : {help, about, settings, exit})
    {
        b->setHeightKind(PixelButton::Small);
        footer->right()->addWidget(b);
    }
    connect(help, &QPushButton::clicked, this, &HomeScreen::helpRequested);
    connect(about, &QPushButton::clicked, this, &HomeScreen::aboutRequested);
    connect(settings, &QPushButton::clicked, this, &HomeScreen::settingsRequested);
    connect(exit, &QPushButton::clicked, this, &HomeScreen::exitRequested);
    page->addWidget(footer);

    preview->show(0);
}

void HomeScreen::enter()
{
    focusCard(focused);
}

void HomeScreen::leave() {}
void HomeScreen::suspend() {}

void HomeScreen::focusCard(int i)
{
    focused = (i + cards.size()) % cards.size();
    cards[focused]->setFocus(Qt::OtherFocusReason);
    preview->show(focused);
}

void HomeScreen::launch(int i)
{
    emit navigate(programs().at(i).target);
}

void HomeScreen::keyPressEvent(QKeyEvent* e)
{
    const int k = e->key();
    if (k == Qt::Key_Escape) { emit exitRequested(); return; }
    if (k >= Qt::Key_1 && k < Qt::Key_1 + cards.size()) { focusCard(k - Qt::Key_1); launch(k - Qt::Key_1); return; }
    if (k == Qt::Key_Return || k == Qt::Key_Enter) { launch(focused); return; }
    // 2-column grid: left/right move by 1, up/down by 2
    if (k == Qt::Key_Left) { focusCard(focused - 1); return; }
    if (k == Qt::Key_Right) { focusCard(focused + 1); return; }
    if (k == Qt::Key_Up) { focusCard(focused - 2); return; }
    if (k == Qt::Key_Down) { focusCard(focused + 2); return; }
    Screen::keyPressEvent(e);
}
