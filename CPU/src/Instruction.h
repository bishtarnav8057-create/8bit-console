#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <string>
#include <vector>

// One line of assembly after parsing. A label definition ("loop:") is its own Instruction
// whose name ends with ':' and has no operands.
struct Instruction
{
    std::string name;
    std::vector<std::string> operands;
    int lineNumber = 0;
};

#endif

