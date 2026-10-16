#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <map>
#include <string>
#include <vector>

#include "Instruction.h"
#include "OperandCheck.h"
#include "../Aashish/src/Bits.h"

// Everything the assembler produces for one program.
struct AssembleResult
{
    bool ok = false;
    std::vector<Byte> bytes;                    // load into ROM at 0x0000 with Rom::load
    std::vector<OperandCheckResult> errors;     // only the failures, each with its line number
    std::map<std::string, Word> labels;         // label name (upper case) -> ROM address
    std::map<Word, int> lineAt;                 // instruction address -> source line, for the debugger
};

// Turns parsed assembly into machine code for the PIXEL-8 CPU.
// Encoding follows CPU/Aashish/src/Opcodes.h, so whatever this writes, the CPU runs
// and the Disassembler prints back as the same instruction.
class Assembler
{
public:
    AssembleResult assemble(const std::vector<Instruction>& instructions);

    // Parse + assemble in one step.
    AssembleResult assembleText(const std::string& text);
    AssembleResult assembleFile(const std::string& filename);
};

#endif
