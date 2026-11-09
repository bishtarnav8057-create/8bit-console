// Tests for the step debugger: load any .asm, step, step back, run, and save the steps as CSV.
// Nothing here reads keys or draws, so it runs on any OS.
// Build and run from the CPU folder:   mingw32-make test

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "../debug/Debugger.h"
#include "../src/SystemState.h"

using namespace std;

int passed = 0;
int failed = 0;

void check(bool condition, const string& name)
{
    cout << (condition ? "[PASS] " : "[FAIL] ") << name << '\n';
    if (condition) passed++; else failed++;
}

const char* TEMP_ASM = "test_debugger_tmp.asm";
const char* TEMP_CSV = "test_debugger_tmp.csv";

void writeFile(const string& file, const string& text)
{
    ofstream out(file);
    out << text;
}

vector<string> readLines(const string& file)
{
    vector<string> lines;
    ifstream in(file);
    string line;
    while (getline(in, line)) lines.push_back(line);
    return lines;
}

// column i (0 = step) of a CSV row whose only quoted field is the instruction
string column(const string& row, int i)
{
    vector<string> cols;
    string cur;
    bool quoted = false;
    for (char ch : row)
    {
        if (ch == '"') quoted = !quoted;
        else if (ch == ',' && !quoted) { cols.push_back(cur); cur.clear(); }
        else cur += ch;
    }
    cols.push_back(cur);
    return i < (int)cols.size() ? cols[i] : "";
}

bool sameRam(Debugger& d, const vector<Byte>& saved)
{
    for (int i = 0; i < (int)saved.size(); ++i)
        if (d.console().bus().peek((Word)(0x8000 + i)) != saved[i]) return false;
    return true;
}

void testSum()
{
    cout << "--sum.asm--\n";
    Debugger d;
    check(d.load("programs/sum.asm"), "sum.asm loads");
    check(d.errors().empty(), "no errors");
    check(d.sourceText(d.currentLine()).find("LDI  R0, 0") != string::npos, "PC starts on the LDI R0, 0 line");
    check(d.currentLine() == 7, "that is line 7");
    check(d.steps() == 0, "no steps recorded yet");

    check(d.run() == StepResult::Halt, "run stops at HLT");
    check(d.console().bus().peek(0x8000) == 55, "RAM[0x8000] = 55");
    check(d.steps() == 34, "34 steps: 2 + 10 x 3 + ST + HLT");
    check(d.step() == StepResult::Halt, "step after HLT returns Halt");
    check(d.steps() == 34, "...and records nothing");

    check(d.writeCsv(TEMP_CSV), "CSV written");
    vector<string> rows = readLines(TEMP_CSV);
    check(rows.size() == 35, "CSV: header + 34 rows");
    check(!rows.empty() && rows[0] == "step,pc,line,instruction,R0,R1,R2,R3,R4,R5,R6,R7,Z,C,N,V,SP,cycles",
          "CSV header");
    check(rows.size() > 1 && rows[1].compare(0, 9, "1,0x0000,") == 0, "first row is step 1 at 0x0000");
    check(rows.size() > 1 && rows[1].find("\"LDI R0, 0x00\"") != string::npos, "instruction text is quoted");
    check(rows.size() > 1 && column(rows[1], 2) == "7", "first row names source line 7");
    check(rows.size() > 2 && column(rows[2], 5) == "10", "row 2 has R1 = 10 after LDI R1, 10");
    check(!rows.empty() && column(rows.back(), 4) == "55", "last row has R0 = 55");
    check(!rows.empty() && column(rows.back(), 0) == "34", "last row is step 34");
    remove(TEMP_CSV);
}

void testBack()
{
    cout << "--step back--\n";
    Debugger d;
    d.load("programs/sum.asm");
    SystemState start = captureState(d.console().cpu());
    vector<Byte> ram(16, 0);
    for (int i = 0; i < 16; ++i) ram[i] = d.console().bus().peek((Word)(0x8000 + i));

    d.run();
    bool allBack = true;
    for (int i = 0; i < 34; ++i) allBack = d.back() && allBack;
    check(allBack, "34 steps back all work");
    check(!d.back(), "a 35th step back is refused");
    check(captureState(d.console().cpu()) == start, "registers, flags, PC, SP, cycles are back to the start");
    check(sameRam(d, ram) && d.console().bus().peek(0x8000) == 0, "RAM is back to the start");

    check(d.step() == StepResult::Ok && d.steps() == 1, "stepping forward again works");
}

void testFault()
{
    cout << "--faults--\n";
    // POP on an empty stack faults at run time (the assembler already rejects ST into ROM)
    writeFile(TEMP_ASM, "    LDI R0, 1\n    POP R0\n    HLT\n");
    Debugger d;
    check(d.load(TEMP_ASM), "the stack-underflow program loads");
    check(d.run() == StepResult::Fault, "POP on an empty stack faults");
    check(d.steps() == 2, "the faulting instruction is recorded");
    check(d.step() == StepResult::Fault && d.steps() == 2, "step after a fault does nothing");
    check(d.back(), "step back over the fault");
    check(d.console().cpu().getState() == CpuState::Running, "the CPU can run again");
    check(d.console().cpu().getPC() == 0x0003, "PC is back on the POP (LDI is 3 bytes)");
    check(d.step() == StepResult::Fault && d.steps() == 2, "stepping the POP again faults again");
    remove(TEMP_ASM);
}

void testIoPorts()
{
    cout << "--step back over I/O ports--\n";
    writeFile(TEMP_ASM,
        "    LDI R0, 65\n    ST  R0, [0xF004]\n"     // print 'A'
        "    LDI R1, 0x12\n    ST  R1, [0xF001]\n"   // new RNG seed
        "    LD  R2, [0xF001]\n"                     // reading RNG moves it on
        "    LDI R3, 5\n    ST  R3, [0xF002]\n"      // PPU control
        "    HLT\n");
    Debugger d;
    check(d.load(TEMP_ASM), "the I/O program loads");
    IoPorts& io = d.console().io();
    Word seed = io.getRandomState();
    check(d.run() == StepResult::Halt && io.getConsoleText() == "A", "it prints A");
    check(io.getRandomState() != seed && io.getPpuControl() == 5, "it changes the RNG and PPU control");
    while (d.back()) {}
    check(io.getRandomState() == seed, "step back restores the RNG state");
    check(io.getPpuControl() == 0, "step back restores PPU control");
    check(io.getConsoleText() == "A", "step back leaves printed text alone (no stray NUL)");
    remove(TEMP_ASM);
}

void testBadFiles()
{
    cout << "--bad files--\n";
    Debugger d;
    bool threw = false;
    bool ok = true;
    try { ok = d.load("programs/no_such_file.asm"); }
    catch (...) { threw = true; }
    check(!threw, "a missing file does not throw");
    check(!ok && !d.errors().empty(), "a missing file is reported");

    writeFile(TEMP_ASM, "    LDI R0, 1\n    LDI R9, 1\n    HLT\n");
    check(!d.load(TEMP_ASM), "a bad register is rejected");
    check(!d.errors().empty() && d.errors()[0].compare(0, 6, "line 2") == 0, "the error names line 2");
    remove(TEMP_ASM);
}

void testReload()
{
    cout << "--loading again--\n";
    Debugger d;
    d.load("programs/sum.asm");
    d.run();
    writeFile(TEMP_ASM, "    LDI R2, 7\n    HLT\n");
    check(d.load(TEMP_ASM), "a second program loads");
    check(d.steps() == 0 && !d.back(), "the old history is gone");
    check(d.console().bus().peek(0x8000) == 0, "the old RAM is gone");
    check(d.run() == StepResult::Halt && d.console().cpu().getReg(2) == 7, "the new program runs");
    check(d.writeCsv(TEMP_CSV) && readLines(TEMP_CSV).size() == 3, "its CSV has only its 2 steps");
    remove(TEMP_CSV);
    remove(TEMP_ASM);
}

void testLimits()
{
    cout << "--limits--\n";
    writeFile(TEMP_ASM, "loop:\n    JMP loop\n");
    Debugger d;
    d.load(TEMP_ASM);
    check(d.run() == StepResult::Ok, "an endless loop stops at the step limit");
    check(d.steps() == 100000, "100000 steps recorded");
    d.run();
    check(d.steps() == 100000, "the history never grows past 100000");
    check(d.writeCsv(TEMP_CSV), "CSV written");
    vector<string> rows = readLines(TEMP_CSV);
    check(rows.size() == 100001 && column(rows[1], 0) == "100001", "the oldest steps were dropped");
    remove(TEMP_CSV);
    remove(TEMP_ASM);

    Debugger e;
    e.load("programs/sum.asm");
    check(!e.writeCsv("no_such_dir/x.csv"), "an unwritable CSV path returns false");
}

int main()
{
    testSum();
    testBack();
    testFault();
    testIoPorts();
    testBadFiles();
    testReload();
    testLimits();

    cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
