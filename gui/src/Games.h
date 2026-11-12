// Games.h - the rules of Snake and Pong. No Qt in here, so it can be tested with plain g++.
// The rules are ported from pixel/pixel8_arcade.cpp (same moves, same scoring).
#ifndef GAMES_H
#define GAMES_H

#include <vector>

namespace Games
{
    const int W = 30;          // the board is 30 cells wide and 20 cells high
    const int H = 20;

    enum Dir { UP = 0, RIGHT = 1, DOWN = 2, LEFT = 3 };

    struct Point { int x; int y; };

    // ---------------------------------------------------------------- Snake
    class Snake
    {
    public:
        Snake();
        void reset();
        void turn(int dir);              // ignored if it is a 180 degree turn
        void step();                     // move one cell
        bool dead() const { return isDead; }
        int  score() const { return points; }
        const std::vector<Point>& body() const { return cells; }   // cells[0] is the head
        Point food() const { return foodAt; }

    private:
        void placeFood();
        bool onSnake(int x, int y) const;

        std::vector<Point> cells;
        Point foodAt;
        int dir, nextDir, points;
        bool isDead;
    };

    // ----------------------------------------------------------------- Pong
    class Pong
    {
    public:
        static const int PADDLE = 5;     // paddle height in cells
        static const int WIN = 5;        // first to 5 wins

        Pong();
        void reset();
        void movePlayer(int dy);         // -1 up, +1 down
        void cpuMove(int tick);          // the CPU paddle follows the ball, but a little slowly
        void step();                     // move the ball one cell

        int ballX() const { return bx; }
        int ballY() const { return by; }
        int playerY() const { return py; }
        int cpuY() const { return cy; }
        int playerScore() const { return ps; }
        int cpuScore() const { return cs; }
        bool over() const { return finished; }

    private:
        void serve(int dir);
        static int clampPaddle(int y);

        int bx, by, vx, vy, py, cy, ps, cs;
        bool finished;
    };
}

#endif
