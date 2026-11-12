#include "CalculatorTab.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

namespace
{
    QString binary8(int value)
    {
        return QString::number(value, 2).rightJustified(8, '0');
    }

    // instructions that put a new value in their first register, so it is worth showing
    bool changesFirstRegister(const QString& mnemonic)
    {
        static const char* names[] = {"LD", "LDI", "MOV", "ADD", "SUB", "AND", "OR", "XOR", "INC", "DEC"};
        for (const char* name : names)
        {
            if (mnemonic == name) return true;
        }
        return false;
    }
}

CalculatorTab::CalculatorTab(QWidget* parent)
    : QWidget(parent), ready(false)
{
    // ---- the two numbers and the operation
    inputA = new QSpinBox(this);
    inputB = new QSpinBox(this);
    inputA->setRange(0, 255);
    inputB->setRange(0, 255);

    QFont big = inputA->font();
    big.setPointSize(18);
    inputA->setFont(big);
    inputB->setFont(big);

    opBox = new QComboBox(this);
    opBox->setFont(big);
    opBox->addItem("+   add", QVariant('+'));
    opBox->addItem("-   subtract", QVariant('-'));
    opBox->addItem("*   multiply", QVariant('*'));
    opBox->addItem("/   divide", QVariant('/'));
    opBox->addItem("&   AND", QVariant('&'));
    opBox->addItem("|   OR", QVariant('|'));
    opBox->addItem("^   XOR", QVariant('^'));

    binaryA = new QLabel(this);
    binaryB = new QLabel(this);
    QFont mono("Consolas");
    mono.setStyleHint(QFont::Monospace);
    binaryA->setFont(mono);
    binaryB->setFont(mono);

    calcButton = new QPushButton("Calculate", this);
    calcButton->setDefault(true);
    calcButton->setMinimumHeight(40);

    QGroupBox* inputBox = new QGroupBox("Enter a sum (8-bit numbers, 0 to 255)", this);
    QGridLayout* grid = new QGridLayout(inputBox);
    grid->addWidget(new QLabel("First number"), 0, 0);
    grid->addWidget(new QLabel("Operation"), 0, 1);
    grid->addWidget(new QLabel("Second number"), 0, 2);
    grid->addWidget(inputA, 1, 0);
    grid->addWidget(opBox, 1, 1);
    grid->addWidget(inputB, 1, 2);
    grid->addWidget(binaryA, 2, 0);
    grid->addWidget(binaryB, 2, 2);
    grid->addWidget(calcButton, 1, 3);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 1);

    // ---- the answer
    resultLabel = new QLabel("Press Calculate", this);
    QFont resultFont = resultLabel->font();
    resultFont.setPointSize(26);
    resultFont.setBold(true);
    resultLabel->setFont(resultFont);
    resultLabel->setAlignment(Qt::AlignCenter);

    detailLabel = new QLabel(this);
    detailLabel->setAlignment(Qt::AlignCenter);
    detailLabel->setFont(mono);

    warningLabel = new QLabel(this);
    warningLabel->setAlignment(Qt::AlignCenter);
    warningLabel->setWordWrap(true);
    warningLabel->setStyleSheet("color: #b00020; font-weight: bold;");

    infoLabel = new QLabel(this);
    infoLabel->setAlignment(Qt::AlignCenter);

    QGroupBox* answerBox = new QGroupBox("Answer", this);
    QVBoxLayout* answerLayout = new QVBoxLayout(answerBox);
    answerLayout->addWidget(resultLabel);
    answerLayout->addWidget(detailLabel);
    answerLayout->addWidget(warningLabel);
    answerLayout->addWidget(infoLabel);

    // ---- what the CPU did
    trace = new QTableWidget(0, 5, this);
    QStringList heads;
    heads << "Line" << "calc.asm instruction" << "Runs" << "Register after" << "What it does";
    trace->setHorizontalHeaderLabels(heads);
    trace->setEditTriggers(QAbstractItemView::NoEditTriggers);
    trace->setSelectionMode(QAbstractItemView::NoSelection);
    trace->setAlternatingRowColors(true);
    trace->verticalHeader()->hide();
    trace->horizontalHeader()->setStretchLastSection(true);
    trace->setColumnWidth(0, 50);
    trace->setColumnWidth(1, 180);
    trace->setColumnWidth(2, 50);
    trace->setColumnWidth(3, 110);
    trace->setFont(mono);

    QGroupBox* traceBox = new QGroupBox("What the CPU did (the lines of calc.asm that ran)", this);
    QVBoxLayout* traceLayout = new QVBoxLayout(traceBox);
    traceLayout->addWidget(trace);

    // ---- which program is loaded
    programLabel = new QLabel(this);
    QPushButton* browseButton = new QPushButton("Choose calc.asm...", this);
    QHBoxLayout* programRow = new QHBoxLayout();
    programRow->addWidget(programLabel, 1);
    programRow->addWidget(browseButton);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(inputBox);
    layout->addWidget(answerBox);
    layout->addWidget(traceBox, 1);
    layout->addLayout(programRow);

    connect(calcButton, SIGNAL(clicked()), this, SLOT(calculate()));
    connect(inputA, SIGNAL(valueChanged(int)), this, SLOT(showBinary()));
    connect(inputB, SIGNAL(valueChanged(int)), this, SLOT(showBinary()));
    connect(browseButton, SIGNAL(clicked()), this, SLOT(browse()));

    showBinary();

    QString path = findCalcAsm();
    if (path.isEmpty()) programLabel->setText("calc.asm was not found. Use \"Choose calc.asm...\".");
    else loadProgram(path);
    calcButton->setEnabled(ready);
}

QString CalculatorTab::findCalcAsm() const
{
    QStringList places;
#ifdef PIXEL8_CPU_DIR
    places << QString(PIXEL8_CPU_DIR) + "/programs/calc.asm";    // where it was at build time
#endif
    QString app = QCoreApplication::applicationDirPath();
    places << app + "/CPU/programs/calc.asm"
           << app + "/../CPU/programs/calc.asm"
           << app + "/../../CPU/programs/calc.asm"
           << app + "/../../../CPU/programs/calc.asm"
           << "CPU/programs/calc.asm"
           << "../CPU/programs/calc.asm";

    for (const QString& place : places)
    {
        if (QFileInfo::exists(place)) return QFileInfo(place).absoluteFilePath();
    }
    return QString();
}

bool CalculatorTab::loadProgram(const QString& path)
{
    ready = false;
    if (!calc.load(path.toLocal8Bit().constData()))
    {
        QString text;
        for (const std::string& e : calc.errors()) text += QString::fromStdString(e) + "\n";
        QMessageBox::warning(this, "Could not load the program", text);
        programLabel->setText("No program loaded.");
    }
    else
    {
        ready = true;
        programLabel->setText(QString("Program: %1  (%2 bytes of machine code)").arg(path).arg(calc.programSize()));
    }
    calcButton->setEnabled(ready);
    return ready;
}

void CalculatorTab::browse()
{
    QString path = QFileDialog::getOpenFileName(this, "Choose calc.asm", QString(), "Assembly (*.asm)");
    if (!path.isEmpty()) loadProgram(path);
}

void CalculatorTab::showBinary()
{
    binaryA->setText("binary  " + binary8(inputA->value()));
    binaryB->setText("binary  " + binary8(inputB->value()));
}

void CalculatorTab::calculate()
{
    if (!ready) return;

    char op = (char)opBox->currentData().toInt();
    int a = inputA->value();
    int b = inputB->value();

    CalcResult r = calc.run(op, (Byte)a, (Byte)b);
    showResult(op, a, b, r);
    fillTrace(r);
}

void CalculatorTab::showResult(char op, int a, int b, const CalcResult& r)
{
    warningLabel->setText("");

    if (!r.ok)
    {
        resultLabel->setText("Error");
        detailLabel->setText("");
        infoLabel->setText("");
        warningLabel->setText(QString::fromStdString(r.error));
        return;
    }

    QString opText = QString(QChar(op));
    int answer = r.answer;

    if (r.divideByZero)
    {
        resultLabel->setText(QString("%1 / %2 = ?").arg(a).arg(b));
        detailLabel->setText("");
        warningLabel->setText("Cannot divide by zero. The program checked for it and stopped safely.");
    }
    else
    {
        QString text = QString("%1 %2 %3 = %4").arg(a).arg(opText).arg(b).arg(answer);
        if (op == '/') text += QString("  remainder %1").arg((int)r.remainder);
        resultLabel->setText(text);
        QString hex = QString::number(answer, 16).toUpper().rightJustified(2, '0');
        detailLabel->setText(QString("hex 0x%1      binary %2").arg(hex).arg(binary8(answer)));
    }

    if (r.overflow && op == '+')
        warningLabel->setText(QString("CARRY: %1 does not fit in 8 bits (0-255), so it wrapped around: %1 - 256 = %2")
                              .arg(a + b).arg(answer));
    else if (r.overflow && op == '-')
        warningLabel->setText(QString("BORROW: %1 is below 0, so it wrapped around: %1 + 256 = %2")
                              .arg(a - b).arg(answer));
    else if (r.overflow && op == '*')
        warningLabel->setText(QString("TOO BIG: %1 does not fit in 8 bits (0-255). The CPU keeps only the low 8 bits: %2")
                              .arg(a * b).arg(answer));

    QString flags;
    if (r.flags & (1 << CPU::FLAG_Z)) flags += " Z";
    if (r.flags & (1 << CPU::FLAG_C)) flags += " C";
    if (r.flags & (1 << CPU::FLAG_N)) flags += " N";
    if (r.flags & (1 << CPU::FLAG_V)) flags += " V";
    if (flags.isEmpty()) flags = " none";
    infoLabel->setText(QString("The CPU ran %1 instructions.   Flags set:%2").arg(r.instructions).arg(flags));
}

void CalculatorTab::fillTrace(const CalcResult& r)
{
    trace->setRowCount(0);
    if (!r.ok) return;

    for (const TraceLine& t : r.trace)
    {
        QString text = QString::fromStdString(calc.sourceLine(t.line));
        QString comment;
        int semicolon = text.indexOf(';');
        if (semicolon >= 0)
        {
            comment = text.mid(semicolon + 1).trimmed();
            text = text.left(semicolon);
        }
        QString code = text.trimmed();

        // e.g. "ADD R0, R1" -> show "R0 = 42"
        QString value;
        QString mnemonic = code.section(' ', 0, 0);
        int r0 = code.indexOf('R');
        if (changesFirstRegister(mnemonic) && r0 >= 0 && r0 + 1 < code.size())
        {
            int index = code.at(r0 + 1).digitValue();
            if (index >= 0 && index < CPU::NUM_REGS)
                value = QString("R%1 = %2").arg(index).arg((int)t.registersAfter[index]);
        }

        int row = trace->rowCount();
        trace->insertRow(row);
        trace->setItem(row, 0, new QTableWidgetItem(QString::number(t.line)));
        trace->setItem(row, 1, new QTableWidgetItem(code));
        trace->setItem(row, 2, new QTableWidgetItem(QString("x%1").arg(t.count)));
        trace->setItem(row, 3, new QTableWidgetItem(value));
        trace->setItem(row, 4, new QTableWidgetItem(comment));
    }
}
