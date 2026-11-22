#include "screens/HelpScreen.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "Aashish/src/MemoryMap.h"
#include "Theme.h"
#include "widgets/Pixel.h"

namespace
{
    const QString UP(QChar(0x2191)), DOWN(QChar(0x2193)), LEFT(QChar(0x2190)), RIGHT(QChar(0x2192)),
        ENTER(QChar(0x23CE));

    enum Section { Global, Calc, Console, Snake, Pong, Memory };

    // keycaps + a description on one row
    QWidget* keyRow(const QStringList& keys, const QString& what)
    {
        QWidget* w = new QWidget();
        QHBoxLayout* row = new QHBoxLayout(w);
        row->setContentsMargins(0, 2, 0, 2);
        row->setSpacing(4);
        for (const QString& k : keys)
        {
            if (k == "/") row->addWidget(Theme::text("or", Theme::mono(12), Theme::textDim));
            else row->addWidget(new KeyCap(k));
        }
        row->addSpacing(12);
        row->addWidget(Theme::text(what, Theme::mono(14), Theme::textHi), 1);
        return w;
    }

    QLabel* para(const QString& text)
    {
        QLabel* l = Theme::text(text, Theme::mono(14), Theme::textMid);
        l->setWordWrap(true);
        return l;
    }

    // the 64 KB address space as a bar, each part to scale (the I/O page gets a minimum width)
    class MemoryBar : public QWidget
    {
    public:
        MemoryBar() { setFixedHeight(76); }

    protected:
        void paintEvent(QPaintEvent*) override
        {
            struct Part { const char* name; int start, end; QColor c; };
            using namespace MemoryMap;
            const Part parts[] = {{"ROM", ROM_START, ROM_END, Theme::amber},
                                  {"RAM", RAM_START, RAM_END, Theme::green},
                                  {"VRAM", VRAM_START, VRAM_END, Theme::greenDeep},
                                  {"IO", IO_START, IO_END, Theme::red},
                                  {"STACK", STACK_START, STACK_END, Theme::cyan}};
            QPainter p(this);
            const int w = width() - 4;
            int x = 2;
            for (const Part& part : parts)
            {
                int pw = (int)((part.end - part.start + 1) * (qint64)w / 65536);
                if (pw < 10) pw = 10;
                if (part.end == STACK_END) pw = width() - 2 - x;
                p.fillRect(x, 2, pw - 2, 30, part.c);
                p.setFont(Theme::pixel(8));
                p.setPen(Theme::bgVoid);
                if (pw > 50) p.drawText(QRect(x, 2, pw - 2, 30), Qt::AlignCenter, part.name);
                p.setFont(Theme::mono(12));
                p.setPen(Theme::textMid);
                p.drawText(QPoint(x, 50), QString("%1").arg(part.start, 4, 16, QChar('0')).toUpper());
                x += pw;
            }
            p.drawText(QRect(0, 54, width() - 2, 20), Qt::AlignRight, "FFFF");
        }
    };

    Panel* page(const QString& title, QWidget* parent)
    {
        Panel* p = new Panel(title, parent);
        return p;
    }
}

HelpScreen::HelpScreen(QWidget* parent) : Screen(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    QVBoxLayout* v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);
    ScreenHeader* header = new ScreenHeader("HELP", this, "BACK");
    connect(header, &ScreenHeader::back, this, &Screen::back);
    v->addWidget(header);

    QHBoxLayout* body = new QHBoxLayout();
    body->setContentsMargins(24, 24, 24, 24);
    body->setSpacing(16);
    list = new QListWidget(this);
    list->setFixedWidth(220);
    list->setFont(Theme::label());
    list->addItems({"CONTROLS", "CALCULATOR", "CONSOLE", "SNAKE", "PONG", "MEMORY MAP"});
    body->addWidget(list, 0, Qt::AlignTop);
    list->setFixedHeight(list->count() * 40 + 8);

    pages = new QStackedWidget(this);
    body->addWidget(pages, 1);
    v->addLayout(body, 1);

    // Global
    Panel* g = page("CONTROLS: EVERYWHERE", this);
    g->body()->addWidget(keyRow({UP, DOWN, LEFT, RIGHT}, "Home: choose a program"));
    g->body()->addWidget(keyRow({ENTER, "/", "1", "2", "3", "4"}, "Home: launch it"));
    g->body()->addWidget(keyRow({"ESC"}, "Back to the menu (in a game: pause menu). On Home: exit."));
    g->body()->addWidget(keyRow({"F1"}, "This help, from any screen"));
    g->body()->addWidget(keyRow({"F11"}, "Full screen on / off"));
    g->body()->addWidget(keyRow({"TAB"}, "Move between buttons"));
    g->body()->addStretch(1);
    pages->addWidget(g);

    // Calculator
    Panel* c = page("CONTROLS: CALCULATOR", this);
    c->body()->addWidget(keyRow({"0-9"}, "Type a number (0 to 255) into A or B"));
    c->body()->addWidget(keyRow({"+", "-", "*", "/", "&", "|", "^"}, "Choose the operation"));
    c->body()->addWidget(keyRow({ENTER}, "Run calc.asm on the CPU"));
    c->body()->addWidget(keyRow({"ESC"}, "Back to the menu"));
    c->body()->addSpacing(8);
    c->body()->addWidget(para("Click a bit to flip it. Click a row of the trace to see the registers after "
                              "that line. REPLAY walks through the rows. Rows are lines of calc.asm in source "
                              "order, not single instructions, so loops show their final values."));
    c->body()->addStretch(1);
    pages->addWidget(c);

    // Console
    Panel* k = page("CONTROLS: CONSOLE", this);
    k->body()->addWidget(keyRow({UP, DOWN, LEFT, RIGHT, "/", "W", "A", "S", "D"}, "D-pad (BTN_UP, DOWN, LEFT, RIGHT)"));
    k->body()->addWidget(keyRow({"Z", "/", "J"}, "A button"));
    k->body()->addWidget(keyRow({"X", "/", "K"}, "B button"));
    k->body()->addWidget(keyRow({ENTER}, "START button"));
    k->body()->addWidget(keyRow({"SHIFT"}, "SELECT button"));
    k->body()->addWidget(keyRow({"SPACE"}, "Run / pause (the screen's key, programs never see it)"));
    k->body()->addWidget(keyRow({"F5"}, "Step one frame (SCREEN) or one line (CODE) while paused"));
    k->body()->addWidget(keyRow({"C"}, "Show the program's code / the screen"));
    k->body()->addSpacing(8);
    k->body()->addWidget(Theme::text("HOW THE KEYS REACH THE CPU", Theme::label(), Theme::textMid));
    k->body()->addWidget(para("Key held  >  IoPorts::setButton  >  port 0xF000 (one bit per button, 1 = held)"
                              "  >  the program reads it with LD R3, [0xF000] once per frame. Releasing the key "
                              "clears the bit. Each frame the program waits for port 0xF003 (the frame counter) "
                              "to change, so it runs about 20 times a second."));
    k->body()->addStretch(1);
    pages->addWidget(k);

    // Snake
    Panel* s = page("CONTROLS: SNAKE", this);
    s->body()->addWidget(keyRow({ENTER, "/", "SPACE"}, "Start"));
    s->body()->addWidget(keyRow({UP, DOWN, LEFT, RIGHT, "/", "W", "A", "S", "D"}, "Turn (a 180 degree turn is ignored)"));
    s->body()->addWidget(keyRow({"P", "/", "SPACE", "/", "ESC"}, "Pause menu"));
    s->body()->addWidget(keyRow({"R"}, "Restart"));
    s->body()->addSpacing(8);
    s->body()->addWidget(para("Eat the byte chips to grow. Hitting a wall or yourself ends the game. Your best "
                              "score is kept. These rules are C++ (a preview); the assembly version will run "
                              "in the Console."));
    s->body()->addStretch(1);
    pages->addWidget(s);

    // Pong
    Panel* o = page("CONTROLS: PONG", this);
    o->body()->addWidget(keyRow({ENTER, "/", "SPACE"}, "Start"));
    o->body()->addWidget(keyRow({UP, DOWN, "/", "W", "S"}, "Move your paddle (hold)"));
    o->body()->addWidget(keyRow({"P", "/", "SPACE", "/", "ESC"}, "Pause menu"));
    o->body()->addWidget(keyRow({"R"}, "Restart"));
    o->body()->addSpacing(8);
    o->body()->addWidget(para("You are the green paddle on the left. COM, the computer, is cyan. First to 5 "
                              "points wins. COM is a little slow on purpose, so it can be beaten."));
    o->body()->addStretch(1);
    pages->addWidget(o);

    // Memory map
    Panel* m = page("MEMORY MAP: 64 KB", this);
    m->body()->addWidget(new MemoryBar());
    m->body()->addWidget(para("ROM 0000-7FFF (32 KB): the program.   RAM 8000-DFFF (24 KB): variables.\n"
                              "VRAM E000-EFFF: the screen, 32 x 16 pixels, one byte each at E000 + y*32 + x.\n"
                              "IO F000-F0FF: F000 buttons, F001 random, F002 PPU control, F003 frame counter, "
                              "F004 text out.\nSTACK F100-FFFF: SP starts at FFFF and grows down."));
    m->body()->addSpacing(8);
    m->body()->addWidget(Theme::text("SCREEN COLOURS (VRAM BYTE)", Theme::label(), Theme::textMid));
    QHBoxLayout* swatches = new QHBoxLayout();
    swatches->setSpacing(12);
    for (int i = 0; i < 8; i++)
    {
        QLabel* sw = new QLabel(QString::number(i));
        sw->setAlignment(Qt::AlignCenter);
        sw->setFixedSize(40, 28);
        sw->setFont(Theme::pixel(8));
        sw->setAutoFillBackground(true);
        QPalette pal = sw->palette();
        pal.setColor(QPalette::Window, Theme::vramColour(i));
        pal.setColor(QPalette::WindowText, i == 0 || i == 6 || i == 7 ? Theme::textHi : Theme::bgVoid);
        sw->setPalette(pal);
        swatches->addWidget(sw);
    }
    swatches->addWidget(Theme::text("8-255 = 1", Theme::mono(12), Theme::textDim));
    swatches->addStretch(1);
    m->body()->addLayout(swatches);
    m->body()->addStretch(1);
    pages->addWidget(m);

    connect(list, &QListWidget::currentRowChanged, pages, &QStackedWidget::setCurrentIndex);
    list->setCurrentRow(0);
}

void HelpScreen::showFor(ScreenId from)
{
    Section s = Global;
    switch (from)
    {
    case ScreenId::Calculator: s = Calc; break;
    case ScreenId::Console: s = Console; break;
    case ScreenId::Snake: s = Snake; break;
    case ScreenId::Pong: s = Pong; break;
    default: break;
    }
    list->setCurrentRow(s);
}

void HelpScreen::enter()
{
    list->setFocus();
}

void HelpScreen::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) { emit back(); return; }
    Screen::keyPressEvent(e);
}
