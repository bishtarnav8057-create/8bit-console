// Tests for the Assembler: exact bytes, labels, errors, and real programs run on the CPU.
// Build from the CPU folder:
//   g++ -std=c++11 -Wall -Wextra tests/test_assembler.cpp src/Assembler.cpp src/Parser.cpp src/OperandCheck.cpp
//       Aashish/src/Bits.cpp Aashish/src/Fault.cpp Aashish/src/Ram.cpp Aashish/src/Rom.cpp Aashish/src/Bus.cpp
//       Aashish/src/Opcodes.cpp Aashish/src/CPU.cpp Aashish/src/Disassembler.cpp -o test_assembler.exe

#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../src/Assembler.h"
#include "../src/Parser.h"
#include "../Aashish/src/MemoryMap.h"
#include "../Aashish/src/Ram.h"
#include "../Aashish/src/Rom.h"
#include "../Aashish/src/Bus.h"
#include "../Aashish/src/Opcodes.h"
#include "../Aashish/src/CPU.h"
#include "../Aashish/src/Disassembler.h"

using namespace std;

int passed = 0;
int failed = 0;

void check(bool condition, const string& name)
{
    cout << (condition ? "[PASS] " : "[FAIL] ") << name << '\n';
    condition ? passed++ : failed++;
}

// A console wired like MemoryMap.h, the same way as CPU/Aashish/tests/test_cpu.cpp.
struct Machine
{
    Rom rom;
    Ram ram;
    Ram vram;
    Ram stack;
    Bus bus;
    CPU cpu;

    Machine() : rom(0x8000), ram(0x6000, "RAM"), vram(0x1000, "VRAM"), stack(0x0F00, "STACK"), bus(), cpu(bus)
    {
        bus.attach(MemoryMap::ROM_START, MemoryMap::ROM_END, &rom);
        bus.attach(MemoryMap::RAM_START, MemoryMap::RAM_END, &ram);
        bus.attach(MemoryMap::VRAM_START, MemoryMap::VRAM_END, &vram);
        bus.attach(MemoryMap::STACK_START, MemoryMap::STACK_END, &stack);
    }

    // Assembles, loads into ROM and runs until HLT. Returns false if anything failed.
    bool run(const string& source)
    {
        Assembler assembler;
        AssembleResult result = assembler.assembleText(source);
        for (const OperandCheckResult& e : result.errors)
        {
            cout << "        line " << e.lineNumber << ": " << e.message << '\n';
        }
        if (!result.ok) return false;

        rom.load(result.bytes.data(), static_cast<unsigned int>(result.bytes.size()));
        cpu.reset();
        return cpu.run(100000) == StepResult::Halt;
    }
};

bool bytesAre(const AssembleResult& result, const vector<Byte>& expected)
{
    return result.ok && result.bytes == expected;
}

void testEncoding()
{
    cout << "--Every format encodes to the right bytes--\n";
    Assembler a;
    check(bytesAre(a.assembleText("NOP\nHLT\n"), {0x00, 0x01}), "Implied: NOP, HLT");
    check(bytesAre(a.assembleText("INC R1\n"), {0x40, 0x01}), "Reg: INC R1 -> 40 01");
    check(bytesAre(a.assembleText("ADD R2, R5\n"), {0x30, 0x25}), "RegReg: ADD R2, R5 -> 30 25 (dst high, src low)");
    check(bytesAre(a.assembleText("LDI R0, 0x2A\n"), {0x11, 0x00, 0x2A}), "RegImm: LDI R0, 0x2A -> 11 00 2A");
    check(bytesAre(a.assembleText("JMP 0x1234\n"), {0x60, 0x34, 0x12}), "Abs: JMP 0x1234 -> 60 34 12 (low byte first)");
    check(bytesAre(a.assembleText("LD R3, [0x8001]\n"), {0x12, 0x03, 0x01, 0x80}), "RegAbs: LD R3, [0x8001] -> 12 03 01 80");
    check(bytesAre(a.assembleText("STP R4\n"), {0x15, 0x04}), "RegInd: STP R4 -> 15 04");
    check(bytesAre(a.assembleText("ldi r1, 10\n"), {0x11, 0x01, 0x0A}), "lower case works");
}

void testLabels()
{
    cout << "--Labels--\n";
    Assembler a;
    AssembleResult r = a.assembleText(
        "start:\n"          // 0x0000
        "    JMP end\n"     // 0x0000, 3 bytes
        "middle:\n"         // 0x0003
        "    NOP\n"         // 0x0003, 1 byte
        "end: JMP start\n"  // 0x0004
        "    JMP Middle\n");// 0x0007, labels ignore case
    check(r.ok, "program with labels assembles");
    check(r.labels["START"] == 0x0000 && r.labels["MIDDLE"] == 0x0003 && r.labels["END"] == 0x0004,
          "label addresses: start=0x0000 middle=0x0003 end=0x0004");
    check(bytesAre(r, {0x60, 0x04, 0x00,  0x00,  0x60, 0x00, 0x00,  0x60, 0x03, 0x00}),
          "forward and backward jumps use the right addresses");
    check(r.lineAt[0x0000] == 2 && r.lineAt[0x0003] == 4 && r.lineAt[0x0004] == 5 && r.lineAt[0x0007] == 6,
          "lineAt maps each address back to its source line");

    r = a.assembleText("JMP data\ndata:\nLD R0, [data]\nHLT\n");
    check(bytesAre(r, {0x60, 0x03, 0x00,  0x12, 0x00, 0x03, 0x00,  0x01}), "a label can be read with LD [label]");
}

void testErrors()
{
    cout << "--Errors--\n";
    Assembler a;
    AssembleResult r = a.assembleText("LDI R0, 5\nMOV A, 5\nADD R0\nHLT\n");
    check(!r.ok && r.bytes.empty(), "a wrong program gives no bytes");
    check(r.errors.size() == 2 && r.errors[0].lineNumber == 2 && r.errors[1].lineNumber == 3,
          "every wrong line is reported, with its line number (2 and 3)");

    r = a.assembleText("JMP nowhere\n");
    check(!r.ok && r.errors.size() == 1 && r.errors[0].message.find("never defined") != string::npos, "undefined label");

    r = a.assembleText("score:\nST R0, [score]\n");
    check(!r.ok && r.errors.size() == 1 && r.errors[0].lineNumber == 2, "ST into a label (ROM) is refused");
    if (!r.errors.empty()) cout << "        example: " << r.errors[0].message << '\n';

    string big;
    for (int i = 0; i < 0x8000 / 3 + 1; ++i) big += "LDI R0, 1\n";
    r = a.assembleText(big);
    check(!r.ok && r.errors.size() == 1 && r.errors[0].message.find("ROM") != string::npos, "a program bigger than 32 KB is refused");

    bool threw = false;
    try { a.assembleFile("this_file_does_not_exist.asm"); }
    catch (const runtime_error&) { threw = true; }
    check(threw, "a missing file throws runtime_error");
}

// For every opcode: bytes -> Disassembler -> text -> Assembler -> must give the same bytes.
void testRoundTrip()
{
    cout << "--Round trip with the Disassembler--\n";
    Assembler a;
    int count = 0;
    bool allSame = true;
    for (int op = 0; op < 256; ++op)
    {
        const OpcodeInfo& info = OpcodeTable::lookup(static_cast<Byte>(op));
        if (!info.valid) continue;

        // register R1 / registers R0, R1 / value 0 / memory 0x8000 (RAM, so ST is legal)
        Byte original[4] = {static_cast<Byte>(op), 0x01, 0x00, 0x80};
        if (info.format == Format::Abs) { original[1] = 0x10; original[2] = 0x00; }   // jump into ROM

        Rom rom(0x8000);
        rom.load(original, 4);
        Bus bus;
        bus.attach(MemoryMap::ROM_START, MemoryMap::ROM_END, &rom);
        Disassembler dis(bus);
        Byte length = 0;
        string text = dis.line(0x0000, length).substr(8);

        AssembleResult r = a.assembleText(text + "\n");
        vector<Byte> expected(original, original + info.length);
        if (!r.ok || r.bytes != expected)
        {
            cout << "        differs: " << text << '\n';
            allSame = false;
        }
        count++;
    }
    check(allSame && count == 38, "all 38 instructions survive disassemble -> assemble unchanged");
}

void testRunsOnCpu()
{
    cout << "--Assembled programs run on the real CPU--\n";
    {
        Machine m;
        bool ok = m.run(
            "; add 1 + 2 + ... + 10 into R0\n"
            "    LDI R0, 0      ; total\n"
            "    LDI R1, 10     ; counter\n"
            "loop:\n"
            "    ADD R0, R1\n"
            "    DEC R1\n"
            "    JNZ loop\n"
            "    HLT\n");
        check(ok && m.cpu.getReg(0) == 55, "sum 1..10 with a loop gives 55");
    }
    {
        Machine m;
        bool ok = m.run(
            "    LDI R0, 7\n"
            "    CALL double\n"
            "    CALL double\n"
            "    HLT\n"
            "double:\n"
            "    ADD R0, R0\n"
            "    RET\n");
        check(ok && m.cpu.getReg(0) == 28 && m.cpu.getSP() == 0xFFFF, "CALL / RET subroutine: 7 doubled twice is 28, stack empty again");
    }
    {
        Machine m;
        bool ok = m.run(
            "    LDI R0, 0x99\n"
            "    ST R0, [0x8000]   ; RAM\n"
            "    LDI R0, 0\n"
            "    LD R1, [0x8000]\n"
            "    LDI R6, 0xE0      ; P = 0xE000, start of VRAM\n"
            "    LDI R7, 0x00\n"
            "    LDI R2, 1\n"
            "    STP R2            ; first pixel on\n"
            "    INP\n"
            "    STP R2            ; second pixel on\n"
            "    HLT\n");
        check(ok && m.cpu.getReg(1) == 0x99, "ST / LD through RAM");
        check(ok && m.bus.read(0xE000) == 1 && m.bus.read(0xE001) == 1 && m.bus.read(0xE002) == 0,
              "STP / INP draw two pixels in VRAM");
    }
    {
        Machine m;
        bool ok = m.run(
            "    LDI R0, 200\n"
            "    ADDI R0, 100   ; 300 does not fit: carry\n"
            "    JC carried\n"
            "    HLT\n"
            "carried:\n"
            "    LDI R3, 1\n"
            "    HLT\n");
        check(ok && m.cpu.getReg(3) == 1 && m.cpu.getReg(0) == 44, "ADDI sets carry and JC follows it (200 + 100 = 44 + carry)");
    }
}

int main()
{
    testEncoding();
    testLabels();
    testErrors();
    testRoundTrip();
    testRunsOnCpu();

    cout << '\n' << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
