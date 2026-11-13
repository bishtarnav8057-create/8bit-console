// ConsoleScreen.h - the emulator itself (plan 8.2): load an .asm, run it frame by frame on the
// Console, draw its 32 x 16 VRAM, pass the keys through IoPorts and show the CPU live.
// Follows UI_HANDOFF.md sections 4 to 6.
#ifndef CONSOLESCREEN_H
#define CONSOLESCREEN_H

#include <QTimer>
#include <array>
#include <map>
#include <string>
#include <vector>

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
    void runFrames(int n);                 // for --shots: n ticks without the timer (a frame, or one line in CODE view)
    void setCodeView(bool on);             // CODE: step the .asm line by line; SCREEN: run frames and draw VRAM

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
    int currentLine();                     // source line about to run (or that just ran, after HLT/fault); 0 if unknown

    Console console;
    State state = State::Empty;
    QString name, path;
    int frames = 0;
    QTimer timer;
    bool codeView = false;
    int steps = 0;                         // instructions run in CODE view since load/reset
    Word lastPc = 0;
    std::map<Word, int> lineAt;
    std::vector<std::string> source;
    std::array<std::string, 8> regNames;

    ScreenHeader* header;
    StatusChip* chip;
    QLabel* frameLabel;
    QListWidget* carts;
    QStackedWidget* centre;
    VramView* screen;
    QWidget* codePage;
    QPlainTextEdit* code;
    QLabel* nowLabel;
    QLabel* namesLine;
    PixelButton* viewButton;
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
