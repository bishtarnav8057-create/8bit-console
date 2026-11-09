// Tests for the key bridge: programs/dot.asm runs on the CPU, reads the buttons from PORT_INPUT
// and moves one dot around VRAM. Nothing here draws a screen, so it runs on any OS.
// Build and run from the CPU folder:   mingw32-make test

#include <cstdlib>
#include <iostream>
#include <string>

#include "../src/Assembler.h"
#include "../Aashish/src/Console.h"
#include "../Aashish/src/MemoryMap.h"

using namespace std;

int passed = 0;
int failed = 0;

void check(bool condition, const string& name)
{
    cout << (condition ? "[PASS] " : "[FAIL] ") << name << '\n';
    if (condition) passed++; else failed++;
}

const Byte UP    = 1 << IoPorts::BTN_UP;
const Byte DOWN  = 1 << IoPorts::BTN_DOWN;
const Byte LEFT  = 1 << IoPorts::BTN_LEFT;
const Byte RIGHT = 1 << IoPorts::BTN_RIGHT;

bool loadDot(Console& c)
{
    Assembler assembler;
    AssembleResult r;
    try
    {
        r = assembler.assembleFile("programs/dot.asm");
    }
    catch (const exception& e)          // the Parser throws if the file is missing
    {
        cout << "  " << e.what() << '\n';
        return false;
    }
    for (const OperandCheckResult& e : r.errors)
        cout << "  line " << e.lineNumber << ": " << e.message << '\n';
    return r.ok && c.loadProgram(r.bytes);
}

// How many pixels of the 32 x 16 screen are lit.
int litCount(Console& c)
{
    int n = 0;
    for (int y = 0; y < MemoryMap::SCREEN_HEIGHT; y++)
        for (int x = 0; x < MemoryMap::SCREEN_WIDTH; x++)
            if (c.pixel(x, y) != 0) n++;
    return n;
}

// One frame with these buttons held, then all buttons released (what the terminal host does).
StepResult frame(Console& c, Byte buttons)
{
    c.io().setInput(buttons);
    StepResult r = c.runFrame();
    c.io().setInput(0);
    return r;
}

// Holds the same buttons for several frames; true if every frame ran without a halt or fault.
bool frames(Console& c, Byte buttons, int count)
{
    bool ok = true;
    for (int i = 0; i < count; i++) ok = (frame(c, buttons) == StepResult::Ok) && ok;
    return ok;
}

// The dot is at (x, y), it is the only lit pixel, and RAM 0x8000/0x8001 (shown by the host) agree.
bool dotAt(Console& c, int x, int y)
{
    return c.pixel(x, y) == 1 && litCount(c) == 1 && c.ram().peek(0) == x && c.ram().peek(1) == y;
}

int main()
{
    Console c;

    cout << "-- loading dot.asm --\n";
    bool loaded = loadDot(c);
    check(loaded, "dot.asm passes the checker and assembles");
    if (!loaded)
    {
        cout << "\n" << passed << " passed, " << failed << " failed\n";
        return 1;
    }

    cout << "-- start --\n";
    check(frame(c, 0) == StepResult::Ok, "the first frame runs without a halt or fault");
    check(dotAt(c, 16, 8), "the dot starts in the middle at (16, 8)");

    cout << "-- one button per frame --\n";
    frame(c, RIGHT); check(dotAt(c, 17, 8), "RIGHT moves the dot to (17, 8)");
    frame(c, LEFT);  check(dotAt(c, 16, 8), "LEFT moves it back to (16, 8)");
    frame(c, UP);    check(dotAt(c, 16, 7), "UP moves the dot to (16, 7)");
    frame(c, DOWN);  check(dotAt(c, 16, 8), "DOWN moves it back to (16, 8)");

    cout << "-- two buttons at once --\n";
    frame(c, UP | RIGHT); check(dotAt(c, 17, 7), "UP + RIGHT moves diagonally to (17, 7)");

    cout << "-- no buttons --\n";
    check(frames(c, 0, 50), "50 frames with no buttons run cleanly");
    check(dotAt(c, 17, 7), "with no buttons the dot stays at (17, 7)");

    cout << "-- edges --\n";
    check(frames(c, LEFT, 40), "40 frames of LEFT run cleanly");
    check(dotAt(c, 0, 7), "LEFT stops at the left edge (0, 7)");
    check(frames(c, RIGHT, 40), "40 frames of RIGHT run cleanly");
    check(dotAt(c, 31, 7), "RIGHT stops at the right edge (31, 7)");
    check(frames(c, UP, 20), "20 frames of UP run cleanly");
    check(dotAt(c, 31, 0), "UP stops at the top edge (31, 0)");
    check(frames(c, DOWN, 20), "20 frames of DOWN run cleanly");
    check(dotAt(c, 31, 15), "DOWN stops at the bottom edge (31, 15)");

    cout << "-- random buttons --\n";
    srand(1);
    bool allOk = true;
    bool oneDot = true;
    for (int i = 0; i < 1000; i++)
    {
        allOk = (frame(c, (Byte)(rand() & 0x0F)) == StepResult::Ok) && allOk;
        oneDot = (litCount(c) == 1) && oneDot;
    }
    check(allOk, "1000 frames of random buttons: no halt or fault");
    check(oneDot, "1000 frames of random buttons: always exactly one dot");
    check(c.ram().peek(0) < 32 && c.ram().peek(1) < 16, "x and y in RAM stay on the screen");

    cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
