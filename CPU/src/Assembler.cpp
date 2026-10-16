#include "Assembler.h"
#include "Parser.h"
#include "../Aashish/src/Opcodes.h"
#include "../Aashish/src/MemoryMap.h"

#include <algorithm>
#include <cctype>
#include <cstdio>

namespace
{
    std::string upper(std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        return text;
    }

    bool isLabelDefinition(const Instruction& instruction)
    {
        return !instruction.name.empty() &&
               instruction.name[instruction.name.size() - 1] == ':';
    }

    std::string labelName(const Instruction& instruction)
    {
        return upper(instruction.name.substr(0, instruction.name.size() - 1));
    }

    std::string hex16(Word value)
    {
        char text[8];
        snprintf(text, sizeof(text), "0x%04X", static_cast<unsigned int>(value));
        return text;
    }

    OperandCheckResult error(int line, const std::string& message)
    {
        OperandCheckResult result;
        result.valid = false;
        result.message = message;
        result.lineNumber = line;
        return result;
    }

    // "[0x8000]" -> "0x8000"
    std::string insideBrackets(const std::string& operand)
    {
        return operand.substr(1, operand.size() - 2);
    }

    // A number or a label. The checker has already made sure it is one of the two.
    Word resolveAddress(const std::string& operand, const std::map<std::string, Word>& labels)
    {
        unsigned long number = 0;
        if (parseOperandNumber(operand, number))
        {
            return static_cast<Word>(number);
        }
        return labels.at(upper(operand));
    }

    Byte reg(const std::string& operand)
    {
        return static_cast<Byte>(registerIndex(operand));
    }

    Byte value(const std::string& operand)
    {
        unsigned long number = 0;
        parseOperandNumber(operand, number);
        return static_cast<Byte>(number);
    }
}

AssembleResult Assembler::assemble(const std::vector<Instruction>& instructions)
{
    AssembleResult result;

    // Step 1: check every line. Nothing is encoded if any line is wrong.
    std::vector<OperandCheckResult> checks = checkProgram(instructions);
    for (const OperandCheckResult& check : checks)
    {
        if (!check)
        {
            result.errors.push_back(check);
        }
    }
    if (!result.errors.empty())
    {
        return result;
    }

    // Step 2 (first pass): work out the address of every label.
    // Each instruction's length comes from its format, so no bytes are needed yet.
    unsigned long address = MemoryMap::ROM_START;
    for (const Instruction& instruction : instructions)
    {
        if (isLabelDefinition(instruction))
        {
            result.labels[labelName(instruction)] = static_cast<Word>(address);
            continue;
        }

        Byte opcode = static_cast<Byte>(OpcodeTable::find(instruction.name));
        address += OpcodeTable::lookup(opcode).length;
    }

    const unsigned long romSize = MemoryMap::ROM_END - MemoryMap::ROM_START + 1;
    if (address > romSize)
    {
        result.errors.push_back(error(instructions.back().lineNumber,
            "Program is " + std::to_string(address) + " bytes, but ROM only holds " +
            std::to_string(romSize)));
        return result;
    }

    // Step 3 (second pass): write the bytes. Layout matches CPU::execute:
    //   Implied  op
    //   Reg      op, reg              RegInd  op, reg
    //   RegReg   op, (dst<<4 | src)
    //   RegImm   op, reg, value
    //   Abs      op, low, high        RegAbs  op, reg, low, high
    for (const Instruction& instruction : instructions)
    {
        if (isLabelDefinition(instruction))
        {
            continue;
        }

        const Byte opcode = static_cast<Byte>(OpcodeTable::find(instruction.name));
        const OpcodeInfo& info = OpcodeTable::lookup(opcode);
        const std::vector<std::string>& ops = instruction.operands;

        result.lineAt[static_cast<Word>(result.bytes.size())] = instruction.lineNumber;
        result.bytes.push_back(opcode);

        switch (info.format)
        {
            case Format::Implied:
                break;

            case Format::Reg:
            case Format::RegInd:
                result.bytes.push_back(reg(ops[0]));
                break;

            case Format::RegReg:
                result.bytes.push_back(static_cast<Byte>((reg(ops[0]) << 4) | reg(ops[1])));
                break;

            case Format::RegImm:
                result.bytes.push_back(reg(ops[0]));
                result.bytes.push_back(value(ops[1]));
                break;

            case Format::Abs:
            {
                Word target = resolveAddress(ops[0], result.labels);
                result.bytes.push_back(lowByte(target));
                result.bytes.push_back(highByte(target));
                break;
            }

            case Format::RegAbs:
            {
                const std::string where = insideBrackets(ops[1]);
                Word target = resolveAddress(where, result.labels);

                // Labels are ROM addresses, so ST [label] would fault with IllegalWrite.
                if (opcode == OP_ST && target <= MemoryMap::ROM_END)
                {
                    result.errors.push_back(error(instruction.lineNumber,
                        "ST to '" + where + "' (" + hex16(target) + ") writes into ROM, which is read-only"));
                }

                result.bytes.push_back(reg(ops[0]));
                result.bytes.push_back(lowByte(target));
                result.bytes.push_back(highByte(target));
                break;
            }
        }
    }

    if (!result.errors.empty())
    {
        result.bytes.clear();
        result.lineAt.clear();
        return result;
    }

    result.ok = true;
    return result;
}

AssembleResult Assembler::assembleText(const std::string& text)
{
    Parser parser;
    return assemble(parser.parseText(text));
}

AssembleResult Assembler::assembleFile(const std::string& filename)
{
    Parser parser;
    return assemble(parser.parseFile(filename));
}
