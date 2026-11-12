// Sprites.h - every picture in the app as a text grid (one char = one pixel). No Qt in here, so a plain
// g++ test can check the grids (tests/test_sprites.cpp). SpriteCache turns them into pixmaps.
#ifndef SPRITES_H
#define SPRITES_H

#include <cstdint>

enum class SpriteId
{
    // 16 x 16 launcher icons
    Logo, Calc, ConsoleIcon, SnakeIcon, PongIcon,
    // 8 x 8 game tiles, drawn facing right / joining left
    SnakeHead, SnakeBody, SnakeCorner, SnakeTail, Chip, Ball, PadTop, PadMid, PadBot,
    // 8 x 8 UI glyphs, drawn in 'w' and tinted when painted
    Back, Play, Pause, Reset, Power, Enter, Arrow,      // Arrow points up; rotate for the others
    // 12 x 12 cartridge for the Console list
    Cartridge,
    // 16 x 16 empty-state pictures
    ChipQuestion, CartridgeX, SleepScreen,
    Count
};

struct Sprite
{
    int w, h;
    const char* rows[16];
};

const Sprite& sprite(SpriteId id);

// The colour of one palette char as 0xRRGGBB, or -1 for '.' (transparent) and unknown chars.
long paletteRgb(char c);

// Palette swaps: the same grid in other colours.
enum class Swap { None, Com, Dead, Dim };
char swapChar(char c, Swap swap);

#endif
