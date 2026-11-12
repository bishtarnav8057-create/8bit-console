#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class CalculatorTab;
class GameTab;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

private slots:
    void about();

private:
    CalculatorTab* calculatorTab;
    GameTab* gameTab;
};

#endif
