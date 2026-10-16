#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <array>
#include <string>
#include <vector>

#include "../src/Assembler.h"
#include "../Aashish/src/Console.h"

// Where calc.asm finds its inputs and leaves its answers (all in RAM).
namespace CalcMemory
{
    const Word A         = 0x8000;   // first number
    const Word B         = 0x8001;   // second number
    const Word ANSWER    = 0x8002;
    const Word REMAINDER = 0x8003;   // division only
    const Word TOO_BIG   = 0x8004;   // 1 if a multiply did not fit in 8 bits
    const Word DIV_ZERO  = 0x8005;   // 1 if asked to divide by 0
}

// One line of calc.asm that ran, how many times, and the registers after its last run.
struct TraceLine
{
    int line = 0;
    unsigned long count = 0;
    std::array<Byte, CPU::NUM_REGS> registersAfter{};
};

struct CalcResult
{
    bool ok = false;
    std::string error;                // set when ok is false
    Byte answer = 0;
    Byte remainder = 0;
    bool overflow = false;            // + carry, - borrow (went below 0), * did not fit
    bool divideByZero = false;
    Byte flags = 0;                   // the CPU's Z C N V flags when it stopped
    unsigned long instructions = 0;
    std::vector<TraceLine> trace;     // in source order, only lines that ran
};

// Runs calc.asm on the real Console: numbers go into RAM over the Bus, the CPU starts at
// the routine for the operation (do_add, do_mul, ...), and the answer is read back from RAM.
class Calculator
{
public:
    // Reads, checks and assembles the program, then loads it into ROM. False if anything fails.
    bool load(const std::string& filename);
    const std::vector<std::string>& errors() const;

    CalcResult run(char op, Byte a, Byte b);

    // The text of one line of calc.asm (1 = first line), "" if there is no such line.
    std::string sourceLine(int line) const;
    int programSize() const;

    static bool isOperator(char op);

private:
    Console console;
    AssembleResult program;
    std::vector<std::string> source;
    std::vector<std::string> problems;
    bool loaded = false;
};

#endif
