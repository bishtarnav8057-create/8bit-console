// Checks AsmNotes without Qt: register names from the header comment, a line's comment and its code.
#include <cstdio>
#include <string>
#include <vector>

#include "../src/model/AsmNotes.h"

static int passed = 0, failed = 0;

static void check(bool ok, const char* what)
{
    if (ok) { passed++; std::printf("  PASS  %s\n", what); }
    else { failed++; std::printf("  FAIL  %s\n", what); }
}

int main()
{
    const auto sum = AsmNotes::registerNames({"; PIXEL-8 SUM", ";   0x8000 = the answer",
                                              "; Registers: R0 = total, R1 = counter", "", "start:"});
    check(sum[0] == "total", "sum: R0 = total");
    check(sum[1] == "counter", "sum: R1 = counter");
    check(sum[2].empty(), "sum: R2 has no name");

    const auto dot = AsmNotes::registerNames(
        {"; Registers: R0 = x, R1 = y, R2 = last frame seen, R3 = buttons, R4 = pixel colour, R5 = scratch"});
    check(dot[2] == "last frame seen", "dot: names keep their spaces");
    check(dot[5] == "scratch", "dot: R5 = scratch");
    check(dot[6].empty(), "dot: R6 has no name");

    bool allEmpty = true;
    for (const std::string& n : AsmNotes::registerNames({"start:", "    HLT"})) allEmpty &= n.empty();
    check(allEmpty, "no registers line: no names");

    const auto bad = AsmNotes::registerNames({"; Registers: R9 = bad, R1 = ok"});
    bool othersEmpty = true;
    for (int i = 0; i < 8; i++) if (i != 1) othersEmpty &= bad[i].empty();
    check(bad[1] == "ok" && othersEmpty, "R9 is ignored, R1 still read");

    check(AsmNotes::registerNames({"    NOP ; Registers: R0 = no"})[0].empty(), "Registers: after code is ignored");

    const std::string add = "    ADD  R0, R1         ; add the counter to the total";
    check(AsmNotes::comment(add) == "add the counter to the total", "comment after ;");
    check(AsmNotes::comment("loop:").empty(), "no comment on a label");
    check(AsmNotes::code(add) == "ADD  R0, R1", "code before ;");

    std::printf("%d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
