// Tests for the assembly parser, using the CPU's real syntax (CPU/Aashish/src/Opcodes.h).
// Build from the CPU folder:
//   g++ -std=c++11 -Wall -Wextra tests/test_parser.cpp src/Parser.cpp -o test_parser.exe

#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../src/Parser.h"

using namespace std;

int passed = 0;
int failed = 0;

void check(bool condition, const string& name)
{
    cout << (condition ? "[PASS] " : "[FAIL] ") << name << '\n';
    condition ? passed++ : failed++;
}

bool sameOperands(const Instruction& inst, const vector<string>& expected)
{
    return inst.operands == expected;
}

void testFile()
{
    cout << "--Reading a file--\n";

    ofstream file("program.asm");
    file << "; Sample program\n";
    file << "\n";
    file << "LDI R0, 5\n";
    file << "LDI R1, 3\n";
    file << "ADD R0, R1\n";
    file << "HLT\n";
    file.close();

    Parser parser;
    vector<Instruction> instructions = parser.parseFile("program.asm");
    remove("program.asm");

    check(instructions.size() == 4, "comments and blank lines are skipped, 4 instructions left");
    if (instructions.size() != 4) return;

    check(instructions[0].name == "LDI", "first instruction is LDI");
    check(sameOperands(instructions[0], {"R0", "5"}), "LDI R0, 5 has operands R0 and 5");
    check(instructions[0].lineNumber == 3, "LDI is on line 3");
    check(instructions[2].name == "ADD" && sameOperands(instructions[2], {"R0", "R1"}), "ADD R0, R1");
    check(instructions[3].name == "HLT" && instructions[3].operands.empty(), "HLT has no operands");
    check(instructions[3].lineNumber == 6, "HLT is on line 6");
}

void testMissingFile()
{
    cout << "--Missing file--\n";

    Parser parser;
    bool threw = false;
    try
    {
        parser.parseFile("this_file_does_not_exist.asm");
    }
    catch (const runtime_error&)
    {
        threw = true;
    }
    check(threw, "a missing file throws runtime_error");
}

void testSpacingAndComments()
{
    cout << "--Spacing and comments--\n";

    Parser parser;
    vector<Instruction> instructions = parser.parseText(
        "   \tINC   R1   ; count up\n"
        "MOV R2,R3\n"
        "  ; only a comment\n"
        "\n");

    check(instructions.size() == 2, "comment-only and blank lines are skipped");
    if (instructions.size() != 2) return;
    check(instructions[0].name == "INC" && sameOperands(instructions[0], {"R1"}), "tabs, extra spaces and comment removed");
    check(sameOperands(instructions[1], {"R2", "R3"}), "no space after the comma still works");
}

void testMemoryOperands()
{
    cout << "--Memory operands--\n";

    Parser parser;
    vector<Instruction> instructions = parser.parseText(
        "LD R0, [0x8000]\n"
        "ST R0, [ value ]\n");

    check(instructions.size() == 2, "two instructions");
    if (instructions.size() != 2) return;
    check(sameOperands(instructions[0], {"R0", "[0x8000]"}), "LD R0, [0x8000] keeps the brackets");
    check(sameOperands(instructions[1], {"R0", "[value]"}), "spaces inside brackets are removed");
}

void testLabels()
{
    cout << "--Labels--\n";

    Parser parser;
    vector<Instruction> instructions = parser.parseText(
        "start:\n"
        "loop: DEC R0   ; label and instruction on one line\n"
        "    JNZ loop\n");

    check(instructions.size() == 4, "label on its own line, and label + instruction split in two");
    if (instructions.size() != 4) return;
    check(instructions[0].name == "start:" && instructions[0].operands.empty(), "start: is a label with no operands");
    check(instructions[1].name == "loop:" && instructions[1].operands.empty(), "loop: is split off as its own label");
    check(instructions[2].name == "DEC" && sameOperands(instructions[2], {"R0"}), "DEC R0 after the label is parsed normally");
    check(instructions[1].lineNumber == 2 && instructions[2].lineNumber == 2, "both halves keep line number 2");
    check(sameOperands(instructions[3], {"loop"}), "JNZ loop uses the label as an operand");
}

void testBadCommasAreKept()
{
    cout << "--Bad commas are kept for the checker--\n";

    Parser parser;
    vector<Instruction> instructions = parser.parseText(
        "ADD R0,, R1\n"
        "ADD R0, R1,\n");

    check(instructions.size() == 2, "two instructions");
    if (instructions.size() != 2) return;
    check(sameOperands(instructions[0], {"R0", "", "R1"}), "ADD R0,, R1 keeps the empty operand");
    check(sameOperands(instructions[1], {"R0", "R1", ""}), "a trailing comma gives an empty last operand");
}

int main()
{
    testFile();
    testMissingFile();
    testSpacingAndComments();
    testMemoryOperands();
    testLabels();
    testBadCommasAreKept();

    cout << '\n' << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}

