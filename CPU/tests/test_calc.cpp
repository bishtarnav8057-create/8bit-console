// Tests for the calculator: every answer comes from programs/calc.asm running on the CPU,
// and is compared with the same sum done in C++.
// Build and run from the CPU folder:   mingw32-make test

#include <iostream>
#include <string>

#include "../calc/Calculator.h"

using namespace std;

int passed = 0;
int failed = 0;

void check(bool condition, const string& name)
{
    cout << (condition ? "[PASS] " : "[FAIL] ") << name << '\n';
    if (condition) passed++; else failed++;
}

// "    ADD  R0, R1" -> "ADD R0, R1", so lines can be found however they are lined up.
string singleSpaced(const string& text)
{
    string result;
    for (char c : text)
    {
        if (c == ' ' || c == '\t')
        {
            if (!result.empty() && result[result.size() - 1] != ' ') result += ' ';
        }
        else
        {
            result += c;
        }
    }
    return result;
}

// How many times a source line ran, found by the text on that line.
unsigned long timesRun(const Calculator& calc, const CalcResult& r, const string& code)
{
    for (const TraceLine& t : r.trace)
    {
        if (singleSpaced(calc.sourceLine(t.line)).find(code) != string::npos) return t.count;
    }
    return 0;
}

// Runs one operation on every pair a, b (0-255) and compares with 'expected'.
template <typename F>
void checkEveryPair(Calculator& calc, char op, F expected, const string& name)
{
    int wrong = 0;
    for (int a = 0; a < 256; a++)
    {
        for (int b = 0; b < 256; b++)
        {
            CalcResult r = calc.run(op, static_cast<Byte>(a), static_cast<Byte>(b));
            if (!r.ok || r.answer != static_cast<Byte>(expected(a, b))) wrong++;
        }
    }
    check(wrong == 0, name + ": all 65536 pairs match C++ (" + to_string(wrong) + " wrong)");
}

int main()
{
    Calculator calc;

    cout << "-- loading calc.asm --\n";
    bool loaded = calc.load("programs/calc.asm");
    check(loaded, "calc.asm passes the checker and assembles");
    for (const string& e : calc.errors()) cout << "    " << e << '\n';
    if (!loaded)
    {
        cout << "\n" << passed << " passed, " << failed << " failed\n";
        return 1;
    }
    check(calc.programSize() > 0, "program bytes are in ROM");

    cout << "-- simple answers --\n";
    CalcResult r = calc.run('+', 25, 17);
    check(r.ok && r.answer == 42 && !r.overflow, "25 + 17 = 42");
    check(r.instructions == 5, "25 + 17 takes 5 instructions (LD, LD, ADD, ST, HLT)");
    check(timesRun(calc, r, "ADD R0, R1") == 1, "the ADD line ran once");

    r = calc.run('-', 50, 8);
    check(r.ok && r.answer == 42 && !r.overflow, "50 - 8 = 42");
    r = calc.run('&', 0x0F, 0x3C);
    check(r.ok && r.answer == 0x0C, "0x0F & 0x3C = 0x0C");
    r = calc.run('|', 0x0F, 0x30);
    check(r.ok && r.answer == 0x3F, "0x0F | 0x30 = 0x3F");
    r = calc.run('^', 0xFF, 0x0F);
    check(r.ok && r.answer == 0xF0, "0xFF ^ 0x0F = 0xF0");

    cout << "-- 8-bit limits --\n";
    r = calc.run('+', 200, 100);
    check(r.ok && r.answer == 44 && r.overflow, "200 + 100 wraps to 44 and sets carry");
    check((r.flags & (1 << CPU::FLAG_C)) != 0, "the CPU's own C flag is set after 200 + 100");
    r = calc.run('-', 5, 9);
    check(r.ok && r.answer == 252 && r.overflow, "5 - 9 = 252 (-4) and sets borrow");
    check((r.flags & (1 << CPU::FLAG_N)) != 0, "N flag is set after 5 - 9");
    r = calc.run('+', 255, 0);
    check(r.ok && r.answer == 255 && !r.overflow, "255 + 0 = 255, no carry");

    cout << "-- multiply: repeated addition --\n";
    r = calc.run('*', 7, 6);
    check(r.ok && r.answer == 42 && !r.overflow, "7 * 6 = 42");
    check(timesRun(calc, r, "ADD R2, R0") == 6, "7 * 6 adds 7 six times");
    r = calc.run('*', 9, 0);
    check(r.ok && r.answer == 0, "9 * 0 = 0");
    check(timesRun(calc, r, "ADD R2, R0") == 0, "9 * 0 never adds");
    r = calc.run('*', 20, 20);
    check(r.ok && r.answer == static_cast<Byte>(400) && r.overflow, "20 * 20 wraps (400 > 255) and reports it");

    cout << "-- divide: repeated subtraction --\n";
    r = calc.run('/', 50, 7);
    check(r.ok && r.answer == 7 && r.remainder == 1, "50 / 7 = 7 remainder 1");
    check(timesRun(calc, r, "SUB R0, R1") == 7, "50 / 7 subtracts 7 seven times");
    r = calc.run('/', 3, 10);
    check(r.ok && r.answer == 0 && r.remainder == 3, "3 / 10 = 0 remainder 3");
    r = calc.run('/', 9, 0);
    check(r.ok && r.divideByZero, "9 / 0 is caught by the program, not a crash");

    cout << "-- the CPU against C++, every possible input --\n";
    checkEveryPair(calc, '+', [](int a, int b) { return a + b; }, "+");
    checkEveryPair(calc, '-', [](int a, int b) { return a - b; }, "-");
    checkEveryPair(calc, '&', [](int a, int b) { return a & b; }, "&");
    checkEveryPair(calc, '|', [](int a, int b) { return a | b; }, "|");
    checkEveryPair(calc, '^', [](int a, int b) { return a ^ b; }, "^");

    int wrong = 0;
    int pairs = 0;
    for (int a = 0; a < 256; a += 3)
    {
        for (int b = 0; b < 256; b += 5)
        {
            pairs++;
            CalcResult m = calc.run('*', static_cast<Byte>(a), static_cast<Byte>(b));
            if (!m.ok || m.answer != static_cast<Byte>(a * b) || m.overflow != (a * b > 255)) wrong++;
            if (b == 0) continue;
            CalcResult d = calc.run('/', static_cast<Byte>(a), static_cast<Byte>(b));
            if (!d.ok || d.answer != a / b || d.remainder != a % b) wrong++;
        }
    }
    check(wrong == 0, "* and / match C++ on " + to_string(pairs) + " sample pairs (" + to_string(wrong) + " wrong)");

    cout << "-- bad input --\n";
    r = calc.run('?', 1, 2);
    check(!r.ok, "an unknown operator is refused");

    cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
