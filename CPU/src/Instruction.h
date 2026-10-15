#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <string>
#include <vector>

struct Instruction
{
    std::string name;
    std::vector<std::string> operands;
    int lineNumber;
};

#endif
