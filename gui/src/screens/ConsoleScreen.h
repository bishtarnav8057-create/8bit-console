// ConsoleScreen.h - the emulator itself (plan 8.2): load an .asm, run it frame by frame on the
// Console, draw its 32 x 16 VRAM, pass the keys through IoPorts and show the CPU live.
// Follows UI_HANDOFF.md sections 4 to 6.
#ifndef CONSOLESCREEN_H
#define CONSOLESCREEN_H

#include <QTimer>

#include "Aashish/src/Console.h"
#include "screens/Screen.h"

class ErrorList;
class FooterHints;
class Panel;
class PixelButton;
class QLabel;
class QListWidget;
class QPlainTextEdit;
class QStackedWidget;
class RegisterView;
class ScreenHeader;
class StatusChip;
class VramView;
class PadView;
class TraceView;

class ConsoleScreen : public Screen
{
    Q_OBJECT
public:
    explicit ConsoleScreen(QWidget* parent = nullptr);
    void enter() override;
    void leave() override;
    void suspend() override;
    bool hasProgressToLose() const override;

    bool load(const QString& path);
    void runFrames(int n);                 // for --shots: n frames without the timer

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;
    void focusOutEvent(QFocusEvent* e) override;

private:
    enum class State { Empty, Running, Paused, Halted, Faulted, Error };

    void tick();
    void run();
    void pause();
    void reset();
    void browse();
    void setState(State s);
    void refresh();
    void releaseAll();
    void showCard(const QString& title, const QColor& colour, const QString& text);
    static bool buttonFor(int key, int& button);

    Console console;
    State state = State::Empty;
    QString name, path;
    int frames = 0;
    QTimer timer;

    ScreenHeader* header;
    StatusChip* chip;
    QLabel* frameLabel;
    QListWidget* carts;
    QStackedWidget* centre;
    VramView* screen;
    Panel* card;
    QLabel* cardTitle;
    QLabel* cardText;
    PixelButton* runButton;
    PixelButton* stepButton;
    PadView* pad;
    QLabel* errorTitle;
    ErrorList* errorList;
    QLabel* cpuLine;
    RegisterView* regs;
    QLabel* cyclesLine;
    QPlainTextEdit* output;
    QLabel* outputEmpty;
    TraceView* trace;
};

#endif
