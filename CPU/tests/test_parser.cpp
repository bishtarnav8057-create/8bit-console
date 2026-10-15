#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "../src/Parser.h"

using namespace std;

int main()
{
    // Create a sample assembly program.
    ofstream file("program.asm");

    if (!file.is_open())
    {
        cout << "FAIL: Could not create test file\n";
        return 1;
    }

    file << "; Sample program\n";
    file << "\n";
    file << "MOV A, 5\n";
    file << "MOV B, 3\n";
    file << "ADD A, B\n";
    file << "HLT\n";

    file.close();

    try
    {
        Parser parser;

        vector<Instruction> instructions =
            parser.parseFile("program.asm");

        // Check the number of instructions.
        if (instructions.size() != 4)
        {
            cout << "FAIL: Incorrect instruction count\n";
            return 1;
        }

        // Check the first instruction.
        if (instructions[0].name != "MOV")
        {
            cout << "FAIL: Incorrect instruction name\n";
            return 1;
        }

        // Check the operands.
        if (instructions[0].operands.size() != 2 ||
            instructions[0].operands[0] != "A" ||
            instructions[0].operands[1] != "5")
        {
            cout << "FAIL: Incorrect operands\n";
            return 1;
        }

        // Check the ADD instruction.
        if (instructions[2].name != "ADD")
        {
            cout << "FAIL: ADD instruction not parsed\n";
            return 1;
        }

        // Check the HLT instruction.
        if (instructions[3].name != "HLT")
        {
            cout << "FAIL: HLT instruction not parsed\n";
            return 1;
        }

        cout << "PASS: Parser test completed successfully\n";
    }
    catch (const exception& e)
    {
        cout << "FAIL: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
