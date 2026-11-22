// AsmNotes.h - reads the plain-English notes out of a .asm file (no Qt), for the Console's CODE view.
#ifndef ASMNOTES_H
#define ASMNOTES_H

#include <array>
#include <string>
#include <vector>

namespace AsmNotes
{
    // From the first comment line with "Registers:", e.g. "; Registers: R0 = total, R1 = counter".
    // names[n] is the name of Rn, "" if the file does not name it.
    std::array<std::string, 8> registerNames(const std::vector<std::string>& lines);

    std::string comment(const std::string& line);   // text after the first ';', trimmed; "" if none
    std::string code(const std::string& line);      // text before the first ';', trimmed
}

#endif
