#include "screens/BootScreen.h"

#include <QFileInfo>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QPainter>

#include "Aashish/src/Console.h"
#include "Aashish/src/MemoryMap.h"
#include "Programs.h"
#include "Theme.h"
#include "art/SpriteCache.h"

BootScreen::BootScreen(QWidget* parent) : Screen(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    reveal.setInterval(150);
    connect(&reveal, &QTimer::timeout, this, &BootScreen::next);
    blink.setInterval(500);
    connect(&blink, &QTimer::timeout, this, [this] { cursorOn = !cursorOn; update(); });
    done.setSingleShot(true);
    connect(&done, &QTimer::timeout, this, &BootScreen::finished);
}

void BootScreen::runChecks()
{
    checks.clear();
    allOk = true;
    Console console;
    Bus& bus = console.bus();
    using namespace MemoryMap;

    // memory: RAM and VRAM keep what is written, ROM refuses writes
    const bool ramOk = bus.write(RAM_START, 0xA5) && bus.peek(RAM_START) == 0xA5 && bus.write(RAM_END, 0x5A) && bus.peek(RAM_END) == 0x5A;
    const bool vramOk = bus.write(VRAM_START, 1) && console.pixel(0, 0) == 1;
    const bool romOk = !bus.write(ROM_START, 0x42);
    bus.clearFault();
    checks.append({"ROM", "32 KB, read-only", romOk, "a write into ROM was not refused"});
    checks.append({"RAM", "24 KB", ramOk, "a byte written was not read back"});
    checks.append({"VRAM", QString("%1 x %2 screen").arg(SCREEN_WIDTH).arg(SCREEN_HEIGHT), vramOk, "pixel (0, 0) did not light"});

    // self-test: sum.asm must halt with 10 + 9 + ... + 1 = 55 in RAM
    QString why;
    bool sumOk = false;
    const QString sum = findProgram("sum.asm");
    const AsmLoad s = assembleProgram(sum);
    if (sum.isEmpty()) why = "sum.asm not found";
    else if (!s.ok) why = "sum.asm does not assemble";
    else if (!console.loadProgram(s.bytes)) why = "sum.asm does not fit in ROM";
    else
    {
        StepResult r = StepResult::Ok;
        for (int f = 0; f < 10 && r != StepResult::Halt && r != StepResult::Fault; f++) r = console.runFrame();
        const int got = bus.peek(RAM_START);
        sumOk = r == StepResult::Halt && got == 55;
        if (!sumOk) why = r == StepResult::Fault ? QString("CPU fault: %1").arg(faultName(console.cpu().getFault().type))
                                                 : QString("RAM 0x8000 = %1, expected 55").arg(got);
    }
    checks.append({"SELF TEST", "sum.asm on the CPU  R0 = 55", sumOk, why});

    const QString calc = findProgram("calc.asm");
    const AsmLoad c = assembleProgram(calc);
    checks.append({"CALC ROM", c.ok ? QString("calc.asm (%1 bytes)").arg(c.bytes.size()) : QString("calc.asm"), c.ok,
                   calc.isEmpty() ? "calc.asm not found" : "calc.asm has errors"});

    for (const Check& k : checks) allOk &= k.ok;
}

void BootScreen::enter()
{
    runChecks();
    shown = 0;
    setFocus();
    blink.start();
    if (Prefs::animations) reveal.start();
    else showAll();
}

void BootScreen::showAll()
{
    if (checks.isEmpty()) runChecks();
    shown = checks.size();
    reveal.stop();
    update();
    done.start(allOk ? 600 : 2000);   // a failure stays readable for 2 s, then Home anyway
}

void BootScreen::next()
{
    if (++shown >= checks.size()) showAll();
    update();
}

void BootScreen::leave()
{
    reveal.stop();
    blink.stop();
    done.stop();
}

void BootScreen::keyPressEvent(QKeyEvent*)
{
    leave();
    emit finished();
}

void BootScreen::mousePressEvent(QMouseEvent*)
{
    leave();
    emit finished();
}

void BootScreen::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), Theme::bgVoid);

    const int w = 640;
    const int left = (width() - w) / 2;
    int y = std::max(40, height() / 2 - 190);

    SpriteCache::draw(p, SpriteId::Logo, QPointF(left, y), 4);
    p.setFont(Theme::pixel(32));
    p.setPen(Theme::amber);
    p.drawText(QPoint(left + 92 + 4, y + 40 + 4), "PIXEL-8");
    p.setPen(Theme::textHi);
    p.drawText(QPoint(left + 92, y + 40), "PIXEL-8");
    p.setFont(Theme::pixel(8));
    p.setPen(Theme::textMid);
    p.drawText(QPoint(left + 94, y + 60), "8-BIT COMPUTER");
    y += 112;

    p.setFont(Theme::pixel(8));
    const QFontMetrics fm(p.font());
    for (int i = 0; i < shown && i < checks.size(); i++)
    {
        const Check& k = checks[i];
        p.setPen(Theme::textHi);
        p.drawText(QPoint(left, y), k.name);
        p.setPen(Theme::textMid);
        p.drawText(QPoint(left + 112, y), k.detail);
        const QString status = k.ok ? "OK" : "FAIL";
        // dot leader up to the status
        const int from = left + 112 + fm.horizontalAdvance(k.detail) + 12, to = left + w - 48;
        p.setPen(Theme::lineStrong);
        for (int x = from; x < to; x += 12) p.drawText(QPoint(x, y), ".");
        p.setPen(k.ok ? Theme::green : Theme::red);
        p.drawText(QPoint(left + w - 32, y), status);
        y += 24;
        if (!k.ok)
        {
            p.setFont(Theme::mono(13));
            p.setPen(Theme::red);
            p.drawText(QPoint(left + 112, y - 6), k.why);
            p.setFont(Theme::pixel(8));
            y += 18;
        }
    }

    if (shown >= checks.size() && !checks.isEmpty())
    {
        y += 32;
        p.setFont(Theme::pixel(16));
        p.setPen(allOk ? Theme::textHi : Theme::red);
        const QString msg = allOk ? QString("PRESS ANY KEY") + (cursorOn ? "_" : " ") : "SELF TEST FAILED";
        p.drawText(QRect(0, y, width(), 24), Qt::AlignHCenter, msg);
    }
}
