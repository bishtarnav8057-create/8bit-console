#include "model/SnakeSegments.h"

namespace SnakeSegments
{
    namespace
    {
        struct Vec { int dx, dy; };

        bool same(Vec a, Vec b) { return a.dx == b.dx && a.dy == b.dy; }

        Vec between(const Games::Point& from, const Games::Point& to)
        {
            return {to.x - from.x, to.y - from.y};
        }

        // right 0, down 1, left 2, up 3 (clockwise on screen, where y grows downwards)
        int rotationOf(Vec v)
        {
            if (v.dx > 0) return 0;
            if (v.dy > 0) return 1;
            if (v.dx < 0) return 2;
            return 3;
        }

        Vec turn(Vec v) { return {-v.dy, v.dx}; }   // 90 degrees clockwise
    }

    Segment at(const std::vector<Games::Point>& body, int i)
    {
        const int n = (int)body.size();
        if (n < 2) return {Head, 0};
        if (i == 0) return {Head, rotationOf(between(body[1], body[0]))};
        if (i == n - 1) return {Tail, rotationOf(between(body[n - 2], body[n - 1]))};

        Vec a = between(body[i], body[i - 1]);
        Vec b = between(body[i], body[i + 1]);
        if (a.dx == -b.dx && a.dy == -b.dy) return {Body, a.dx != 0 ? 0 : 1};

        Vec p = {-1, 0}, q = {0, 1};   // the corner as drawn joins left and down
        for (int r = 0; r < 4; r++)
        {
            if ((same(p, a) && same(q, b)) || (same(p, b) && same(q, a))) return {Corner, r};
            p = turn(p);
            q = turn(q);
        }
        return {Body, 0};   // not neighbours (cannot happen with the game rules)
    }
}
