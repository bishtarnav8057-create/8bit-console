#include "Calculator.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>

namespace
{
    // A loop that never reaches HLT is stopped after this many instructions.
    const unsigned long MAX_INSTRUCTIONS = 100000;

    // The label in calc.asm for each operation ("" if there is none).
    std::string routineFor(char op)
    {
        switch (op)
        {
            case '+': return "DO_ADD";
            case '-': return "DO_SUB";
            case '*': return "DO_MUL";
            case '/': return "DO_DIV";
            case '&': return "DO_AND";
            case '|': return "DO_OR";
            case '^': return "DO_XOR";
        }
        return "";
    }
}

bool Calculator::isOperator(char op)
{
    return !routineFor(op).empty();
}

bool Calculator::load(const std::string& filename)
{
    loaded = false;
    problems.clear();
    source.clear();

    std::ifstream file(filename);
    if (!file.is_open())
    {
        problems.push_back("Could not open " + filename);
        return false;
    }

    std::stringstream text;
    text << file.rdbuf();

    std::string line;
    std::istringstream lines(text.str());
    while (std::getline(lines, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
        source.push_back(line);
    }

    // Arnav's assembler runs Piyush's operand checker first, so this does both.
    Assembler assembler;
    program = assembler.assembleText(text.str());
    if (!program.ok)
    {
        for (const OperandCheckResult& e : program.errors)
        {
            problems.push_back("line " + std::to_string(e.lineNumber) + ": " + e.message);
        }
        return false;
    }

    if (!console.loadProgram(program.bytes))
    {
        problems.push_back("Program does not fit in ROM");
        return false;
    }

    loaded = true;
    return true;
}

const std::vector<std::string>& Calculator::errors() const
{
    return problems;
}

CalcResult Calculator::run(char op, Byte a, Byte b)
{
    CalcResult result;

    if (!loaded)
    {
        result.error = "calc.asm is not loaded";
        return result;
    }

    const std::string routine = routineFor(op);
    if (routine.empty())
    {
        result.error = std::string("Unknown operation '") + op + "'";
        return result;
    }

    std::map<std::string, Word>::const_iterator start = program.labels.find(routine);
    if (start == program.labels.end())
    {
        result.error = "calc.asm has no " + routine + " label";
        return result;
    }

    // Put the numbers into RAM through the Bus, and clear the answer slots.
    Bus& bus = console.bus();
    bus.write(CalcMemory::A, a);
    bus.write(CalcMemory::B, b);
    bus.write(CalcMemory::ANSWER, 0);
    bus.write(CalcMemory::REMAINDER, 0);
    bus.write(CalcMemory::TOO_BIG, 0);
    bus.write(CalcMemory::DIV_ZERO, 0);

    CPU& cpu = console.cpu();
    cpu.reset(start->second);

    // Run until HLT, counting how often each source line runs.
    std::map<int, size_t> traceIndex;
    StepResult step = StepResult::Ok;
    while (step == StepResult::Ok && result.instructions < MAX_INSTRUCTIONS)
    {
        const Word pc = cpu.getPC();
        step = console.step();
        result.instructions++;

        std::map<Word, int>::const_iterator line = program.lineAt.find(pc);
        if (line == program.lineAt.end()) continue;

        std::map<int, size_t>::iterator seen = traceIndex.find(line->second);
        if (seen == traceIndex.end())
        {
            seen = traceIndex.insert(std::make_pair(line->second, result.trace.size())).first;
            TraceLine fresh;
            fresh.line = line->second;
            result.trace.push_back(fresh);
        }

        TraceLine& t = result.trace[seen->second];
        t.count++;
        for (int i = 0; i < CPU::NUM_REGS; i++) t.registersAfter[i] = cpu.getReg(i);
    }

    std::sort(result.trace.begin(), result.trace.end(),
              [](const TraceLine& x, const TraceLine& y) { return x.line < y.line; });

    if (step == StepResult::Fault)
    {
        result.error = "CPU fault: " + cpu.getFault().message;
        return result;
    }
    if (step != StepResult::Halt)
    {
        result.error = "The program did not stop after " + std::to_string(MAX_INSTRUCTIONS) + " instructions";
        return result;
    }

    // Read the answers back out of RAM. peek() changes nothing, so the reads leave no trace.
    result.answer = bus.peek(CalcMemory::ANSWER);
    result.remainder = bus.peek(CalcMemory::REMAINDER);
    result.divideByZero = bus.peek(CalcMemory::DIV_ZERO) != 0;
    result.flags = cpu.getFlags();

    if (op == '+' || op == '-')
    {
        result.overflow = cpu.getFlag(CPU::FLAG_C);   // ST and HLT keep the ADD/SUB flags
    }
    else if (op == '*')
    {
        result.overflow = bus.peek(CalcMemory::TOO_BIG) != 0;
    }

    result.ok = true;
    return result;
}

std::string Calculator::sourceLine(int line) const
{
    if (line < 1 || line > static_cast<int>(source.size())) return "";
    return source[line - 1];
}

int Calculator::programSize() const
{
    return static_cast<int>(program.bytes.size());
}
