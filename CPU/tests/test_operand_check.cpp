// Tests for OperandCheck and SystemState against the real CPU code in CPU/Aashish/src.
// Build from the CPU folder (Windows/MinGW or Linux):
//   g++ -std=c++11 -Wall -Wextra tests/test_operand_check.cpp src/OperandCheck.cpp src/SystemState.cpp src/Parser.cpp
//       Aashish/src/Bits.cpp Aashish/src/Fault.cpp Aashish/src/Ram.cpp Aashish/src/Rom.cpp Aashish/src/Bus.cpp
//       Aashish/src/Opcodes.cpp Aashish/src/CPU.cpp Aashish/src/Disassembler.cpp -o test_operand_check.exe

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "../src/OperandCheck.h"
#include "../src/SystemState.h"
#include "../src/Parser.h"
#include "../Aashish/src/Ram.h"
#include "../Aashish/src/Rom.h"
#include "../Aashish/src/Bus.h"
#include "../Aashish/src/Disassembler.h"

int passed = 0;
int failed = 0;

void check(bool condition, const std::string& name) {
    printf("[%s] %s\n", condition ? "PASS" : "FAIL", name.c_str());
    condition ? passed++ : failed++;
}

Instruction make(const std::string& name, const std::vector<std::string>& operands, int line = 1) {
    Instruction inst;
    inst.name = name;
    inst.operands = operands;
    inst.lineNumber = line;
    return inst;
}

bool valid(const std::string& name, const std::vector<std::string>& operands) {
    return checkInstruction(make(name, operands)).valid;
}

// Writes text to a file and runs Arnav's Parser on it, so the tests use the real pipeline.
std::vector<Instruction> parseText(const std::string& text) {
    std::ofstream file("operand_check_test.asm");
    file << text;
    file.close();
    Parser parser;
    return parser.parseFile("operand_check_test.asm");
}

bool allValid(const std::vector<OperandCheckResult>& results) {
    for (const auto& r : results) if (!r) return false;
    return true;
}

void testEveryFormat() {
    printf("--Every instruction format--\n");
    check(valid("NOP", {}), "NOP with no operands");
    check(valid("hlt", {}), "lower case mnemonics work");
    check(valid("INC", {"R1"}), "INC R1");
    check(valid("ADD", {"R0", "r7"}), "ADD R0, r7");
    check(valid("MOV", {"R2", "R3"}), "MOV R2, R3");
    check(valid("LDI", {"R0", "42"}), "LDI R0, 42 (decimal)");
    check(valid("LDI", {"R0", "0xFF"}), "LDI R0, 0xFF (hex)");
    check(valid("LDI", {"R0", "0b1010"}), "LDI R0, 0b1010 (binary)");
    check(valid("JMP", {"0x0010"}), "JMP 0x0010");
    check(valid("LD", {"R0", "[0x8000]"}), "LD R0, [0x8000]");
    check(valid("ST", {"R0", "[0xE000]"}), "ST R0, [0xE000] (VRAM)");
    check(valid("LDP", {"R3"}), "LDP R3");
    check(valid("PUSHF", {}), "PUSHF");
}

void testRejected() {
    printf("--Wrong programs are rejected--\n");
    check(!valid("FOO", {}), "unknown instruction");
    check(!valid("NOP", {"R1"}), "NOP with an operand");
    check(!valid("ADD", {"R0"}), "ADD with one operand");
    check(!valid("INC", {"R8"}), "R8 does not exist");
    check(!valid("ADD", {"R0", "5"}), "ADD takes two registers, not a number");
    check(!valid("LDI", {"R0", "256"}), "256 does not fit in a byte");
    check(!valid("LDI", {"R0", "-1"}), "negative numbers are refused");
    check(!valid("LDI", {"R0", "0xZZ"}), "bad hex is refused");
    check(!valid("JMP", {"0x8000"}), "jump into RAM is refused (CPU would raise InvalidJumpTarget)");
    check(!valid("LD", {"R0", "0x8000"}), "LD without brackets is refused");
    check(!valid("ST", {"R0", "[0x0010]"}), "ST into ROM is refused (CPU would raise IllegalWrite)");
    check(!valid("JMP", {"R0"}), "a register is not a jump target");
}

void testOldSyntax() {
    printf("--Old starter syntax gives a helpful message--\n");
    OperandCheckResult r = checkInstruction(make("MOV", {"A", "5"}));
    check(!r && r.message.find("R0-R7") != std::string::npos, "MOV A, 5 says registers are R0-R7");
    printf("        example: %s\n", r.message.c_str());
    r = checkInstruction(make("MOV", {"R0", "5"}));
    check(!r && r.message.find("LDI") != std::string::npos, "MOV R0, 5 suggests LDI");
    printf("        example: %s\n", r.message.c_str());
    r = checkInstruction(make("LOAD", {"R0", "5"}));
    check(!r && r.message.find("LDI") != std::string::npos, "LOAD suggests LDI / LD");
    r = checkInstruction(make("STORE", {"R0", "5"}));
    check(!r && r.message.find("ST") != std::string::npos, "STORE suggests ST");
}

void testLabels() {
    printf("--Labels--\n");
    std::vector<OperandCheckResult> results = checkProgram(parseText(
        "; count R0 down to zero\n"
        "start:\n"
        "    LDI R0, 5\n"
        "loop:\n"
        "    DEC R0\n"
        "    JNZ loop\n"
        "    CALL done\n"
        "    JMP start\n"
        "done:\n"
        "    RET\n"));
    check(results.size() == 9 && allValid(results), "labels used before and after they are defined");

    results = checkProgram(parseText("JMP nowhere\n"));
    check(results.size() == 1 && !results[0] && results[0].message.find("never defined") != std::string::npos,
          "undefined label is reported");
    check(results.size() == 1 && results[0].lineNumber == 1, "error carries the line number");

    results = checkProgram(parseText("here:\nhere:\nJMP here\n"));
    check(results.size() == 3 && !results[1] && results[1].lineNumber == 2, "label defined twice is reported on line 2");

    results = checkProgram(parseText("value:\nLD R0, [value]\n"));
    check(results.size() == 2 && allValid(results), "label inside brackets for LD");

    // (the new Parser splits "loop: DEC R0" into two Instructions; unsplit, the checker refuses it)
    check(!checkInstruction(make("loop:", {"DEC R0"})).valid, "unsplit label + instruction is refused");

    check(!checkInstruction(make("R1:", {})).valid, "a register name cannot be a label");
    check(!checkInstruction(make("ADD:", {})).valid, "a mnemonic cannot be a label");
}

// Every instruction the disassembler prints must pass the checker, so the two always agree.
void testAgreesWithDisassembler() {
    printf("--Agrees with the CPU's OpcodeTable and Disassembler--\n");
    int checkedCount = 0;
    bool allAgree = true;
    for (int op = 0; op < 256; ++op) {
        const OpcodeInfo& info = OpcodeTable::lookup(static_cast<Byte>(op));
        if (!info.valid) continue;

        // operand bytes: register 1 / regs R0,R1 / value 0 / jump 0x0001 / memory 0x8000
        Byte program[4] = {static_cast<Byte>(op), 0x01, 0x00, 0x80};
        Rom rom(0x8000);
        rom.load(program, 4);
        Bus bus;
        bus.attach(MemoryMap::ROM_START, MemoryMap::ROM_END, &rom);
        Disassembler dis(bus);
        Byte length = 0;
        std::string text = dis.line(0x0000, length).substr(8);   // drop "0x0000  "

        std::vector<Instruction> parsed = parseText(text + "\n");
        if (parsed.size() != 1 || !checkInstruction(parsed[0])) {
            printf("        disagrees on: %s\n", text.c_str());
            allAgree = false;
        }
        checkedCount++;
    }
    check(allAgree, "all " + std::to_string(checkedCount) + " disassembled instructions pass the checker");
    check(checkedCount == 38, "the checker covers all 38 CPU instructions");
}

void testSystemState() {
    printf("--SystemState matches the CPU--\n");
    Rom rom(0x8000);
    Ram ram(0x6000, "RAM");
    Ram stack(0x0F00, "STACK");
    Bus bus;
    bus.attach(MemoryMap::ROM_START, MemoryMap::ROM_END, &rom);
    bus.attach(MemoryMap::RAM_START, MemoryMap::RAM_END, &ram);
    bus.attach(MemoryMap::STACK_START, MemoryMap::STACK_END, &stack);
    CPU cpu(bus);

    SystemState fresh = captureState(cpu);
    SystemState defaults;
    check(fresh == defaults, "a reset CPU equals a default SystemState");
    check(fresh.stackPointer == 0xFFFF, "SP starts at 0xFFFF");

    // LDI R0, 0x80 / PUSH R0 / HLT
    Byte program[] = {OP_LDI, 0x00, 0x80, OP_PUSH, 0x00, OP_HLT};
    rom.load(program, sizeof(program));
    cpu.reset();
    cpu.run(100);
    SystemState s = captureState(cpu);
    check(s.registers[0] == 0x80, "R0 is copied");
    check(s.negativeFlag && !s.zeroFlag, "N and Z flags are copied");
    check(s.stackPointer == 0xFFFE, "SP is copied after PUSH");
    check(s.programCounter == cpu.getPC(), "PC is copied");
    check(s.cpuState == CpuState::Halted, "Halted state is copied");
    check(s.cycles == cpu.getCycles() && s.cycles > 0, "cycle count is copied");
    check(s != fresh, "a changed CPU is not equal to the old snapshot");

    // undo: snapshot halfway, run on, restore, and the CPU is exactly back at the snapshot
    Byte counter[] = {OP_LDI, 0x00, 0x00,  OP_INC, 0x00,  OP_CMPI, 0x00, 0x0A,  OP_JNZ, 0x03, 0x00,  OP_HLT};
    rom.load(counter, sizeof(counter));
    cpu.reset();
    cpu.run(10);
    SystemState halfway = captureState(cpu);
    cpu.run(100);
    check(cpu.getState() == CpuState::Halted && cpu.getReg(0) == 10, "the counter program runs to 10 and halts");
    restoreState(cpu, halfway);
    check(captureState(cpu) == halfway, "restoreState puts the CPU back exactly as captured");
    cpu.run(100);
    check(cpu.getState() == CpuState::Halted && cpu.getReg(0) == 10, "after restoring, it runs to the same end again");
}

int main() {
    testEveryFormat();
    testRejected();
    testOldSyntax();
    testLabels();
    testAgreesWithDisassembler();
    testSystemState();
    std::remove("operand_check_test.asm");
    printf("\n%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
