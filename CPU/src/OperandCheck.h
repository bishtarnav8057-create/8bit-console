#ifndef OPERANDCHECK_H
#define OPERANDCHECK_H

#include "Instruction.h"
#include <string>
#include <vector>

struct OperandCheckResult {
    bool valid;
    std::string message;
    explicit operator bool() const { return valid; }
};

// Validates the parsed instruction against the starter ISA documented in docs/.
OperandCheckResult checkInstruction(const Instruction& instruction);
std::vector<OperandCheckResult> checkProgram(const std::vector<Instruction>& instructions);

#endif
