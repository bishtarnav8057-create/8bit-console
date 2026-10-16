#include "Parser.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace
{
    bool notSpace(unsigned char ch)
    {
        return !std::isspace(ch);
    }

    // Remove leading and trailing whitespace.
    std::string trim(const std::string& text)
    {
        std::string result = text;

        result.erase(
            result.begin(),
            std::find_if(result.begin(), result.end(), notSpace)
        );

        result.erase(
            std::find_if(result.rbegin(), result.rend(), notSpace).base(),
            result.end()
        );

        return result;
    }

    // "[ 0x8000 ]" -> "[0x8000]", so memory operands look the same however they are typed.
    std::string tidyBrackets(const std::string& operand)
    {
        if (operand.size() < 2 || operand[0] != '[' || operand[operand.size() - 1] != ']')
        {
            return operand;
        }

        return "[" + trim(operand.substr(1, operand.size() - 2)) + "]";
    }

    // Parses one line and adds what it finds (a label, an instruction, or both) to instructions.
    void parseLine(std::string line, int lineNumber, std::vector<Instruction>& instructions)
    {
        // Remove comments beginning with ';'.
        size_t commentPosition = line.find(';');

        if (commentPosition != std::string::npos)
        {
            line = line.substr(0, commentPosition);
        }

        line = trim(line);

        // Ignore empty lines and comment-only lines.
        if (line.empty())
        {
            return;
        }

        std::stringstream ss(line);

        Instruction instruction;
        instruction.lineNumber = lineNumber;

        // Extract the instruction name.
        ss >> instruction.name;

        // "loop: DEC R0" is a label followed by an instruction on the same line.
        // Store the label on its own, then parse the rest of the line as normal.
        if (instruction.name.size() > 1 &&
            instruction.name[instruction.name.size() - 1] == ':')
        {
            instructions.push_back(instruction);

            std::string rest;
            std::getline(ss, rest);
            parseLine(rest, lineNumber, instructions);
            return;
        }

        // Get the remaining text containing operands.
        std::string operandText;
        std::getline(ss, operandText);
        operandText = trim(operandText);

        // Split operands using commas. Empty operands ("ADD R0,, R1") are kept
        // so the operand checker reports them instead of them silently disappearing.
        if (!operandText.empty())
        {
            std::stringstream operandStream(operandText);
            std::string operand;

            while (std::getline(operandStream, operand, ','))
            {
                instruction.operands.push_back(tidyBrackets(trim(operand)));
            }

            // getline does not return a final empty piece for a trailing comma ("ADD R0, R1,").
            if (operandText[operandText.size() - 1] == ',')
            {
                instruction.operands.push_back("");
            }
        }

        // Store the parsed instruction.
        instructions.push_back(instruction);
    }

    std::vector<Instruction> parseStream(std::istream& input)
    {
        std::vector<Instruction> instructions;

        std::string line;
        int lineNumber = 0;

        // Read one line at a time.
        while (std::getline(input, line))
        {
            lineNumber++;
            parseLine(line, lineNumber, instructions);
        }

        return instructions;
    }
}

std::vector<Instruction> Parser::parseFile(
    const std::string& filename)
{
    std::ifstream file(filename);

    // Check whether the file opened successfully.
    if (!file.is_open())
    {
        throw std::runtime_error(
            "Could not open file: " + filename
        );
    }

    return parseStream(file);
}

std::vector<Instruction> Parser::parseText(
    const std::string& text)
{
    std::istringstream input(text);
    return parseStream(input);
}

