#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include "Instruction.h"

class Parser
{
public:
    std::vector<Instruction> parseFile(const std::string& filename);
};

#endif
