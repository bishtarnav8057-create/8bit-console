#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include "Instruction.h"

class Parser
{
public:
    // Reads an .asm file. Throws std::runtime_error if it cannot be opened.
    std::vector<Instruction> parseFile(const std::string& filename);

    // Same as parseFile, but takes the program text directly (for tests and the editor).
    std::vector<Instruction> parseText(const std::string& text);
};

#endif


