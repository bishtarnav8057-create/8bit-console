#ifndef OPERANDCHECK_H
#define OPERANDCHECK_H

#include "Instruction.h"
#include <string>
#include <vector>

struct OperandCheckResult {
    bool valid;
    std::string message;
    int lineNumber;
    explicit operator bool() const { return valid; }
};

// Validates one parsed instruction against the CPU's OpcodeTable (CPU/Aashish/src/Opcodes.h).
// Labels used as operands are only checked for spelling here; checkProgram checks they exist.
OperandCheckResult checkInstruction(const Instruction& instruction);

// Validates a whole program: every instruction, plus label definitions and references.
// Returns one result per instruction, in the same order.
std::vector<OperandCheckResult> checkProgram(const std::vector<Instruction>& instructions);

// Shared with the Assembler so both read operands exactly the same way.
// Number: decimal (42), hex (0x2A) or binary (0b101010), 0-65535, no minus sign.
bool parseOperandNumber(const std::string& text, unsigned long& value);
// "R0".."R7" in any case -> 0..7, anything else -> -1.
int registerIndex(const std::string& text);

#endif
