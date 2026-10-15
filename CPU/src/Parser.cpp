#include "Parser.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>

std::vector<Instruction> Parser::parseFile(
    const std::string& filename)
{
    std::vector<Instruction> instructions;

    std::ifstream file(filename);

    // Check whether the file opened successfully.
    if (!file.is_open())
    {
        throw std::runtime_error(
            "Could not open file: " + filename
        );
    }

    std::string line;
    int lineNumber = 0;

    // Read the file one line at a time.
    while (std::getline(file, line))
    {
        lineNumber++;

        // Remove comments beginning with ';'.
        size_t commentPosition = line.find(';');

        if (commentPosition != std::string::npos)
        {
            line = line.substr(0, commentPosition);
        }

        // Remove leading and trailing whitespace.
        auto notSpace = [](unsigned char ch)
        {
            return !std::isspace(ch);
        };

        line.erase(
            line.begin(),
            std::find_if(
                line.begin(),
                line.end(),
                notSpace
            )
        );

        line.erase(
            std::find_if(
                line.rbegin(),
                line.rend(),
                notSpace
            ).base(),
            line.end()
        );

        // Ignore empty lines and comment-only lines.
        if (line.empty())
        {
            continue;
        }

        std::stringstream ss(line);

        Instruction instruction;
        instruction.lineNumber = lineNumber;

        // Extract the instruction name.
        ss >> instruction.name;

        // Get the remaining text containing operands.
        std::string operandText;
        std::getline(ss, operandText);

        // Split operands using commas.
        std::stringstream operandStream(operandText);
        std::string operand;

        while (std::getline(operandStream, operand, ','))
        {
            // Remove whitespace around each operand.
            operand.erase(
                operand.begin(),
                std::find_if(
                    operand.begin(),
                    operand.end(),
                    notSpace
                )
            );

            operand.erase(
                std::find_if(
                    operand.rbegin(),
                    operand.rend(),
                    notSpace
                ).base(),
                operand.end()
            );

            if (!operand.empty())
            {
                instruction.operands.push_back(operand);
            }
        }

        // Store the parsed instruction.
        instructions.push_back(instruction);
    }

    return instructions;
}
