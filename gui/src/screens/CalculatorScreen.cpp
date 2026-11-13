#include "screens/CalculatorScreen.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QIntValidator>
#include <QWheelEvent>
#include <QStackedWidget>
#include <QTableWidget>
#include <QVBoxLayout>

#include "Theme.h"
#include "widgets/CpuViews.h"
#include "widgets/Pixel.h"

namespace
{
    QString binary8(int value)
    {
        const QString b = QString::number(value & 0xFF, 2).rightJustified(8, '0');
        return b.left(4) + " " + b.mid(4);
    }

    QString hex2(int value)
    {
        return "0x" + QString::number(value & 0xFF, 16).toUpper().rightJustified(2, '0');
    }

    // the sign shown for each operation (Press Start 2P has the Latin-1 x and divide signs)
    QString opSign(char op)
    {
        switch (op)
        {
        case '*': return QString(QChar(0x00D7));
        case '/': return QString(QChar(0x00F7));
        case '&': return "AND";
        case '|': return "OR";
        case '^': return "XOR";
        default: return QString(QChar(op));
        }
    }

    // instructions that put a new value in their first register, so it is worth showing
    bool changesFirstRegister(const QString& mnemonic)
    {
        static const char* names[] = {"LD", "LDI", "MOV", "ADD", "SUB", "AND", "OR", "XOR", "INC", "DEC"};
        for (const char* name : names)
            if (mnemonic == name) return true;
        return false;
    }

    const char OPS[] = {'+', '-', '*', '/', '&', '|', '^'};
}

// A small status badge in the display: CARRY, BORROW, TOO BIG, DIV BY 0, CPU STOPPED.
class ChipLabel : public QWidget
{
public:
    explicit ChipLabel(QWidget* parent) : QWidget(parent) { setFixedHeight(24); }
    void setChip(const QString& t, const QColor& face = QColor(), const QColor& ink = QColor())
    {
        text = t;
        this->face = face;
        this->ink = ink;
        updateGeometry();
        update();
    }
    QSize sizeHint() const override
    {
        return QSize(text.isEmpty() ? 0 : QFontMetrics(Theme::pixel(8)).horizontalAdvance(text) + 20, 24);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        if (text.isEmpty()) return;
        QPainter p(this);
        p.fillRect(rect(), face);
        p.setFont(Theme::pixel(8));
        p.setPen(ink);
        p.drawText(rect(), Qt::AlignCenter, text);
    }

private:
    QString text;
    QColor face, ink;
};

CalculatorScreen::CalculatorScreen(QWidget* parent) : Screen(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    QVBoxLayout* page = new QVBoxLayout(this);
    page->setContentsMargins(0, 0, 0, 0);
    page->setSpacing(0);

    ScreenHeader* header = new ScreenHeader("CALCULATOR", this);
    connect(header, &ScreenHeader::back, this, &Screen::back);
    programChip = new StatusChip(header);
    header->right()->addWidget(programChip);
    page->addWidget(header);

    body = new QStackedWidget(this);
    page->addWidget(body, 1);

    // ================================================================ workspace
    QWidget* work = new QWidget(body);
    QHBoxLayout* cols = new QHBoxLayout(work);
    cols->setContentsMargins(24, 20, 24, 20);
    cols->setSpacing(16);

    // ---- left: the calculator
    Panel* left = new Panel("CALCULATOR", work);
    QVBoxLayout* l = left->body();
    l->setSpacing(10);

    Panel* display = new Panel(QString(), left);
    display->setColours(Theme::screenOff, Theme::greenDeep);
    display1 = Theme::text("", Theme::pixel(24), Theme::green, display);
    display1->setTextFormat(Qt::RichText);
    display1->setMinimumHeight(36);
    display2 = Theme::text("", Theme::mono(14), Theme::cyan, display);
    statusChip = new ChipLabel(display);
    QHBoxLayout* d2 = new QHBoxLayout();
    d2->addWidget(display2, 1);
    d2->addWidget(statusChip);
    display->body()->addWidget(display1);
    display->body()->addLayout(d2);
    l->addWidget(display);

    auto byteRow = [&](const QString& name, QLineEdit*& spin, BitRow*& bits, QLabel*& hex) {
        QHBoxLayout* row = new QHBoxLayout();
        row->setSpacing(12);
        QLabel* n = Theme::text(name, Theme::pixel(16), Theme::textMid, left);
        n->setFixedWidth(20);
        row->addWidget(n);
        // a line edit, not a QSpinBox: a styled spin box drew a 1 px wide text field with this Qt version
        spin = new QLineEdit(left);
        spin->setValidator(new QIntValidator(0, 255, spin));
        spin->setFont(Theme::pixel(16));
        spin->setFixedSize(112, 44);
        spin->setToolTip("0 to 255. Up/Down or the mouse wheel change it by 1.");
        spin->installEventFilter(this);
        row->addWidget(spin);
        bits = new BitRow(left);
        bits->setEditable(true);
        row->addWidget(bits);
        hex = Theme::text("0x00", Theme::mono(14), Theme::cyan, left);
        row->addWidget(hex);
        row->addStretch(1);
        l->addLayout(row);
        QLineEdit* s = spin; BitRow* b = bits; QLabel* h = hex;
        connect(s, &QLineEdit::textChanged, this, [b, h](const QString& t) { b->setValue(t.toInt()); h->setText(hex2(t.toInt())); });
        connect(b, &BitRow::valueChanged, s, [s](int v) { s->setText(QString::number(v)); });
    };

    byteRow("A", inputA, bitsA, hexA);

    QHBoxLayout* keypad = new QHBoxLayout();
    keypad->setSpacing(0);
    keypad->addSpacing(28);
    ops = new QButtonGroup(this);
    ops->setExclusive(true);
    for (char op : OPS)
    {
        const QString label = op == '-' ? QString(QChar(0x2212)) : opSign(op);
        PixelButton* b = new PixelButton(label, PixelButton::Secondary, left);
        b->setCheckable(true);
        b->setFocusPolicy(Qt::NoFocus);
        b->setToolTip(QString("Key: %1").arg(QChar(op)));
        ops->addButton(b, op);
        keypad->addWidget(b);
    }
    ops->button('+')->setChecked(true);
    keypad->addStretch(1);
    l->addLayout(keypad);

    byteRow("B", inputB, bitsB, hexB);

    runButton = new PixelButton("Run on CPU", PixelButton::Primary, left);
    runButton->setTrailingGlyph(SpriteId::Enter);
    runButton->setFocusPolicy(Qt::NoFocus);
    runButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(runButton, &QPushButton::clicked, this, &CalculatorScreen::calculate);
    l->addWidget(runButton);

    why = new Panel("WHY?", left);
    why->setTitleAccent(Theme::red);
    whyText = Theme::text("", Theme::mono(12), Theme::textHi, why);
    whyText->setWordWrap(true);
    QHBoxLayout* whyRow = new QHBoxLayout();
    whyBits = new BitRow(why);
    whyBits->setNinth(true);
    whyBitsNote = Theme::text(QString(QChar(0x2190)) + " 9th bit doesn't fit: dropped", Theme::mono(12), Theme::red, why);
    whyRow->addWidget(whyBits);
    whyRow->addWidget(whyBitsNote);
    whyRow->addStretch(1);
    why->body()->addLayout(whyRow);
    why->body()->addWidget(whyText);
    why->hide();
    l->addWidget(why);
    l->addStretch(1);
    cols->addWidget(left, 1);

    // ---- right: inside the CPU
    Panel* right = new Panel("INSIDE THE CPU", work);
    QVBoxLayout* r = right->body();
    regs = new RegisterView(4, right);
    r->addWidget(regs, 0, Qt::AlignLeft);
    info = Theme::text("Run a sum to see what the CPU did.", Theme::mono(13), Theme::textMid, right);
    r->addWidget(info);
    r->addSpacing(4);
    r->addWidget(Theme::text("TRACE", Theme::label(), Theme::textMid, right));
    QLabel* caption = Theme::text("Lines of calc.asm that ran, in source order, with values after each line's "
                                  "last run. Click a row for its registers.",
                                  Theme::mono(12), Theme::textDim, right);
    caption->setWordWrap(true);
    r->addWidget(caption);

    trace = new QTableWidget(0, 5, right);
    trace->setHorizontalHeaderLabels({"LINE", "INSTRUCTION", "RUNS", "RESULT", "NOTE"});
    trace->horizontalHeader()->setFont(Theme::label());
    trace->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    trace->setEditTriggers(QAbstractItemView::NoEditTriggers);
    trace->setSelectionMode(QAbstractItemView::SingleSelection);
    trace->setSelectionBehavior(QAbstractItemView::SelectRows);
    trace->setFocusPolicy(Qt::NoFocus);
    trace->setAlternatingRowColors(true);
    trace->setShowGrid(false);
    trace->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    trace->setTextElideMode(Qt::ElideRight);
    trace->verticalHeader()->hide();
    trace->verticalHeader()->setDefaultSectionSize(28);
    trace->horizontalHeader()->setStretchLastSection(true);
    trace->setColumnWidth(0, 56);
    trace->setColumnWidth(1, 160);
    trace->setColumnWidth(2, 56);
    trace->setColumnWidth(3, 92);
    connect(trace, &QTableWidget::currentCellChanged, this, [this](int row) { showRow(row); });
    r->addWidget(trace, 1);

    replayButton = new PixelButton("Replay", PixelButton::Secondary, right);
    replayButton->setGlyph(SpriteId::Play);
    replayButton->setHeightKind(PixelButton::Small);
    replayButton->setFocusPolicy(Qt::NoFocus);
    replayButton->setEnabled(false);
    connect(replayButton, &QPushButton::clicked, this, [this] { replay.isActive() ? stopReplay() : startReplay(); });
    r->addWidget(replayButton, 0, Qt::AlignLeft);
    cols->addWidget(right, 1);
    body->addWidget(work);

    // ================================================================ problem page (missing or broken calc.asm)
    Panel* problem = new Panel(QString(), body);
    QVBoxLayout* pb = problem->body();
    pb->setContentsMargins(48, 48, 48, 48);
    pb->setSpacing(16);
    pb->addWidget(new SpriteView(SpriteId::ChipQuestion, 6, problem), 0, Qt::AlignLeft);
    problemTitle = Theme::text("", Theme::pixel(16), Theme::red, problem);
    problemText = Theme::text("", Theme::mono(14), Theme::textMid, problem);
    problemText->setWordWrap(true);
    problemText->setTextInteractionFlags(Qt::TextSelectableByMouse);
    problemErrors = new ErrorList(problem);
    pb->addWidget(problemTitle);
    pb->addWidget(problemText);
    pb->addWidget(problemErrors);
    QHBoxLayout* pbButtons = new QHBoxLayout();
    PixelButton* locate = new PixelButton("Locate calc.asm...", PixelButton::Primary, problem);
    retryButton = new PixelButton("Retry", PixelButton::Secondary, problem);
    retryButton->setGlyph(SpriteId::Reset);
    connect(locate, &QPushButton::clicked, this, &CalculatorScreen::browse);
    connect(retryButton, &QPushButton::clicked, this, [this] { loadProgram(programPath); });
    pbButtons->addWidget(locate);
    pbButtons->addWidget(retryButton);
    pbButtons->addStretch(1);
    pb->addLayout(pbButtons);
    pb->addStretch(1);
    body->addWidget(problem);

    // ================================================================ footer
    FooterHints* footer = new FooterHints(this);
    footer->setHints({{{"0-9"}, "TYPE"}, {{"TAB"}, "A / B"}, {{"+", "-", "*", "/", "&", "|", "^"}, "OPERATION"},
                      {{QString(QChar(0x23CE))}, "RUN"}, {{"ESC"}, "MENU"}});
    page->addWidget(footer);

    replay.setInterval(250);
    connect(&replay, &QTimer::timeout, this, [this] {
        const int next = trace->currentRow() + 1;
        if (next >= trace->rowCount()) stopReplay();
        else trace->setCurrentCell(next, 0);
    });
    cursorBlink.setInterval(500);
    connect(&cursorBlink, &QTimer::timeout, this, [this] {
        cursorOn = !cursorOn;
        if (idle) display1->setText(cursorOn ? "READY_" : "READY");
    });

    inputA->setText("200");
    inputB->setText("100");
    bitsA->setValue(200);
    bitsB->setValue(100);
    hexA->setText(hex2(200));
    hexB->setText(hex2(100));
    display1->setText("READY_");
    display2->setText("Pick two numbers and press RUN.");

    const QString path = findProgram("calc.asm");
    if (path.isEmpty())
        showProblem("CALC.ASM NOT FOUND",
                    "The calculator needs CPU/programs/calc.asm. It was looked for next to the program and in "
                    "the folders above it. Use LOCATE to pick the file.",
                    {});
    else loadProgram(path);
}

// ---- the program

bool CalculatorScreen::loadProgram(const QString& path)
{
    ready = false;
    programPath = path;
    if (path.isEmpty() || !calc.load(path.toLocal8Bit().constData()))
    {
        QVector<AsmError> errors;
        for (const std::string& e : calc.errors()) errors.append(parseErrorText(QString::fromStdString(e)));
        showProblem(QString("CALC.ASM HAS %1 ERROR%2").arg(errors.size()).arg(errors.size() == 1 ? "" : "S"),
                    path + "\nFix it in an editor and press RETRY, or LOCATE another copy.", errors);
        return false;
    }
    ready = true;
    programChip->setState(QString("%1  %2  %3 B").arg(QFileInfo(path).fileName()).arg(QChar(0x00B7)).arg(calc.programSize()),
                          Theme::green);
    programChip->setToolTip(path);
    body->setCurrentIndex(0);
    return true;
}

void CalculatorScreen::showProblem(const QString& title, const QString& text, const QVector<AsmError>& errors)
{
    programChip->setState("NO PROGRAM", Theme::red);
    problemTitle->setText(title);
    problemText->setText(text);
    problemErrors->setErrors(errors);
    problemErrors->setVisible(!errors.isEmpty());
    retryButton->setVisible(!programPath.isEmpty());
    body->setCurrentIndex(1);
}

void CalculatorScreen::browse()
{
    const QString path = QFileDialog::getOpenFileName(this, "Locate calc.asm", QString(), "PIXEL-8 assembly (*.asm)");
    if (!path.isEmpty()) loadProgram(path);
}

// ---- running a sum

char CalculatorScreen::currentOp() const
{
    return (char)ops->checkedId();
}

void CalculatorScreen::selectOp(char op)
{
    if (QAbstractButton* b = ops->button(op)) b->setChecked(true);
}

void CalculatorScreen::setSum(int a, char op, int b)
{
    inputA->setText(QString::number(a));
    inputB->setText(QString::number(b));
    selectOp(op);
    calculate();
}

void CalculatorScreen::calculate()
{
    if (!ready) return;
    stopReplay();
    const char op = currentOp();
    const int a = inputA->text().toInt(), b = inputB->text().toInt();

    last = calc.run(op, (Byte)a, (Byte)b);
    idle = false;
    cursorBlink.stop();
    showResult(op, a, b, last);
    fillTrace(last);
}

void CalculatorScreen::showResult(char op, int a, int b, const CalcResult& r)
{
    why->hide();
    whyBits->hide();
    whyBitsNote->hide();
    Theme::setColour(display1, Theme::green);

    if (!r.ok)
    {
        Theme::setColour(display1, Theme::red);
        display1->setText("ERROR");
        display2->setText(QString::fromStdString(r.error));
        statusChip->setChip("CPU STOPPED", Theme::red, Theme::redInk);
        info->setText("The CPU stopped before the answer was ready.");
        return;
    }

    const QString sign = opSign(op);
    const int answer = r.answer;
    if (r.divideByZero)
    {
        display1->setText(QString("%1 %2 %3 = %4").arg(a).arg(sign).arg(b).arg(QChar(0x2014)));
        display2->setText("No answer: nothing can be divided by 0.");
        statusChip->setChip("DIV BY 0", Theme::red, Theme::redInk);
        why->setTitle("WHY NO ANSWER?");
        whyText->setText("The program checked for 0 before dividing and stopped safely. The trace on the right "
                         "shows the check running.");
        why->show();
    }
    else
    {
        QString text = QString("%1 %2 %3 = %4").arg(a).arg(sign).arg(b).arg(answer);
        if (op == '/') text += QString(" <span style='color:%1'>R %2</span>").arg(Theme::cyan.name()).arg((int)r.remainder);
        display1->setText(text);
        display2->setText(QString("%1   %2").arg(hex2(answer), binary8(answer)));
        statusChip->setChip("");

        if (r.overflow && op == '+')
        {
            statusChip->setChip("CARRY", Theme::amber, Theme::amberInk);
            why->setTitle(QString("WHY %1?").arg(answer));
            whyBits->setValue(a + b);
            whyBits->show();
            whyBitsNote->show();
            whyText->setText(QString("%1 + %2 = %3 needs 9 bits and a byte has 8: the 9th bit (256) is dropped, "
                                     "%3 - 256 = %4. The C flag records the carry.")
                                 .arg(a).arg(b).arg(a + b).arg(answer));
            why->show();
        }
        else if (r.overflow && op == '-')
        {
            statusChip->setChip("BORROW", Theme::amber, Theme::amberInk);
            why->setTitle(QString("WHY %1?").arg(answer));
            whyText->setText(QString("%1 - %2 = %3 is below 0 and a byte cannot be negative, so it wraps "
                                     "around: %3 + 256 = %4. The C flag records the borrow.")
                                 .arg(a).arg(b).arg(a - b).arg(answer));
            why->show();
        }
        else if (r.overflow && op == '*')
        {
            statusChip->setChip("TOO BIG", Theme::amber, Theme::amberInk);
            why->setTitle(QString("WHY %1?").arg(answer));
            whyText->setText(QString("%1 x %2 = %3, which does not fit in 8 bits (0-255). The CPU keeps only "
                                     "the low 8 bits: %3 mod 256 = %4.")
                                 .arg(a).arg(b).arg(a * b).arg(answer));
            why->show();
        }
    }
    info->setText(QString("The CPU ran %1 instructions.").arg(r.instructions));
}

void CalculatorScreen::fillTrace(const CalcResult& r)
{
    trace->blockSignals(true);
    trace->setRowCount(0);
    trace->blockSignals(false);
    replayButton->setEnabled(r.ok && !r.trace.empty());
    if (!r.ok) { regs->clear(); return; }

    int haltRow = -1;
    for (const TraceLine& t : r.trace)
    {
        QString text = QString::fromStdString(calc.sourceLine(t.line));
        QString comment;
        const int semicolon = text.indexOf(';');
        if (semicolon >= 0)
        {
            comment = text.mid(semicolon + 1).trimmed();
            text = text.left(semicolon);
        }
        const QString code = text.trimmed().simplified();

        // e.g. "ADD R0, R1" -> "R0 = 44". The register is the first operand, after the mnemonic
        // (searching the whole line for 'R' found the R of OR and XOR instead).
        QString value;
        const QString mnemonic = code.section(' ', 0, 0);
        const QString operand = code.section(' ', 1).trimmed();
        if (changesFirstRegister(mnemonic) && operand.size() > 1 && operand.at(0) == 'R')
        {
            const int index = operand.at(1).digitValue();
            if (index >= 0 && index < CPU::NUM_REGS)
                value = QString("R%1 = %2").arg(index).arg((int)t.registersAfter[index]);
        }

        const int row = trace->rowCount();
        trace->insertRow(row);
        auto item = [&](int col, const QString& s, const QColor& c, const QFont& f) {
            QTableWidgetItem* it = new QTableWidgetItem(s);
            it->setForeground(c);
            it->setFont(f);
            trace->setItem(row, col, it);
        };
        item(0, QString::number(t.line), Theme::textDim, Theme::mono(13));
        item(1, code, Theme::textHi, Theme::mono(13));
        item(2, QString("%1%2").arg(QChar(0x00D7)).arg(t.count), Theme::textMid, Theme::mono(13));
        item(3, value, Theme::green, Theme::mono(13));
        item(4, comment, Theme::textMid, Theme::mono(12));
        if (mnemonic == "HLT") haltRow = row;
    }
    // the HLT row holds the final registers (each routine of calc.asm ends at its own HLT)
    trace->setCurrentCell(haltRow >= 0 ? haltRow : trace->rowCount() - 1, 0);
}

void CalculatorScreen::showRow(int row)
{
    if (row < 0 || row >= (int)last.trace.size()) return;
    const std::array<Byte, 8> zero{};
    const std::array<Byte, 8>& before = row > 0 ? last.trace[row - 1].registersAfter : zero;
    regs->setRegisters(last.trace[row].registersAfter, last.flags, before);
}

void CalculatorScreen::startReplay()
{
    if (trace->rowCount() == 0) return;
    trace->setCurrentCell(0, 0);
    replayButton->setText("Stop");
    replayButton->setGlyph(SpriteId::Pause);
    replay.start();
}

void CalculatorScreen::stopReplay()
{
    replay.stop();
    replayButton->setText("Replay");
    replayButton->setGlyph(SpriteId::Play);
}

// ---- keys

bool CalculatorScreen::handleKey(QKeyEvent* e)
{
    if (replay.isActive()) stopReplay();   // any key stops the replay
    switch (e->key())
    {
    case Qt::Key_Escape: emit back(); return true;
    case Qt::Key_Return: case Qt::Key_Enter: calculate(); return true;
    default: break;
    }
    const QString t = e->text();
    if (t.size() == 1)
        for (char op : OPS)
            if (t.at(0) == QChar(op)) { selectOp(op); return true; }
    return false;
}

void CalculatorScreen::stepValue(QLineEdit* e, int delta)
{
    e->setText(QString::number(std::clamp(e->text().toInt() + delta, 0, 255)));
}

bool CalculatorScreen::eventFilter(QObject* o, QEvent* e)
{
    QLineEdit* edit = o == inputA ? inputA : o == inputB ? inputB : nullptr;
    if (edit && e->type() == QEvent::KeyPress)
    {
        QKeyEvent* k = static_cast<QKeyEvent*>(e);
        if (k->key() == Qt::Key_Up || k->key() == Qt::Key_Down) { stepValue(edit, k->key() == Qt::Key_Up ? 1 : -1); return true; }
        return handleKey(k);
    }
    if (edit && e->type() == QEvent::Wheel)
    {
        stepValue(edit, static_cast<QWheelEvent*>(e)->angleDelta().y() > 0 ? 1 : -1);
        return true;
    }
    return Screen::eventFilter(o, e);
}

void CalculatorScreen::keyPressEvent(QKeyEvent* e)
{
    if (!handleKey(e)) Screen::keyPressEvent(e);
}

void CalculatorScreen::enter()
{
    if (idle) cursorBlink.start();
    if (body->currentIndex() == 0)
    {
        inputA->setFocus();
        inputA->selectAll();
    }
    else setFocus();
}

void CalculatorScreen::leave()
{
    stopReplay();
    cursorBlink.stop();
}
