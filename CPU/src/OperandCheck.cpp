#include "OperandCheck.h"
#include "../Aashish/src/Opcodes.h"
#include "../Aashish/src/MemoryMap.h"
#include <algorithm>
#include <cctype>
#include <set>

// The instruction list is NOT repeated here. Mnemonics, formats and operand counts all come
// from OpcodeTable, so the checker, the CPU and the disassembler can never disagree.

namespace {
std::string upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::toupper(c)); });
    return s;
}

OperandCheckResult ok(int line) { OperandCheckResult r; r.valid = true; r.message = "OK"; r.lineNumber = line; return r; }
OperandCheckResult fail(int line, const std::string& message) {
    OperandCheckResult r; r.valid = false; r.message = message; r.lineNumber = line; return r;
}

// How each format is written, used in error messages.
const char* usage(Format format) {
    switch (format) {
        case Format::Implied: return "no operands";
        case Format::Reg:     return "Rn";
        case Format::RegReg:  return "Rd, Rs";
        case Format::RegImm:  return "Rn, value";
        case Format::Abs:     return "address or label";
        case Format::RegAbs:  return "Rn, [address or label]";
        case Format::RegInd:  return "Rn (the address is in P = R6:R7)";
    }
    return "";
}

int operandCount(Format format) {
    switch (format) {
        case Format::Implied: return 0;
        case Format::Reg:
        case Format::Abs:
        case Format::RegInd:  return 1;
        default:              return 2;
    }
}

bool isRegister(const std::string& raw) { return registerIndex(raw) >= 0; }
bool parseNumber(const std::string& raw, unsigned long& value) { return parseOperandNumber(raw, value); }

bool looksLikeNumber(const std::string& s) { return !s.empty() && (std::isdigit(static_cast<unsigned char>(s[0])) || s[0] == '-'); }

// A label: letter or '_' first, then letters, digits, '_'. Not a register name or mnemonic.
bool isLabelName(const std::string& s) {
    if (s.empty() || !(std::isalpha(static_cast<unsigned char>(s[0])) || s[0] == '_')) return false;
    if (!std::all_of(s.begin(), s.end(), [](unsigned char c){ return std::isalnum(c) || c == '_'; })) return false;
    return !isRegister(s) && OpcodeTable::find(s) < 0;
}

bool isLabelDefinition(const Instruction& inst) {
    return !inst.name.empty() && inst.name[inst.name.size() - 1] == ':';
}

// Hints for the old starter syntax (MOV A, 5 / LOAD / STORE), so old programs are easy to fix.
std::string oldNameHint(const std::string& name) {
    if (name == "LOAD")  return " (use LDI Rn, value for a number, or LD Rn, [address] for memory)";
    if (name == "STORE") return " (use ST Rn, [address])";
    return "";
}

std::string registerHint(const std::string& op) {
    std::string s = upper(op);
    if (s.size() == 1 && s[0] >= 'A' && s[0] <= 'D') return " (registers are R0-R7)";
    return "";
}

// Checks one operand. Puts the error in 'error' and returns false if it is wrong.
bool checkRegister(const std::string& op, const std::string& name, std::string& error) {
    if (isRegister(op)) return true;
    error = "'" + op + "' is not a register for " + name + registerHint(op);
    return false;
}

bool checkValue(const std::string& op, const std::string& name, std::string& error) {
    unsigned long n = 0;
    if (parseNumber(op, n) && n <= 0xFF) return true;
    if (looksLikeNumber(op)) error = "Value '" + op + "' for " + name + " must be 0-255 (0x00-0xFF)";
    else                     error = "'" + op + "' is not a number for " + name + registerHint(op);
    return false;
}

// Jump targets must be in ROM, or the CPU raises InvalidJumpTarget.
bool checkJumpTarget(const std::string& op, const std::string& name, std::string& error) {
    unsigned long n = 0;
    if (looksLikeNumber(op)) {
        if (!parseNumber(op, n)) { error = "'" + op + "' is not a valid address for " + name; return false; }
        if (n > MemoryMap::ROM_END) { error = name + " target " + op + " is outside ROM (0x0000-0x7FFF)"; return false; }
        return true;
    }
    if (isLabelName(op)) return true;
    error = "'" + op + "' is not an address or label for " + name;
    return false;
}

// Memory operands are written in brackets: LD R0, [0x8000]. ST into ROM raises IllegalWrite.
bool checkMemory(const std::string& op, const std::string& name, std::string& error) {
    if (op.size() < 3 || op[0] != '[' || op[op.size() - 1] != ']') {
        error = name + " needs its address in brackets, e.g. " + name + " R0, [0x8000]";
        return false;
    }
    std::string inner = op.substr(1, op.size() - 2);
    unsigned long n = 0;
    if (looksLikeNumber(inner)) {
        if (!parseNumber(inner, n)) { error = "'" + inner + "' is not a valid address for " + name; return false; }
        if (name == "ST" && n <= MemoryMap::ROM_END) { error = "ST to " + inner + " writes into ROM, which is read-only"; return false; }
        return true;
    }
    if (isLabelName(inner)) return true;
    error = "'" + inner + "' is not an address or label for " + name;
    return false;
}

// The label an operand refers to, or "" if it is a number/register.
std::string labelUsed(const Instruction& inst, Format format) {
    if (format == Format::Abs && !inst.operands.empty() && !looksLikeNumber(inst.operands[0]))
        return inst.operands[0];
    if (format == Format::RegAbs && inst.operands.size() == 2) {
        const std::string& op = inst.operands[1];
        if (op.size() >= 3 && op[0] == '[' && op[op.size() - 1] == ']') {
            std::string inner = op.substr(1, op.size() - 2);
            if (!looksLikeNumber(inner)) return inner;
        }
    }
    return "";
}
}

// R0..R7, any case.
int registerIndex(const std::string& raw) {
    std::string s = upper(raw);
    if (s.size() == 2 && s[0] == 'R' && s[1] >= '0' && s[1] <= '7') return s[1] - '0';
    return -1;
}

// Written by hand because strtoul treats "010" as octal.
bool parseOperandNumber(const std::string& raw, unsigned long& value) {
    std::string s = upper(raw);
    int base = 10;
    size_t start = 0;
    if (s.size() > 2 && s[0] == '0' && (s[1] == 'X' || s[1] == 'B')) { base = (s[1] == 'X') ? 16 : 2; start = 2; }
    if (start >= s.size()) return false;
    value = 0;
    for (size_t i = start; i < s.size(); ++i) {
        int digit;
        if (s[i] >= '0' && s[i] <= '9') digit = s[i] - '0';
        else if (s[i] >= 'A' && s[i] <= 'F') digit = s[i] - 'A' + 10;
        else return false;
        if (digit >= base) return false;
        value = value * base + digit;
        if (value > 0xFFFF) return false;   // bigger than any operand can be
    }
    return true;
}

OperandCheckResult checkInstruction(const Instruction& inst) {
    const int line = inst.lineNumber;

    // "loop:" on its own line defines a label.
    if (isLabelDefinition(inst)) {
        std::string label = inst.name.substr(0, inst.name.size() - 1);
        if (!inst.operands.empty()) return fail(line, "Put label '" + label + ":' on its own line");
        if (!isLabelName(label))    return fail(line, "'" + label + "' cannot be a label name");
        return ok(line);
    }

    const std::string name = upper(inst.name);
    int opcode = OpcodeTable::find(name);
    if (opcode < 0) return fail(line, "Unknown instruction '" + inst.name + "'" + oldNameHint(name));

    const OpcodeInfo& info = OpcodeTable::lookup(static_cast<Byte>(opcode));
    const int expected = operandCount(info.format);
    if (static_cast<int>(inst.operands.size()) != expected)
        return fail(line, name + " expects " + std::to_string(expected) + " operand(s) (" + usage(info.format) +
                          "), got " + std::to_string(inst.operands.size()));

    std::string error;
    bool good = true;
    switch (info.format) {
        case Format::Implied:
            break;
        case Format::Reg:
        case Format::RegInd:
            good = checkRegister(inst.operands[0], name, error);
            break;
        case Format::RegReg:
            good = checkRegister(inst.operands[0], name, error) && checkRegister(inst.operands[1], name, error);
            if (!good && name == "MOV" && isRegister(inst.operands[0]))
                error += " (MOV copies a register; use LDI Rn, value for a number)";
            break;
        case Format::RegImm:
            good = checkRegister(inst.operands[0], name, error) && checkValue(inst.operands[1], name, error);
            break;
        case Format::Abs:
            good = checkJumpTarget(inst.operands[0], name, error);
            break;
        case Format::RegAbs:
            good = checkRegister(inst.operands[0], name, error) && checkMemory(inst.operands[1], name, error);
            break;
    }
    return good ? ok(line) : fail(line, error);
}

std::vector<OperandCheckResult> checkProgram(const std::vector<Instruction>& instructions) {
    std::vector<OperandCheckResult> results;
    results.reserve(instructions.size());

    // Pass 1: check each line and collect label definitions (labels may be used before they appear).
    std::set<std::string> labels;
    for (const auto& inst : instructions) {
        OperandCheckResult r = checkInstruction(inst);
        if (r && isLabelDefinition(inst)) {
            std::string label = upper(inst.name.substr(0, inst.name.size() - 1));
            if (!labels.insert(label).second) r = fail(inst.lineNumber, "Label '" + label + "' is defined twice");
        }
        results.push_back(r);
    }

    // Pass 2: every label an instruction uses must be defined somewhere.
    for (size_t i = 0; i < instructions.size(); ++i) {
        if (!results[i] || isLabelDefinition(instructions[i])) continue;
        int opcode = OpcodeTable::find(instructions[i].name);
        std::string label = labelUsed(instructions[i], OpcodeTable::lookup(static_cast<Byte>(opcode)).format);
        if (!label.empty() && labels.count(upper(label)) == 0)
            results[i] = fail(instructions[i].lineNumber, "Label '" + label + "' is never defined");
    }
    return results;
}
