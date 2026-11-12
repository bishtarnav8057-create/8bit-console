// CalculatorTab.h - the "Calculator" tab. Every answer is worked out by our CPU running
// CPU/programs/calc.asm (through the Calculator class). The window only shows the result.
#ifndef CALCULATORTAB_H
#define CALCULATORTAB_H

#include <QString>
#include <QWidget>

#include "calc/Calculator.h"

class QComboBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QTableWidget;

class CalculatorTab : public QWidget
{
    Q_OBJECT

public:
    explicit CalculatorTab(QWidget* parent = nullptr);

    bool loadProgram(const QString& path);      // false (with a message box) if calc.asm is bad

private slots:
    void calculate();
    void showBinary();
    void browse();

private:
    QString findCalcAsm() const;
    void showResult(char op, int a, int b, const CalcResult& r);
    void fillTrace(const CalcResult& r);

    Calculator calc;
    bool ready;

    QSpinBox* inputA;
    QSpinBox* inputB;
    QComboBox* opBox;
    QLabel* binaryA;
    QLabel* binaryB;
    QPushButton* calcButton;
    QLabel* resultLabel;
    QLabel* detailLabel;
    QLabel* warningLabel;
    QLabel* infoLabel;
    QLabel* programLabel;
    QTableWidget* trace;
};

#endif
