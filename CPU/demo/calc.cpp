// PIXEL-8 CALCULATOR: every answer is worked out by our CPU running programs/calc.asm.
// Run from the CPU folder:
//   mingw32-make calc              type sums yourself
//   mingw32-make calc ARGS=demo    shows a fixed list of examples, Enter between each

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "../calc/Calculator.h"
#include "../src/OperandCheck.h"

using namespace std;

namespace
{
    string trim(const string& text)
    {
        size_t first = text.find_first_not_of(" \t\r\n");
        if (first == string::npos) return "";
        size_t last = text.find_last_not_of(" \t\r\n");
        return text.substr(first, last - first + 1);
    }

    // Input piped in from PowerShell can start with an invisible UTF-8 marker (EF BB BF).
    string withoutByteOrderMark(const string& text)
    {
        return text.compare(0, 3, "\xEF\xBB\xBF") == 0 ? text.substr(3) : text;
    }

    string padRight(const string& text, size_t width)
    {
        return text.size() >= width ? text + " " : text + string(width - text.size(), ' ');
    }

    string binary(Byte value)
    {
        string bits;
        for (int i = 7; i >= 0; i--) bits += ((value >> i) & 1) ? '1' : '0';
        return bits;
    }

    // Instructions that put a new value in their first register, so it is worth showing.
    bool changesFirstRegister(const string& mnemonic)
    {
        static const char* names[] = {"LD", "LDI", "MOV", "ADD", "SUB", "AND", "OR", "XOR", "INC", "DEC"};
        for (const char* name : names)
        {
            if (mnemonic == name) return true;
        }
        return false;
    }

    // One line of the "what the CPU did" list:  code   xN   comment   R0 = 42
    void printTraceLine(const Calculator& calc, const TraceLine& t, bool showCounts)
    {
        string text = calc.sourceLine(t.line);
        string comment;
        size_t semicolon = text.find(';');
        if (semicolon != string::npos)
        {
            comment = trim(text.substr(semicolon + 1));
            text = text.substr(0, semicolon);
        }
        string code = trim(text);

        string value;
        string mnemonic = code.substr(0, code.find(' '));
        size_t r = code.find('R', mnemonic.size());
        if (changesFirstRegister(mnemonic) && r != string::npos && r + 1 < code.size())
        {
            int index = code[r + 1] - '0';
            if (index >= 0 && index < CPU::NUM_REGS)
            {
                value = string("R") + code[r + 1] + " = " + to_string(t.registersAfter[index]);
            }
        }

        cout << "      " << padRight(code, 20);
        if (showCounts) cout << padRight("x" + to_string(t.count), 5);
        cout << padRight(comment, 44) << value << '\n';
    }

    void printBits(char op, Byte a, Byte b, Byte answer)
    {
        cout << "\n         " << binary(a) << "   (" << static_cast<int>(a) << ")\n"
             << "       " << op << " " << binary(b) << "   (" << static_cast<int>(b) << ")\n"
             << "       = " << binary(answer) << "   (" << static_cast<int>(answer) << ")\n";
    }

    void printFlags(Byte flags)
    {
        string set;
        if (flags & (1 << CPU::FLAG_Z)) set += " Z(zero)";
        if (flags & (1 << CPU::FLAG_C)) set += " C(carry/borrow)";
        if (flags & (1 << CPU::FLAG_N)) set += " N(negative)";
        if (flags & (1 << CPU::FLAG_V)) set += " V(overflow)";
        cout << "   flags set:" << (set.empty() ? " none" : set);
    }

    void calculate(Calculator& calc, char op, Byte a, Byte b)
    {
        CalcResult r = calc.run(op, a, b);
        if (!r.ok)
        {
            cout << "   Error: " << r.error << "\n\n";
            return;
        }

        if (op == '*') cout << "    The CPU has no multiply instruction, so it adds in a loop:\n";
        if (op == '/' && !r.divideByZero) cout << "    The CPU has no divide instruction, so it subtracts in a loop:\n";

        bool loops = false;
        for (const TraceLine& t : r.trace)
        {
            if (t.count > 1) loops = true;
        }
        for (const TraceLine& t : r.trace) printTraceLine(calc, t, loops);

        const int realAnswer = (op == '+') ? a + b : (op == '-') ? a - b : (op == '*') ? a * b : 0;

        if (op == '&' || op == '|' || op == '^') printBits(op, a, b, r.answer);

        cout << "\n   = ";
        if (r.divideByZero)
        {
            cout << "cannot divide by zero. The program checked for it and stopped safely.\n";
        }
        else
        {
            cout << static_cast<int>(r.answer);
            if (op == '/') cout << " remainder " << static_cast<int>(r.remainder);
            cout << '\n';
        }

        if (r.overflow && op == '+')
        {
            cout << "   !! CARRY: " << realAnswer << " does not fit in 8 bits (0-255), so it wrapped around:\n"
                 << "      " << realAnswer << " - 256 = " << static_cast<int>(r.answer)
                 << ". The CPU sets the C flag to say this happened.\n";
        }
        if (r.overflow && op == '-')
        {
            cout << "   !! BORROW: the answer went below zero. 8 bits store " << realAnswer << " as "
                 << static_cast<int>(r.answer) << " (256 - " << -realAnswer << ").\n"
                 << "      The CPU sets C (borrow) and N (negative) to say this happened.\n";
        }
        if (r.overflow && op == '*')
        {
            cout << "   !! TOO BIG: " << realAnswer << " does not fit in 8 bits (0-255), so only the low 8 bits\n"
                 << "      are kept. The program noticed the carry while adding and reported it.\n";
        }

        cout << "\n   " << r.instructions << " instructions run on the CPU.";
        if (op == '+' || op == '-') printFlags(r.flags);
        cout << "\n\n";
    }

    // "25 + 17", "25+17", "0x0F & 0b111": number, operator, number. False if it is not that.
    bool parseSum(const string& line, char& op, Byte& a, Byte& b, string& problem)
    {
        size_t at = string::npos;
        for (size_t i = 1; i < line.size(); i++)
        {
            if (Calculator::isOperator(line[i])) { at = i; break; }
        }
        if (at == string::npos)
        {
            problem = "Type a sum like 25 + 17  (operations: + - * / & | ^)";
            return false;
        }

        op = line[at];
        const string left = trim(line.substr(0, at));
        const string right = trim(line.substr(at + 1));

        unsigned long x = 0;
        unsigned long y = 0;
        if (!parseOperandNumber(left, x) || !parseOperandNumber(right, y))
        {
            problem = "Both sides must be whole numbers, like 25, 0x1F or 0b101";
            return false;
        }
        if (x > 255 || y > 255)
        {
            problem = "This is an 8-bit CPU: numbers must be 0-255";
            return false;
        }

        a = static_cast<Byte>(x);
        b = static_cast<Byte>(y);
        return true;
    }

    void printHelp()
    {
        cout << "   Type a sum and press Enter, for example:   25 + 17     200 + 100     7 * 6     50 / 7\n"
             << "   Operations: +  -  *  /  &  |  ^      Numbers: 0-255 (also 0x1F or 0b101)\n"
             << "   help = this text     demo = show the examples     q = quit\n\n";
    }

    void runDemo(Calculator& calc, bool pause)
    {
        struct Example { Byte a; char op; Byte b; const char* why; };
        const Example examples[] = {
            {25, '+', 17,  "a normal addition"},
            {50, '-', 8,   "a normal subtraction"},
            {200, '+', 100, "too big for 8 bits: watch the carry"},
            {5, '-', 9,    "going below zero"},
            {7, '*', 6,    "multiply, done with a loop of additions"},
            {20, '*', 20,  "a multiply that is too big"},
            {50, '/', 7,   "divide, done with a loop of subtractions"},
            {9, '/', 0,    "divide by zero"},
            {12, '&', 10,  "bitwise AND"},
            {12, '|', 10,  "bitwise OR"},
            {12, '^', 10,  "bitwise XOR"},
        };

        for (const Example& e : examples)
        {
            cout << " > " << static_cast<int>(e.a) << ' ' << e.op << ' ' << static_cast<int>(e.b)
                 << "        (" << e.why << ")\n";
            calculate(calc, e.op, e.a, e.b);
            if (pause)
            {
                cout << "   [Enter] next example";
                string ignored;
                if (!getline(cin, ignored)) return;
                cout << '\n';
            }
        }
    }
}

int main(int argc, char** argv)
{
    string file = "programs/calc.asm";
    bool demo = false;
    bool pause = true;
    for (int i = 1; i < argc; i++)
    {
        string arg = argv[i];
        if (arg == "demo") demo = true;
        else if (arg == "nopause") { demo = true; pause = false; }
        else file = arg;
    }

    cout << "\n =====================================================================\n"
         << "   PIXEL-8 CALCULATOR  -  every answer is worked out by our 8-bit CPU\n"
         << " =====================================================================\n\n";

    Calculator calc;
    if (!calc.load(file))
    {
        cout << "   Could not get " << file << " ready:\n";
        for (const string& e : calc.errors()) cout << "     " << e << '\n';
        cout << "\n   (Run this from the CPU folder, or give the path: calc.exe path/to/calc.asm)\n";
        return 1;
    }

    cout << "   " << file << "\n"
         << "     [ok] read into instructions     Parser        (Arnav)\n"
         << "     [ok] every operand checked      OperandCheck  (Piyush)\n"
         << "     [ok] " << padRight("turned into " + to_string(calc.programSize()) + " bytes", 27)
         << "Assembler     (Arnav)\n"
         << "     [ok] loaded into ROM            Console + Bus + CPU (Aashish)\n\n"
         << "   Your two numbers go into RAM over the Bus, the CPU runs the program,\n"
         << "   and the answer is read back out of RAM.\n\n";

    if (demo)
    {
        runDemo(calc, pause);
        return 0;
    }

    printHelp();
    string line;
    while (true)
    {
        cout << " > ";
        if (!getline(cin, line)) break;
        line = trim(withoutByteOrderMark(line));
        if (line.empty()) continue;
        if (line == "q" || line == "quit" || line == "exit") break;
        if (line == "help" || line == "?") { printHelp(); continue; }
        if (line == "demo") { runDemo(calc, true); continue; }

        char op = 0;
        Byte a = 0;
        Byte b = 0;
        string problem;
        if (!parseSum(line, op, a, b, problem))
        {
            cout << "   " << problem << "\n\n";
            continue;
        }
        calculate(calc, op, a, b);
    }
    return 0;
}
