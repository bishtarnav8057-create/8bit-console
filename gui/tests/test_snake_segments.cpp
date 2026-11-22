// Checks SnakeSegments::at without Qt: straight parts, the 4 corners, head and tail in 4 directions.
#include <cstdio>
#include <vector>

#include "../src/model/SnakeSegments.h"

using namespace SnakeSegments;

static int passed = 0, failed = 0;

static void check(bool ok, const char* what)
{
    if (ok) { passed++; std::printf("  PASS  %s\n", what); }
    else { failed++; std::printf("  FAIL  %s\n", what); }
}

static bool is(Segment s, Kind k, int r) { return s.kind == k && s.rotation == r; }

int main()
{
    // head first: moving right along y = 5
    std::vector<Games::Point> right = {{5, 5}, {4, 5}, {3, 5}};
    check(is(at(right, 0), Head, 0), "head facing right");
    check(is(at(right, 1), Body, 0), "horizontal body");
    check(is(at(right, 2), Tail, 2), "tail pointing left when moving right");

    std::vector<Games::Point> down = {{5, 6}, {5, 5}, {5, 4}};
    check(is(at(down, 0), Head, 1), "head facing down");
    check(is(at(down, 1), Body, 1), "vertical body");
    check(is(at(down, 2), Tail, 3), "tail pointing up when moving down");

    std::vector<Games::Point> left = {{3, 5}, {4, 5}};
    check(is(at(left, 0), Head, 2), "head facing left");
    check(is(at(left, 1), Tail, 0), "tail pointing right when moving left");

    std::vector<Games::Point> up = {{5, 4}, {5, 5}};
    check(is(at(up, 0), Head, 3), "head facing up");
    check(is(at(up, 1), Tail, 1), "tail pointing down when moving up");

    // corners: the middle cell joins two neighbours
    // left + down = rotation 0
    check(is(at({{5, 6}, {5, 5}, {4, 5}}, 1), Corner, 0), "corner left+down");
    // up + left = rotation 1 (left+down turned once clockwise)
    check(is(at({{5, 4}, {5, 5}, {4, 5}}, 1), Corner, 1), "corner left+up");
    // right + up = rotation 2
    check(is(at({{6, 5}, {5, 5}, {5, 4}}, 1), Corner, 2), "corner up+right");
    // down + right = rotation 3
    check(is(at({{6, 5}, {5, 5}, {5, 6}}, 1), Corner, 3), "corner right+down");
    // order of the neighbours does not matter
    check(is(at({{4, 5}, {5, 5}, {5, 6}}, 1), Corner, 0), "corner down+left (other order)");

    // the start snake of Games::Snake: length 3, moving right
    Games::Snake snake;
    const std::vector<Games::Point>& b = snake.body();
    check(b.size() == 3 && is(at(b, 0), Head, 0) && is(at(b, 1), Body, 0) && is(at(b, 2), Tail, 2),
          "start snake: head right, body, tail");

    std::printf("Result: %d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
