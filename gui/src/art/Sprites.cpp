#include "art/Sprites.h"

namespace
{
    // Order must match SpriteId.
    const Sprite SPRITES[] = {
        // Logo: a chip with an "8"
        {16, 16, {
            "..p.p.p.p.p.p...",
            ".mmmmmmmmmmmmm..",
            "pmkkkkkkkkkkkmp.",
            ".mkkkggggkkkkm..",
            "pmkkgkkkkgkkkmp.",
            ".mkkgkkkkgkkkm..",
            "pmkkkggggkkkkmp.",
            ".mkkgkkkkgkkkm..",
            "pmkkgkkkkgkkkmp.",
            ".mkkgkkkkgkkkm..",
            "pmkkkggggkkkkmp.",
            ".mkkkkkkkkkkkm..",
            "pmkkkkkkkkkkamp.",
            ".mmmmmmmmmmmmm..",
            "..p.p.p.p.p.p...",
            "................"}},
        // Calc
        {16, 16, {
            "..mmmmmmmmmmmm..",
            ".mkkkkkkkkkkkkm.",
            ".mknnnnnnnnnnkm.",
            ".mkngggngggnnkm.",
            ".mknnnnnnnnnnkm.",
            ".mkkkkkkkkkkkkm.",
            ".mkwwkwwkwwkaam.",
            ".mkwwkwwkwwkaam.",
            ".mkkkkkkkkkkkkm.",
            ".mkwwkwwkwwkaam.",
            ".mkwwkwwkwwkaam.",
            ".mkkkkkkkkkkkkm.",
            ".mkwwkwwkwwkrrm.",
            ".mkwwkwwkwwkrrm.",
            ".mkkkkkkkkkkkkm.",
            "..mmmmmmmmmmmm.."}},
        // ConsoleIcon: a monitor with one lit pixel and a keyboard
        {16, 16, {
            "................",
            ".mmmmmmmmmmmmmm.",
            ".mkkkkkkkkkkkkm.",
            ".mknnnnnnnnnnkm.",
            ".mknnnnnnnnnnkm.",
            ".mknnnngnnnnnkm.",
            ".mknnnnnnnnnnkm.",
            ".mknnnnnnnnnnkm.",
            ".mkkkkkkkkkkkkm.",
            ".mmmmmmmmmmmmmm.",
            ".....mmmmmm.....",
            "..mmmmmmmmmmmm..",
            "..mkgkkkkkakrm..",
            "..mgggkkkkkkkm..",
            "..mkgkkkkkkakm..",
            "..mmmmmmmmmmmm.."}},
        // SnakeIcon
        {16, 16, {
            "................",
            "................",
            "...GGGGGGGGGG...",
            "...HHHHHHHHHHG..",
            "...GGGGGGGGGGG..",
            "...........GGG..",
            "...CC......GHG..",
            "...CC......GHG..",
            "...........GHG..",
            "...........GHG..",
            "..GGGGGGGGGGHG..",
            ".rGWKHHHHHHHHG..",
            "..GGGGGGGGGGGG..",
            "................",
            "................",
            "................"}},
        // PongIcon
        {16, 16, {
            "................",
            "................",
            ".PP.............",
            ".PP....s........",
            ".PP.............",
            ".PQ....s........",
            ".QQ.........UU..",
            ".......s....UU..",
            ".........BB.UU..",
            ".......s.BB.UU..",
            "............UV..",
            ".......s....VV..",
            "................",
            ".......s........",
            "................",
            "................"}},
        // SnakeHead (facing right)
        {8, 8, {
            "oooooo..",
            "HHHHHHo.",
            "GGGGWKGo",
            "GGGGGGGo",
            "GdGGGGGo",
            "GGGGWKGo",
            "GGGGGGo.",
            "oooooo.."}},
        // SnakeBody (horizontal)
        {8, 8, {
            "oooooooo",
            "HHHHHHHH",
            "GGGGGGGG",
            "GGdGGGdG",
            "GdGGGdGG",
            "GGGGGGGG",
            "GGGGGGGG",
            "oooooooo"}},
        // SnakeCorner (joins left and down)
        {8, 8, {
            "ooooooo.",
            "HHHHHHHo",
            "GGGGGGGo",
            "GGdGGGGo",
            "GGGGGdGo",
            "GGGGGGGo",
            "GGGGGGGo",
            "oGGGGGGo"}},
        // SnakeTail (tip on the right)
        {8, 8, {
            "oooo....",
            "HHHHoo..",
            "GGGGGGo.",
            "GGdGGGGo",
            "GdGGGGGo",
            "GGGGGGo.",
            "GGGGoo..",
            "oooo...."}},
        // Chip: the food, a byte with pins
        {8, 8, {
            "..p..p..",
            ".CCCCCC.",
            "pCLLLLCp",
            ".CLccLC.",
            ".CLccLC.",
            "pCLLLLCp",
            ".CCCCCC.",
            "..p..p.."}},
        // Ball
        {8, 8, {
            "..xxxx..",
            ".xBBBBx.",
            "xBBBBBsx",
            "xBBBBBsx",
            "xBBBBssx",
            "xBBBsssx",
            ".xssssx.",
            "..xxxx.."}},
        // PadTop
        {8, 8, {
            ".PPPPPP.",
            "PPPPPPPP",
            "PPPPPPQQ",
            "PPPPPPQQ",
            "PPPPPPQQ",
            "PPPPPPQQ",
            "PPPPPPQQ",
            "PPPPPPQQ"}},
        // PadMid
        {8, 8, {
            "PPPPPPQQ",
            "PPPPPPQQ",
            "PPPPPPQQ",
            "PPPPPPQQ",
            "PPPPPPQQ",
            "PPPPPPQQ",
            "PPPPPPQQ",
            "PPPPPPQQ"}},
        // PadBot
        {8, 8, {
            "PPPPPPQQ",
            "PPPPPPQQ",
            "PPPPPPQQ",
            "PPPPPPQQ",
            "PPPPPPQQ",
            "PPPPPQQQ",
            "QQQQQQQQ",
            ".QQQQQQ."}},
        // Back
        {8, 8, {
            "....w...",
            "...ww...",
            "..www...",
            ".wwwwwww",
            ".wwwwwww",
            "..www...",
            "...ww...",
            "....w..."}},
        // Play
        {8, 8, {
            ".w......",
            ".www....",
            ".wwwww..",
            ".wwwwwww",
            ".wwwwwww",
            ".wwwww..",
            ".www....",
            ".w......"}},
        // Pause
        {8, 8, {
            "........",
            ".ww..ww.",
            ".ww..ww.",
            ".ww..ww.",
            ".ww..ww.",
            ".ww..ww.",
            ".ww..ww.",
            "........"}},
        // Reset
        {8, 8, {
            "w.wwww..",
            "ww....w.",
            "www....w",
            ".......w",
            "w......w",
            "w......w",
            ".w....w.",
            "..wwww.."}},
        // Power
        {8, 8, {
            "...ww...",
            ".w.ww.w.",
            "w..ww..w",
            "w..ww..w",
            "w......w",
            "w......w",
            ".w....w.",
            "..wwww.."}},
        // Enter
        {8, 8, {
            "......ww",
            "......ww",
            "......ww",
            "..w...ww",
            ".ww...ww",
            "wwwwwwww",
            ".ww.....",
            "..w....."}},
        // Arrow (up)
        {8, 8, {
            "...ww...",
            "..wwww..",
            ".wwwwww.",
            "wwwwwwww",
            "...ww...",
            "...ww...",
            "...ww...",
            "...ww..."}},
        // Cartridge
        {12, 12, {
            ".mmmmmmmmmm.",
            ".mkkkkkkkkm.",
            ".mknnnnnnkm.",
            ".mkngggnnkm.",
            ".mknnnnnnkm.",
            ".mkkkkkkkkm.",
            ".mkkkkkkkkm.",
            ".mkaaaaaakm.",
            ".mkkkkkkkkm.",
            ".mmmmmmmmmm.",
            "..pp.pp.pp..",
            "............"}},
        // ChipQuestion: calc.asm not found
        {16, 16, {
            "..p.p.p.p.p.p...",
            ".mmmmmmmmmmmmm..",
            "pmkkkkkkkkkkkmp.",
            ".mkkkkaaaakkkm..",
            "pmkkkakkkkakkmp.",
            ".mkkkkkkkkakkm..",
            "pmkkkkkkkakkkmp.",
            ".mkkkkkkaakkkm..",
            "pmkkkkkkakkkkmp.",
            ".mkkkkkkakkkkm..",
            "pmkkkkkkkkkkkmp.",
            ".mkkkkkkakkkkm..",
            "pmkkkkkkkkkkkmp.",
            ".mmmmmmmmmmmmm..",
            "..p.p.p.p.p.p...",
            "................"}},
        // CartridgeX: the program has errors
        {16, 16, {
            "................",
            "..mmmmmmmmmmmm..",
            "..mkkkkkkkkkkm..",
            "..mknnnnnnnnkm..",
            "..mknrnnnnrnkm..",
            "..mknnrnnrnnkm..",
            "..mknnnrrnnnkm..",
            "..mknnnrrnnnkm..",
            "..mknnrnnrnnkm..",
            "..mknrnnnnrnkm..",
            "..mknnnnnnnnkm..",
            "..mkkkkkkkkkkm..",
            "..mkkaaaaaakkm..",
            "..mmmmmmmmmmmm..",
            "...pp.pp.pp.pp..",
            "................"}},
        // SleepScreen: the program prints nothing
        {16, 16, {
            "................",
            ".mmmmmmmmmmmmmm.",
            ".mkkkkkkkkkkkkm.",
            ".mknnnnnnnnnnkm.",
            ".mknnnnnnsssnkm.",
            ".mknnnnnnnsnnkm.",
            ".mknnnnnnsssnkm.",
            ".mknssnnnnnnnkm.",
            ".mknnsnnnnnnnkm.",
            ".mknssnnnnnnnkm.",
            ".mkkkkkkkkkkkkm.",
            ".mmmmmmmmmmmmmm.",
            ".....mmmmmm.....",
            "..mmmmmmmmmmmm..",
            "................",
            "................"}},
    };

    static_assert(sizeof(SPRITES) / sizeof(SPRITES[0]) == (int)SpriteId::Count, "one grid per SpriteId");
}

const Sprite& sprite(SpriteId id)
{
    return SPRITES[(int)id];
}

long paletteRgb(char c)
{
    switch (c)
    {
    case 'o': return 0x174D2B;   // snake outline
    case 'G': return 0x4FC46E;   // snake body
    case 'H': return 0x9BF5AE;   // snake highlight
    case 'd': return 0x2E8A4C;   // snake scales
    case 'W': return 0xECE8D8;   // eye white
    case 'K': return 0x0B0C14;   // pupil
    case 'p': return 0xA9ADC6;   // chip pins
    case 'C': return 0xFFB547;   // chip body
    case 'L': return 0xB8741A;   // chip dark
    case 'c': return 0xFFE3A3;   // chip highlight
    case 'B': return 0xECE8D8;   // ball
    case 's': return 0xA9ADC6;   // ball shade, net
    case 'x': return 0x5A5F7A;   // ball outline
    case 'P': return 0x7DF09B;   // YOU paddle
    case 'Q': return 0x3FA765;   // YOU paddle shade
    case 'U': return 0x62D2FF;   // COM paddle
    case 'V': return 0x2C8FBF;   // COM paddle shade
    case 'k': return 0x242840;   // icon body
    case 'm': return 0x454C73;   // icon rim
    case 'n': return 0x12141F;   // icon screen
    case 'g': return 0x7DF09B;   // accent green
    case 'a': return 0xFFB547;   // accent amber
    case 'r': return 0xFF6B78;   // accent red
    case 'w': return 0xECE8D8;   // glyphs, accent white
    case 'y': return 0x80859F;   // dead snake body
    default:  return -1;
    }
}

char swapChar(char c, Swap swap)
{
    switch (swap)
    {
    case Swap::Com:
        return c == 'P' ? 'U' : c == 'Q' ? 'V' : c;
    case Swap::Dead:
        return c == 'G' ? 'y' : c == 'H' ? 's' : c == 'd' ? 'x' : c == 'o' ? 'm' : c;
    case Swap::Dim:
        return c == 'c' ? 'C' : c;
    default:
        return c;
    }
}
