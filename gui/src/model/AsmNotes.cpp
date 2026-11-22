#include "model/AsmNotes.h"

namespace
{
    std::string trim(const std::string& s)
    {
        const size_t a = s.find_first_not_of(" \t\r\n");
        if (a == std::string::npos) return "";
        return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
    }
}

namespace AsmNotes
{
    std::array<std::string, 8> registerNames(const std::vector<std::string>& lines)
    {
        std::array<std::string, 8> names;
        for (const std::string& line : lines)
        {
            const std::string t = trim(line);
            const size_t at = t.find("Registers:");
            if (t.empty() || t[0] != ';' || at == std::string::npos) continue;

            std::string rest = t.substr(at + 10);
            size_t start = 0;
            while (start <= rest.size())
            {
                size_t comma = rest.find(',', start);
                if (comma == std::string::npos) comma = rest.size();
                const std::string piece = trim(rest.substr(start, comma - start));   // "R0 = total"
                const size_t eq = piece.find('=');
                if (piece.size() >= 2 && (piece[0] == 'R' || piece[0] == 'r') && piece[1] >= '0' && piece[1] <= '7'
                    && eq != std::string::npos && trim(piece.substr(2, eq - 2)).empty())
                    names[piece[1] - '0'] = trim(piece.substr(eq + 1));
                start = comma + 1;
            }
            break;   // only the first Registers: line counts
        }
        return names;
    }

    std::string comment(const std::string& line)
    {
        const size_t semi = line.find(';');
        return semi == std::string::npos ? "" : trim(line.substr(semi + 1));
    }

    std::string code(const std::string& line)
    {
        return trim(line.substr(0, line.find(';')));
    }
}
