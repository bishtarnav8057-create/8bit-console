#include "Games.h"

#include <cstdlib>

namespace Games
{
    namespace
    {
        const int DX[4] = {0, 1, 0, -1};
        const int DY[4] = {-1, 0, 1, 0};
    }

    // ---------------------------------------------------------------- Snake
    Snake::Snake() { reset(); }

    void Snake::reset()
    {
        cells.clear();
        for (int i = 0; i < 3; i++)
        {
            Point p = {W / 2 - i, H / 2};
            cells.push_back(p);
        }
        dir = nextDir = RIGHT;
        points = 0;
        isDead = false;
        placeFood();
    }

    bool Snake::onSnake(int x, int y) const
    {
        for (size_t i = 0; i < cells.size(); i++)
        {
            if (cells[i].x == x && cells[i].y == y) return true;
        }
        return false;
    }

    void Snake::placeFood()
    {
        for (int tries = 0; tries < 1000; tries++)
        {
            int x = std::rand() % W;
            int y = std::rand() % H;
            if (!onSnake(x, y))
            {
                foodAt.x = x;
                foodAt.y = y;
                return;
            }
        }
        // the board is almost full: take the first free cell
        for (int y = 0; y < H; y++)
            for (int x = 0; x < W; x++)
                if (!onSnake(x, y)) { foodAt.x = x; foodAt.y = y; return; }
    }

    void Snake::turn(int d)
    {
        if (d < 0 || d > 3) return;
        if (d != (dir + 2) % 4) nextDir = d;
    }

    void Snake::step()
    {
        if (isDead) return;
        dir = nextDir;

        int nx = cells[0].x + DX[dir];
        int ny = cells[0].y + DY[dir];

        if (nx < 0 || nx >= W || ny < 0 || ny >= H)       // hit a wall
        {
            isDead = true;
            return;
        }

        bool grow = (nx == foodAt.x && ny == foodAt.y);

        // the tail moves away this step, so touching the last cell is safe unless we grow
        size_t check = grow ? cells.size() : cells.size() - 1;
        for (size_t i = 0; i < check; i++)
        {
            if (cells[i].x == nx && cells[i].y == ny)      // hit our own body
            {
                isDead = true;
                return;
            }
        }

        Point head = {nx, ny};
        cells.insert(cells.begin(), head);
        if (grow)
        {
            points++;
            placeFood();
        }
        else
        {
            cells.pop_back();
        }
    }

    // ----------------------------------------------------------------- Pong
    Pong::Pong() { reset(); }

    int Pong::clampPaddle(int y)
    {
        if (y < 0) return 0;
        if (y > H - PADDLE) return H - PADDLE;
        return y;
    }

    void Pong::serve(int dir)
    {
        bx = W / 2;
        by = std::rand() % H;
        vx = dir;
        vy = (std::rand() % 2) ? 1 : -1;
    }

    void Pong::reset()
    {
        py = cy = H / 2 - PADDLE / 2;
        ps = cs = 0;
        finished = false;
        serve(1);
    }

    void Pong::movePlayer(int dy)
    {
        if (!finished) py = clampPaddle(py + dy);
    }

    void Pong::cpuMove(int tick)
    {
        if (finished || tick % 3 == 0) return;            // skips every 3rd move, so it can be beaten
        int target = by - PADDLE / 2;
        if (cy < target) cy++;
        else if (cy > target) cy--;
        cy = clampPaddle(cy);
    }

    void Pong::step()
    {
        if (finished) return;

        int nx = bx + vx;
        int ny = by + vy;

        if (ny < 0 || ny >= H)                             // bounce off top and bottom
        {
            vy = -vy;
            ny = by + vy;
        }

        if (vx < 0 && nx <= 1 && ny >= py && ny < py + PADDLE)          // player paddle
        {
            vx = 1;
            nx = 2;
            int off = ny - (py + PADDLE / 2);
            if (off) vy = off < 0 ? -1 : 1;
        }
        else if (vx > 0 && nx >= W - 2 && ny >= cy && ny < cy + PADDLE) // CPU paddle
        {
            vx = -1;
            nx = W - 3;
            int off = ny - (cy + PADDLE / 2);
            if (off) vy = off < 0 ? -1 : 1;
        }
        else if (nx < 0)                                   // ball got past the player
        {
            cs++;
            serve(1);
            finished = cs >= WIN;
            return;
        }
        else if (nx >= W)                                  // ball got past the CPU
        {
            ps++;
            serve(-1);
            finished = ps >= WIN;
            return;
        }

        bx = nx;
        by = ny;
    }
}
