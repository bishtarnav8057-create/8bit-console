// PIXEL-8 DOT: the key bridge.
// Keys on the laptop -> PIXEL-8 buttons (PORT_INPUT) -> programs/dot.asm on our CPU -> VRAM -> this screen.
// The game logic is all in dot.asm; this file only passes keys in and draws VRAM.
// Build and run from the CPU folder:   mingw32-make dot
// Keys: W A S D or the arrow keys move the dot, Q quits.

#include <cstdio>
#include <exception>
#include <string>

#include "../src/Assembler.h"
#include "../Aashish/src/Console.h"
#include "../Aashish/src/MemoryMap.h"

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#endif

int main()
{
    Assembler assembler;
    AssembleResult program;
    try
    {
        program = assembler.assembleFile("programs/dot.asm");
    }
    catch (const std::exception& e)     // the Parser throws if the file is missing
    {
        std::printf("%s (run this from the CPU folder)\n", e.what());
        return 1;
    }
    if (!program.ok)
    {
        for (const OperandCheckResult& e : program.errors)
            std::printf("dot.asm line %d: %s\n", e.lineNumber, e.message.c_str());
        return 1;
    }

    Console console;
    if (!console.loadProgram(program.bytes))
    {
        std::printf("dot.asm is too big for ROM\n");
        return 1;
    }

#ifdef _WIN32
    // shortcut: terminal display, keys only report presses (no release), so each press holds the
    // button for one frame. Replace this block when the GUI window arrives; dot.asm stays.
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (!GetConsoleMode(out, &mode) || !SetConsoleMode(out, mode | 0x0004))   // ENABLE_VIRTUAL_TERMINAL_PROCESSING
    {
        std::printf("dot needs a Windows console (Windows Terminal, PowerShell or cmd).\n"
                    "It can't run with its output redirected or in the Git Bash (mintty) window.\n");
        return 1;
    }
    std::printf("\x1b[2J");             // clear the screen (cursor stays visible, so Ctrl+C leaves the terminal normal)

    for (;;)
    {
        bool quit = false;
        while (_kbhit())
        {
            int k = _getch();
            if (k == 0 || k == 224)     // arrow keys come as two codes
            {
                k = _getch();
                k = k == 72 ? 'W' : k == 80 ? 'S' : k == 75 ? 'A' : k == 77 ? 'D' : 0;
            }
            if (k >= 'a' && k <= 'z') k -= 'a' - 'A';

            if (k == 'W') console.io().setButton(IoPorts::BTN_UP, true);
            else if (k == 'S') console.io().setButton(IoPorts::BTN_DOWN, true);
            else if (k == 'A') console.io().setButton(IoPorts::BTN_LEFT, true);
            else if (k == 'D') console.io().setButton(IoPorts::BTN_RIGHT, true);
            else if (k == 'Q') quit = true;
        }
        if (quit) break;

        StepResult result = console.runFrame();
        console.io().setInput(0);       // release every button again

        std::string screen = "\x1b[H PIXEL-8 DOT: this dot is moved by dot.asm running on our CPU\n\n";
        for (int y = 0; y < MemoryMap::SCREEN_HEIGHT; y++)
        {
            screen += ' ';
            for (int x = 0; x < MemoryMap::SCREEN_WIDTH; x++)
                screen += console.pixel(x, y) ? "##" : "..";
            screen += '\n';
        }
        char footer[96];
        std::snprintf(footer, sizeof(footer), "\n x = %2d  y = %2d    W A S D / arrows = move   Q = quit\n",
                      console.ram().peek(0), console.ram().peek(1));
        screen += footer;
        std::fputs(screen.c_str(), stdout);
        std::fflush(stdout);

        if (result == StepResult::Halt)
        {
            std::printf("\n dot.asm reached HLT.\n");
            break;
        }
        if (result == StepResult::Fault)
        {
            const Fault& f = console.cpu().getFault();
            std::printf("\n The CPU stopped: %s at PC 0x%04X. %s\n", faultName(f.type), f.pc, f.message.c_str());
            break;
        }
        Sleep(50);
    }
    return 0;
#else
    std::printf("Playing needs Windows; run mingw32-make test to check dot.asm.\n");
    return 0;
#endif
}
