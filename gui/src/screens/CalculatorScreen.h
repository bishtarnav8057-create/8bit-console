// CalculatorScreen.h - "a calculator with its case opened" (plan 8.1). Left: the calculator. Right: inside
// the CPU (registers, flags, the lines of calc.asm that ran). Every answer is worked out by our CPU
// running CPU/programs/calc.asm through the Calculator class; the window only shows the result.
#ifndef CALCULATORSCREEN_H
#define CALCULATORSCREEN_H

#include <QString>
#include <QTimer>

#include "Programs.h"
#include "calc/Calculator.h"
#include "screens/Screen.h"

class BitRow;
class ErrorList;
class Panel;
class PixelButton;
class QButtonGroup;
class QLabel;
class QLineEdit;
class QStackedWidget;
class QTableWidget;
class RegisterView;
class StatusChip;
class ChipLabel;

class CalculatorScreen : public Screen
{
    Q_OBJECT

public:
    explicit CalculatorScreen(QWidget* parent = nullptr);
    void enter() override;
    void leave() override;

    bool loadProgram(const QString& path);
    void setSum(int a, char op, int b);         // for --shots and tests: fill in and run

protected:
    void keyPressEvent(QKeyEvent* e) override;
    bool eventFilter(QObject* o, QEvent* e) override;

private:
    void calculate();
    void browse();
    void selectOp(char op);
    void showResult(char op, int a, int b, const CalcResult& r);
    void fillTrace(const CalcResult& r);
    void showRow(int row);
    void startReplay();
    void stopReplay();
    void showProblem(const QString& title, const QString& text, const QVector<AsmError>& errors);
    bool handleKey(QKeyEvent* e);
    char currentOp() const;
    static void stepValue(QLineEdit* e, int delta);

    Calculator calc;
    bool ready = false;
    QString programPath;
    CalcResult last;

    StatusChip* programChip;
    QStackedWidget* body;
    QLabel* display1;
    QLabel* display2;
    ChipLabel* statusChip;
    QLineEdit* inputA;
    QLineEdit* inputB;
    BitRow* bitsA;
    BitRow* bitsB;
    QLabel* hexA;
    QLabel* hexB;
    QButtonGroup* ops;
    PixelButton* runButton;
    Panel* why;
    QLabel* whyText;
    BitRow* whyBits;
    QLabel* whyBitsNote;
    RegisterView* regs;
    QLabel* info;
    QTableWidget* trace;
    PixelButton* replayButton;
    QTimer replay, cursorBlink;
    bool cursorOn = true;
    bool idle = true;

    QLabel* problemTitle;
    QLabel* problemText;
    ErrorList* problemErrors;
    PixelButton* retryButton;
};

#endif
