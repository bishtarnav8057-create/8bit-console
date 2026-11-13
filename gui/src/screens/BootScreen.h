// BootScreen.h - start-up (plan 10.1). A real self-test on the Console: memory checks, sum.asm must
// leave 55 in RAM, calc.asm must assemble. Lines appear one by one; any key skips. Never blocks.
#ifndef BOOTSCREEN_H
#define BOOTSCREEN_H

#include <QStringList>
#include <QTimer>
#include <QVector>

#include "screens/Screen.h"

class BootScreen : public Screen
{
    Q_OBJECT
public:
    explicit BootScreen(QWidget* parent = nullptr);
    void enter() override;
    void leave() override;
    void showAll();                 // for --shots

signals:
    void finished();

protected:
    void paintEvent(QPaintEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void mousePressEvent(QMouseEvent*) override;

private:
    struct Check { QString name, detail; bool ok; QString why; };
    void runChecks();
    void next();

    QVector<Check> checks;
    int shown = 0;
    bool allOk = true;
    bool cursorOn = true;
    QTimer reveal, blink, done;
};

#endif
