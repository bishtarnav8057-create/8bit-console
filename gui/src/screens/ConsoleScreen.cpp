#include "screens/ConsoleScreen.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "Aashish/src/Disassembler.h"
#include "Aashish/src/MemoryMap.h"
#include "Programs.h"
#include "Theme.h"
#include "art/SpriteCache.h"
#include "widgets/CpuViews.h"
#include "widgets/Pixel.h"

namespace
{
    const int FRAME_MS = 50;   // about 20 frames a second, like demo/dot.cpp (handoff section 4)

    QString hex4(int v) { return QString("0x%1").arg(v, 4, 16, QChar('0')).toUpper().replace("0X", "0x"); }

    QString grouped(unsigned long n)   // 1204311 -> "1 204 311"
    {
        QString s = QString::number(n);
        for (int i = s.size() - 3; i > 0; i -= 3) s.insert(i, ' ');
        return s;
    }
}

// ---------------------------------------------------------------- PadView
// The 8 buttons as LEDs: a D-pad, A, B, START and SELECT. Lit while held.
class PadView : public QWidget
{
public:
    explicit PadView(QWidget* parent) : QWidget(parent) { setFixedSize(236, 52); }
    void setInput(Byte b) { if (b != input) { input = b; update(); } }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        auto led = [&](int x, int y, int bit) { Led::paintLed(p, x, y, (input >> bit) & 1, Theme::amber); };
        led(20, 4, IoPorts::BTN_UP);
        led(4, 20, IoPorts::BTN_LEFT);
        led(36, 20, IoPorts::BTN_RIGHT);
        led(20, 36, IoPorts::BTN_DOWN);
        p.fillRect(22, 22, 6, 6, Theme::line);

        p.setFont(Theme::pixel(8));
        auto button = [&](int x, int bit, const char* label) {
            led(x, 12, bit);
            p.setPen((input >> bit) & 1 ? Theme::amber : Theme::textDim);
            p.drawText(QRect(x - 20, 28, 50, 14), Qt::AlignCenter, label);
        };
        button(76, IoPorts::BTN_A, "A");
        button(108, IoPorts::BTN_B, "B");
        button(152, IoPorts::BTN_START, "START");
        button(206, IoPorts::BTN_SELECT, "SELECT");
    }

private:
    Byte input = 0;
};

// ---------------------------------------------------------------- TraceView
// The last instructions the CPU started, newest at the bottom (as many as fit).
class TraceView : public QWidget
{
public:
    explicit TraceView(QWidget* parent) : QWidget(parent) {}
    void setLines(const QStringList& l) { lines = l; update(); }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        const int rowH = 18;
        const int fit = std::max(1, height() / rowH);
        const int first = std::max(0, (int)lines.size() - fit);
        p.setFont(Theme::mono(12));
        for (int i = first, y = 0; i < lines.size(); i++, y += rowH)
        {
            const bool newest = i == lines.size() - 1;
            if (newest)
            {
                p.fillRect(0, y, width(), rowH, Theme::bgRaised);
                p.fillRect(0, y, 2, rowH, Theme::amber);
            }
            p.setPen(newest ? Theme::textHi : Theme::textMid);
            p.drawText(QRect(10, y, width() - 10, rowH), Qt::AlignVCenter | Qt::AlignLeft, lines[i]);
        }
        if (lines.isEmpty())
        {
            p.setPen(Theme::textDim);
            p.drawText(rect(), Qt::AlignCenter, "Nothing has run yet.");
        }
    }

private:
    QStringList lines;
};

// ---------------------------------------------------------------- ConsoleScreen

ConsoleScreen::ConsoleScreen(QWidget* parent) : Screen(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    timer.setTimerType(Qt::PreciseTimer);
    timer.setInterval(FRAME_MS);
    connect(&timer, &QTimer::timeout, this, &ConsoleScreen::tick);

    QVBoxLayout* page = new QVBoxLayout(this);
    page->setContentsMargins(0, 0, 0, 0);
    page->setSpacing(0);

    header = new ScreenHeader("CONSOLE", this);
    connect(header, &ScreenHeader::back, this, &Screen::back);
    chip = new StatusChip(header);
    frameLabel = Theme::text("", Theme::pixel(8), Theme::textMid, header);
    header->right()->addWidget(chip);
    header->right()->addWidget(frameLabel);
    page->addWidget(header);

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(24, 16, 24, 16);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(12);
    page->addLayout(grid, 1);

    // ---- cartridges
    Panel* cartPanel = new Panel("CARTRIDGES", this);
    cartPanel->setFixedWidth(220);
    carts = new QListWidget(cartPanel);
    carts->setFocusPolicy(Qt::NoFocus);
    carts->setIconSize(QSize(24, 24));
    carts->setFont(Theme::mono(13));
    carts->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    carts->setTextElideMode(Qt::ElideRight);
    const QIcon cartIcon(SpriteCache::pixmap(SpriteId::Cartridge, 2));
    const struct { const char* file; const char* about; } builtIn[] = {
        {"dot.asm", "Move the dot"}, {"sum.asm", "Adds 10..1, halts"}};
    for (const auto& b : builtIn)
    {
        const QString found = findProgram(b.file);
        if (found.isEmpty()) continue;
        QListWidgetItem* it = new QListWidgetItem(cartIcon, QString("%1\n%2").arg(b.file, b.about), carts);
        it->setData(Qt::UserRole, found);
        it->setToolTip(found);
    }
    connect(carts, &QListWidget::itemClicked, this, [this](QListWidgetItem* it) {
        load(it->data(Qt::UserRole).toString());
        setFocus();
    });
    PixelButton* loadButton = new PixelButton("Load .asm...", PixelButton::Secondary, cartPanel);
    loadButton->setHeightKind(PixelButton::Small);
    loadButton->setFocusPolicy(Qt::NoFocus);
    connect(loadButton, &QPushButton::clicked, this, &ConsoleScreen::browse);
    cartPanel->body()->addWidget(carts, 1);
    cartPanel->body()->addWidget(loadButton);
    grid->addWidget(cartPanel, 0, 0);

    // ---- centre: the screen (or the error list)
    centre = new QStackedWidget(this);
    QWidget* screenPage = new QWidget(centre);
    QVBoxLayout* sp = new QVBoxLayout(screenPage);
    sp->setContentsMargins(0, 0, 0, 0);
    sp->setSpacing(12);
    QGridLayout* stage = new QGridLayout();
    screen = new VramView(screenPage);
    stage->addWidget(screen, 0, 0);
    card = new Panel(QString(), screenPage);
    card->setColours(Theme::bgPanel, Theme::lineStrong);
    card->setFixedWidth(360);
    cardTitle = Theme::text("", Theme::pixel(16), Theme::green, card);
    cardText = Theme::text("", Theme::mono(13), Theme::textHi, card);
    cardText->setWordWrap(true);
    PixelButton* cardReset = new PixelButton("Reset", PixelButton::Primary, card);
    cardReset->setGlyph(SpriteId::Reset);
    cardReset->setFocusPolicy(Qt::NoFocus);
    connect(cardReset, &QPushButton::clicked, this, [this] { reset(); run(); });
    card->body()->addWidget(cardTitle);
    card->body()->addWidget(cardText);
    card->body()->addWidget(cardReset, 0, Qt::AlignRight);
    card->hide();
    stage->addWidget(card, 0, 0, Qt::AlignCenter);
    sp->addLayout(stage, 1);

    QHBoxLayout* controls = new QHBoxLayout();
    controls->setSpacing(4);
    runButton = new PixelButton("Pause", PixelButton::Primary, screenPage);
    stepButton = new PixelButton("Step frame", PixelButton::Secondary, screenPage);
    PixelButton* resetButton = new PixelButton("Reset", PixelButton::Secondary, screenPage);
    resetButton->setGlyph(SpriteId::Reset);
    for (PixelButton* b : {runButton, stepButton, resetButton})
    {
        b->setFocusPolicy(Qt::NoFocus);   // the keys must keep reaching the program
        controls->addWidget(b);
    }
    connect(runButton, &QPushButton::clicked, this, [this] { state == State::Running ? pause() : run(); });
    connect(stepButton, &QPushButton::clicked, this, [this] { if (state == State::Paused) runFrames(1); });
    connect(resetButton, &QPushButton::clicked, this, [this] { const bool was = state == State::Running; reset(); if (was) run(); });
    controls->addStretch(1);
    sp->addLayout(controls);
    centre->addWidget(screenPage);

    Panel* errorPage = new Panel(QString(), centre);
    QHBoxLayout* errTop = new QHBoxLayout();
    errTop->addWidget(new SpriteView(SpriteId::CartridgeX, 4, errorPage));
    errorTitle = Theme::text("", Theme::pixel(16), Theme::red, errorPage);
    errorTitle->setWordWrap(true);
    errTop->addWidget(errorTitle, 1);
    errorPage->body()->addLayout(errTop);
    errorPage->body()->addWidget(Theme::text("Fix the file in an editor, then press RETRY.", Theme::mono(13), Theme::textMid));
    errorList = new ErrorList(errorPage);
    errorPage->body()->addWidget(errorList, 1);
    QHBoxLayout* errButtons = new QHBoxLayout();
    errButtons->addStretch(1);
    PixelButton* other = new PixelButton("Load another...", PixelButton::Secondary, errorPage);
    PixelButton* retry = new PixelButton("Retry", PixelButton::Primary, errorPage);
    retry->setGlyph(SpriteId::Reset);
    for (PixelButton* b : {other, retry}) { b->setFocusPolicy(Qt::NoFocus); errButtons->addWidget(b); }
    connect(other, &QPushButton::clicked, this, &ConsoleScreen::browse);
    connect(retry, &QPushButton::clicked, this, [this] { load(path); });
    errorPage->body()->addLayout(errButtons);
    centre->addWidget(errorPage);
    grid->addWidget(centre, 0, 1);

    // ---- CPU panel
    Panel* cpuPanel = new Panel("CPU", this);
    cpuPanel->setFixedWidth(290);
    cpuLine = Theme::text("", Theme::mono(13), Theme::cyan, cpuPanel);
    regs = new RegisterView(4, cpuPanel, 56);
    cyclesLine = Theme::text("", Theme::mono(13), Theme::textMid, cpuPanel);
    cpuPanel->body()->addWidget(cpuLine);
    cpuPanel->body()->addWidget(regs, 0, Qt::AlignLeft);
    cpuPanel->body()->addWidget(cyclesLine);
    cpuPanel->body()->addStretch(1);
    cpuPanel->body()->addWidget(Theme::text("BUTTONS HELD", Theme::label(), Theme::textMid, cpuPanel));
    pad = new PadView(cpuPanel);   // lights while a key is held: the key bridge, visible
    cpuPanel->body()->addWidget(pad);
    grid->addWidget(cpuPanel, 0, 2);

    // ---- bottom row: output and the last instructions
    Panel* outPanel = new Panel("OUTPUT", this);
    outPanel->setFixedHeight(160);
    QGridLayout* outStage = new QGridLayout();
    output = new QPlainTextEdit(outPanel);
    output->setReadOnly(true);
    output->setFocusPolicy(Qt::NoFocus);
    output->setFont(Theme::mono(13));
    outStage->addWidget(output, 0, 0);
    outputEmpty = Theme::text("This program prints nothing.", Theme::mono(13), Theme::textDim, outPanel);
    outputEmpty->setAlignment(Qt::AlignCenter);
    outStage->addWidget(outputEmpty, 0, 0);
    outPanel->body()->addLayout(outStage);
    grid->addWidget(outPanel, 1, 0, 1, 2);

    Panel* tracePanel = new Panel("LAST 16 INSTRUCTIONS", this);
    tracePanel->setFixedHeight(160);
    trace = new TraceView(tracePanel);
    tracePanel->body()->addWidget(trace);
    grid->addWidget(tracePanel, 1, 2);
    grid->setColumnStretch(1, 1);
    grid->setRowStretch(0, 1);

    FooterHints* footer = new FooterHints(this);
    const QString U(QChar(0x2191)), D(QChar(0x2193)), L(QChar(0x2190)), R(QChar(0x2192)), E(QChar(0x23CE));
    footer->setHints({{{U, D, L, R}, "PAD"}, {{"Z"}, "A"}, {{"X"}, "B"}, {{E}, "START"}, {{"SHIFT"}, "SELECT"},
                      {{"SPACE"}, "RUN/PAUSE"}, {{"F5"}, "STEP"}, {{"ESC"}, "MENU"}});
    page->addWidget(footer);

    setState(State::Empty);
    refresh();
}

// ---- loading

bool ConsoleScreen::load(const QString& file)
{
    timer.stop();
    releaseAll();
    path = file;
    name = QFileInfo(file).fileName();
    header->setTitle("CONSOLE  " + QString(QChar(0x00B7)) + "  " + name.toUpper());
    carts->setCurrentRow(-1);
    for (int i = 0; i < carts->count(); i++)
        if (carts->item(i)->data(Qt::UserRole).toString() == file) carts->setCurrentRow(i);

    const AsmLoad a = assembleProgram(file);
    QVector<AsmError> errors = a.errors;
    if (a.ok && !console.loadProgram(a.bytes))
        errors = {{0, QString("Program does not fit in ROM (%1 bytes > 32 KB)").arg(a.bytes.size())}};
    if (!a.ok || !errors.isEmpty())
    {
        errorTitle->setText(errors.size() == 1 && errors[0].line == 0
                                ? QString("%1 COULD NOT BE LOADED").arg(name.toUpper())
                                : QString("%1 HAS %2 ERROR%3").arg(name.toUpper()).arg(errors.size()).arg(errors.size() == 1 ? "" : "S"));
        errorList->setErrors(errors);
        centre->setCurrentIndex(1);
        setState(State::Error);
        refresh();
        return false;
    }

    centre->setCurrentIndex(0);
    frames = 0;
    regs->clear();
    output->clear();
    console.io().clearConsoleText();
    run();
    refresh();
    return true;
}

void ConsoleScreen::browse()
{
    suspend();
    const QString start = path.isEmpty() ? QFileInfo(findProgram("dot.asm")).absolutePath() : QFileInfo(path).absolutePath();
    const QString file = QFileDialog::getOpenFileName(this, "Load a PIXEL-8 program", start, "PIXEL-8 assembly (*.asm)");
    if (!file.isEmpty())
    {
        bool known = false;
        for (int i = 0; i < carts->count(); i++) known |= carts->item(i)->data(Qt::UserRole).toString() == file;
        if (!known)
        {
            QListWidgetItem* it = new QListWidgetItem(QIcon(SpriteCache::pixmap(SpriteId::Cartridge, 2)),
                                                      QFileInfo(file).fileName() + "\nyour file", carts);
            it->setData(Qt::UserRole, file);
            it->setToolTip(file);
        }
        load(file);
    }
    setFocus();
}

// ---- running

void ConsoleScreen::run()
{
    if (state == State::Halted || state == State::Faulted) reset();
    card->hide();
    setState(State::Running);
    timer.start();
}

void ConsoleScreen::pause()
{
    timer.stop();
    releaseAll();
    if (state == State::Running) setState(State::Paused);
}

void ConsoleScreen::reset()
{
    timer.stop();
    console.reset();    // keeps the program in ROM, clears RAM, VRAM, stack and ports
    frames = 0;
    card->hide();
    output->clear();
    regs->clear();
    setState(State::Paused);
    refresh();
}

void ConsoleScreen::tick()
{
    const StepResult r = console.runFrame();
    frames++;
    refresh();
    if (r == StepResult::Halt)
    {
        timer.stop();
        setState(State::Halted);
        showCard("PROGRAM ENDED", Theme::green,
                 QString("The CPU reached HLT after %1 frame%2. The registers on the right are its final values.")
                     .arg(frames).arg(frames == 1 ? "" : "s"));
    }
    else if (r == StepResult::Fault)
    {
        timer.stop();
        setState(State::Faulted);
        const Fault& f = console.cpu().getFault();
        showCard(QString(faultName(f.type)).toUpper(), Theme::red,
                 QString("PC %1\n%2").arg(hex4(f.pc), QString::fromStdString(f.message)));
    }
}

void ConsoleScreen::runFrames(int n)
{
    if (state != State::Running && state != State::Paused) return;
    for (int i = 0; i < n && (state == State::Running || state == State::Paused); i++) tick();
}

void ConsoleScreen::showCard(const QString& title, const QColor& colour, const QString& text)
{
    cardTitle->setText(title);
    Theme::setColour(cardTitle, colour);
    cardText->setText(text);
    card->show();
}

// ---- drawing the machine state (const reads only)

void ConsoleScreen::refresh()
{
    pad->setInput(console.io().getInput());
    if (state == State::Empty || state == State::Error)   // nothing loaded: show nothing stale
    {
        screen->setFrame({});
        screen->setPower(false);
        regs->clear();
        cpuLine->setText("PC ------  SP ------  P ------");
        cyclesLine->setText("CYCLES  -\nFRAMES  -");
        frameLabel->setText("");
        output->clear();
        outputEmpty->setVisible(true);
        trace->setLines({});
        return;
    }

    std::array<Byte, 512> px{};
    for (int y = 0; y < MemoryMap::SCREEN_HEIGHT; y++)
        for (int x = 0; x < MemoryMap::SCREEN_WIDTH; x++)
            px[y * MemoryMap::SCREEN_WIDTH + x] = console.pixel(x, y);
    screen->setFrame(px);
    screen->setPower(state == State::Running || state == State::Paused);

    CPU& cpu = console.cpu();
    std::array<Byte, 8> r{};
    for (int i = 0; i < 8; i++) r[i] = cpu.getReg(i);
    regs->setRegisters(r, cpu.getFlags());
    cpuLine->setText(QString("PC %1  SP %2  P %3").arg(hex4(cpu.getPC()), hex4(cpu.getSP()), hex4(cpu.getP())));
    cyclesLine->setText(QString("CYCLES  %1\nFRAMES  %2").arg(grouped(cpu.getCycles())).arg(frames));
    frameLabel->setText(QString("FRAME %1").arg(frames, 4, 10, QChar('0')));

    const QString text = QString::fromStdString(console.io().getConsoleText());
    if (text != output->toPlainText())
    {
        output->setPlainText(text);
        output->verticalScrollBar()->setValue(output->verticalScrollBar()->maximum());
    }
    outputEmpty->setVisible(text.isEmpty());

    QStringList lines;
    const InstructionTrace& t = cpu.getTrace();
    Disassembler dis(console.bus());
    for (int i = 0; i < t.size(); i++)
    {
        Byte len = 0;
        lines << QString::fromStdString(dis.line(t.at(i).pc, len));
    }
    trace->setLines(lines);
}

void ConsoleScreen::setState(State s)
{
    state = s;
    switch (s)
    {
    case State::Running: chip->setState("RUNNING", Theme::green); break;
    case State::Paused: chip->setState("PAUSED", Theme::amber); break;
    case State::Halted: chip->setState("HALTED", Theme::textMid); break;
    case State::Faulted: chip->setState("FAULT", Theme::red); break;
    case State::Error: chip->setState("NOT LOADED", Theme::red); break;
    case State::Empty: chip->setState("NO PROGRAM", Theme::textDim, false); break;
    }
    const bool running = s == State::Running;
    runButton->setText(running ? "Pause" : "Run");
    runButton->setGlyph(running ? SpriteId::Pause : SpriteId::Play);
    runButton->setEnabled(s != State::Empty && s != State::Error);
    stepButton->setEnabled(s == State::Paused);
}

// ---- keys -> buttons (the key bridge)

bool ConsoleScreen::buttonFor(int key, int& button)
{
    switch (key)
    {
    case Qt::Key_Up: case Qt::Key_W: button = IoPorts::BTN_UP; return true;
    case Qt::Key_Down: case Qt::Key_S: button = IoPorts::BTN_DOWN; return true;
    case Qt::Key_Left: case Qt::Key_A: button = IoPorts::BTN_LEFT; return true;
    case Qt::Key_Right: case Qt::Key_D: button = IoPorts::BTN_RIGHT; return true;
    case Qt::Key_Z: case Qt::Key_J: button = IoPorts::BTN_A; return true;
    case Qt::Key_X: case Qt::Key_K: button = IoPorts::BTN_B; return true;
    case Qt::Key_Return: case Qt::Key_Enter: button = IoPorts::BTN_START; return true;
    case Qt::Key_Shift: button = IoPorts::BTN_SELECT; return true;
    default: return false;
    }
}

void ConsoleScreen::keyPressEvent(QKeyEvent* e)
{
    int button;
    if (buttonFor(e->key(), button))
    {
        console.io().setButton((IoPorts::Button)button, true);   // no setInput(0) after frames (handoff 4)
        pad->setInput(console.io().getInput());
        return;
    }
    switch (e->key())
    {
    case Qt::Key_Escape: emit back(); return;
    case Qt::Key_Space:
        if (!e->isAutoRepeat() && (state == State::Running || state == State::Paused || state == State::Halted || state == State::Faulted))
            state == State::Running ? pause() : run();
        return;
    case Qt::Key_F5: if (state == State::Paused) runFrames(1); return;
    case Qt::Key_O: if (e->modifiers() & Qt::ControlModifier) browse(); return;
    default: Screen::keyPressEvent(e);
    }
}

void ConsoleScreen::keyReleaseEvent(QKeyEvent* e)
{
    if (e->isAutoRepeat()) return;   // the OS repeats held keys; those fake releases are ignored
    int button;
    if (buttonFor(e->key(), button))
    {
        console.io().setButton((IoPorts::Button)button, false);
        pad->setInput(console.io().getInput());
    }
}

void ConsoleScreen::focusOutEvent(QFocusEvent*)
{
    releaseAll();   // Alt+Tab while holding a key must not leave it held
}

void ConsoleScreen::releaseAll()
{
    console.io().setInput(0);
    pad->setInput(0);
}

// ---- screen life cycle

void ConsoleScreen::enter()
{
    setFocus();
    if (state == State::Empty)
    {
        const QString dot = findProgram("dot.asm");
        if (!dot.isEmpty()) load(dot);
    }
}

void ConsoleScreen::leave()
{
    pause();
}

void ConsoleScreen::suspend()
{
    pause();
}

bool ConsoleScreen::hasProgressToLose() const
{
    return state == State::Running || state == State::Paused;
}
