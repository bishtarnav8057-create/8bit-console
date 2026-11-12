// Checks for Games.cpp (no window needed).   Build:  see gui/README.md
#include <cstdio>
#include <cstdlib>

#include "../src/Games.h"

using namespace Games;

static int passed = 0, failed = 0;
#define CHECK(name, cond) do { if (cond) { ++passed; std::printf("  PASS  %s\n", name); } \
                               else { ++failed; std::printf("  FAIL  %s\n", name); } } while (0)

int main()
{
    std::srand(1);

    std::printf("-- Snake --\n");
    Snake s;
    CHECK("starts with length 3, score 0", s.body().size() == 3 && s.score() == 0 && !s.dead());
    int x0 = s.body()[0].x;
    s.step();
    CHECK("moves right one cell", s.body()[0].x == x0 + 1 && s.body().size() == 3);
    s.turn(LEFT); s.step();
    CHECK("180 degree turn is ignored", s.body()[0].x == x0 + 2 && !s.dead());
    s.reset(); s.turn(UP); s.step();
    CHECK("turning up moves the head up", s.body()[0].y == H / 2 - 1);

    s.reset();
    // the food can be anywhere, so steer toward it until the snake eats it
    int guard = 0;
    while (s.score() == 0 && !s.dead() && guard++ < 500)
    {
        Point h = s.body()[0], f = s.food();
        if (h.x < f.x) s.turn(RIGHT); else if (h.x > f.x) s.turn(LEFT);
        else if (h.y < f.y) s.turn(DOWN); else s.turn(UP);
        s.step();
    }
    CHECK("eating food scores and grows", s.score() == 1 && s.body().size() == 4);

    s.reset();
    for (int i = 0; i < W + 2 && !s.dead(); i++) s.step();
    CHECK("hitting the wall kills the snake", s.dead());

    bool ok = true;
    s.reset();
    for (int i = 0; i < 20000; i++)
    {
        s.turn(std::rand() % 4);
        s.step();
        if (s.dead()) s.reset();
        ok = ok && s.body()[0].x >= 0 && s.body()[0].x < W && s.body()[0].y >= 0 && s.body()[0].y < H;
    }
    CHECK("20000 random steps stay on the board", ok);

    std::printf("-- Pong --\n");
    Pong p;
    CHECK("starts 0-0 and not over", p.playerScore() == 0 && p.cpuScore() == 0 && !p.over());
    p.movePlayer(-100);
    CHECK("paddle cannot leave the top", p.playerY() >= 0);
    for (int i = 0; i < 100; i++) p.movePlayer(1);
    CHECK("paddle cannot leave the bottom", p.playerY() == H - Pong::PADDLE);

    Pong q;                                   // the player never moves, so the CPU should score
    for (int i = 0; i < 20000 && !q.over(); i++) { q.cpuMove(i); q.step(); }
    CHECK("game ends when someone reaches 5", q.over() && (q.playerScore() == Pong::WIN || q.cpuScore() == Pong::WIN));

    ok = true;
    q.reset();
    for (int i = 0; i < 20000; i++)
    {
        q.cpuMove(i); q.step();
        if (q.over()) q.reset();
        ok = ok && q.ballX() >= 0 && q.ballX() < W && q.ballY() >= 0 && q.ballY() < H
                && q.cpuY() >= 0 && q.cpuY() <= H - Pong::PADDLE;
    }
    CHECK("20000 ticks keep ball and CPU paddle in bounds", ok);

    std::printf("\nResult: %d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
