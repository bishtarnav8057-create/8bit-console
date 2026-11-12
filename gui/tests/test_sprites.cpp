// Checks every sprite grid without Qt: right size, only palette chars, swaps stay in the palette.
#include <cstdio>
#include <cstring>
#include <initializer_list>

#include "../src/art/Sprites.h"

static int passed = 0, failed = 0;

static void check(bool ok, const char* what, int id)
{
    if (ok) passed++;
    else { failed++; std::printf("  FAIL  sprite %d: %s\n", id, what); }
}

int main()
{
    for (int i = 0; i < (int)SpriteId::Count; i++)
    {
        const Sprite& s = sprite((SpriteId)i);
        bool sizeOk = (s.w == 8 && s.h == 8) || (s.w == 12 && s.h == 12) || (s.w == 16 && s.h == 16);
        check(sizeOk, "size is 8x8, 12x12 or 16x16", i);

        bool rowsOk = true, charsOk = true, swapsOk = true, anyPixel = false;
        for (int y = 0; y < s.h; y++)
        {
            if (!s.rows[y] || (int)std::strlen(s.rows[y]) != s.w) { rowsOk = false; continue; }
            for (int x = 0; x < s.w; x++)
            {
                char c = s.rows[y][x];
                if (c == '.') continue;
                anyPixel = true;
                if (paletteRgb(c) < 0) charsOk = false;
                for (Swap sw : {Swap::Com, Swap::Dead, Swap::Dim})
                    if (paletteRgb(swapChar(c, sw)) < 0) swapsOk = false;
            }
        }
        check(rowsOk, "every row is exactly w chars", i);
        check(charsOk, "only palette chars", i);
        check(swapsOk, "palette swaps stay in the palette", i);
        check(anyPixel, "not empty", i);
    }

    check(swapChar('P', Swap::Com) == 'U' && swapChar('Q', Swap::Com) == 'V', "COM swap", -1);
    check(swapChar('G', Swap::None) == 'G', "no swap keeps the char", -1);

    std::printf("Result: %d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
