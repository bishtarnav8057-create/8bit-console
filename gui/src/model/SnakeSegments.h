// SnakeSegments.h - which tile each snake cell uses and how it is turned. No Qt, tested by
// tests/test_snake_segments.cpp. Rotation r = r * 90 degrees clockwise from the tile as drawn
// (head/tail facing right, body horizontal, corner joining left and down).
#ifndef SNAKESEGMENTS_H
#define SNAKESEGMENTS_H

#include <vector>

#include "Games.h"

namespace SnakeSegments
{
    enum Kind { Head, Body, Corner, Tail };

    struct Segment
    {
        Kind kind;
        int rotation;   // 0..3
    };

    Segment at(const std::vector<Games::Point>& body, int i);   // body[0] is the head
}

#endif
