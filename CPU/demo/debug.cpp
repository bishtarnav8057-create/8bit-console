// PIXEL-8 STEP DEBUGGER: load any .asm, step forwards and backwards, save every step as CSV.
// Reads whole lines, so it works in cmd, PowerShell, Git Bash, and with piped input.
//   mingw32-make debug ARGS="programs/sum.asm"          (from the CPU folder)
//   debug.exe <file.asm> [out.csv]                      (CSV defaults to trace.csv)

#include <cstdio>
#include <iostream>
#include <string>

#include "../debug/Debugger.h"

using namespace std;

const char* KEYS = "keys: Enter or s = step, b = back, r = run, q = quit (saves the CSV)";

string trim(const string& s)
{
    size_t first = s.find_first_not_of(" \t");
    if (first == string::npos) return "";
    return s.substr(first, s.find_last_not_of(" \t") - first + 1);
}

// one line: how many steps so far, the next instruction, registers, flags, SP
void status(Debugger& d)
{
    CPU& cpu = d.console().cpu();
    int line = d.currentLine();
    string code = d.sourceText(line);
    code = trim(code.substr(0, code.find(';')));    // the code only; the comment would push the registers off screen
    char where[16];
    snprintf(where, sizeof(where), line ? "line %d:" : "line -:", line);
    // "next:" because the step count and registers are after the last step, the line is the one about to run
    printf("#%-6lu PC=0x%04X  next: %-9s %-18s", d.steps(), (unsigned)cpu.getPC(), where, code.c_str());
    for (int i = 0; i < CPU::NUM_REGS; ++i) printf(" R%d=%d", i, cpu.getReg(i));
    printf("  Z=%d C=%d N=%d V=%d  SP=0x%04X\n", cpu.getFlag(CPU::FLAG_Z), cpu.getFlag(CPU::FLAG_C),
           cpu.getFlag(CPU::FLAG_N), cpu.getFlag(CPU::FLAG_V), (unsigned)cpu.getSP());
}

void report(Debugger& d, StepResult result)
{
    if (result == StepResult::Halt) printf("halted\n");
    else if (result == StepResult::Fault)
    {
        const Fault& f = d.console().cpu().getFault();
        printf("FAULT %s: %s\n", faultName(f.type), f.message.c_str());
    }
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("usage: debug.exe <file.asm> [out.csv]\n");
        return 1;
    }
    string csv = argc >= 3 ? argv[2] : "trace.csv";

    Debugger d;
    if (!d.load(argv[1]))
    {
        printf("could not load %s\n", argv[1]);
        for (const string& e : d.errors()) printf("  %s\n", e.c_str());
        return 1;
    }

    printf("PIXEL-8 debugger: %s\n%s\n", argv[1], KEYS);
    status(d);

    string command;
    while (getline(cin, command))                   // end of input counts as q
    {
        command = trim(command);
        if (command == "q") break;
        if (command.empty() || command == "s") report(d, d.step());
        else if (command == "b") { if (!d.back()) printf("at the start\n"); }
        else if (command == "r")
        {
            StepResult result = d.run();
            if (result == StepResult::Ok) printf("still running after %lu steps\n", Debugger::MAX_STEPS);
            report(d, result);
        }
        else { printf("%s\n", KEYS); continue; }
        status(d);
    }

    if (!d.writeCsv(csv))
    {
        printf("could not write %s\n", csv.c_str());
        return 1;
    }
    printf("wrote %s (%lu steps)\n", csv.c_str(), d.steps());
    return 0;
}
