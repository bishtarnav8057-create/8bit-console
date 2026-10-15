
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#endif


const int W = 40, H = 22;            
const int FY = 7, FH = H - FY;      
uint8_t screen[H][W];                

void clearScreen() { std::memset(screen, 0, sizeof(screen)); }
void plot(int x, int y, int c) { if (x >= 0 && x < W && y >= 0 && y < H) screen[y][x] = (uint8_t)c; }
int  getPixel(int x, int y) { return (x >= 0 && x < W && y >= 0 && y < H) ? screen[y][x] : 0; }
void fillRect(int x, int y, int w, int h, int c) {
    for (int j = 0; j < h; ++j) for (int i = 0; i < w; ++i) plot(x + i, y + j, c);
}
int countLit() { int n = 0; for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) n += screen[y][x] != 0; return n; }


const uint8_t LET[26][5] = {
    {2,5,7,5,5},{6,5,6,5,6},{3,4,4,4,3},{6,5,5,5,6},{7,4,6,4,7},{7,4,6,4,4},{3,4,5,5,3},
    {5,5,7,5,5},{7,2,2,2,7},{1,1,1,5,2},{5,5,6,5,5},{4,4,4,4,7},{5,7,7,5,5},{6,5,5,5,5},
    {2,5,5,5,2},{6,5,6,4,4},{2,5,5,6,3},{6,5,6,5,5},{3,4,2,1,6},{7,2,2,2,2},{5,5,5,5,7},
    {5,5,5,5,2},{5,5,7,7,5},{5,5,2,5,5},{5,5,2,2,2},{7,1,2,4,7}};
const uint8_t DIG[10][5] = {{7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},{5,5,7,1,1},
                            {7,4,7,1,7},{7,4,7,5,7},{7,1,1,1,1},{7,5,7,5,7},{7,5,7,1,7}};
const uint8_t BLANK[5] = {0,0,0,0,0}, DASH[5] = {0,0,7,0,0};

const uint8_t* glyph(int ch) {
    if (std::isalpha((unsigned char)ch)) return LET[std::toupper(ch) - 'A'];
    if (ch >= '0' && ch <= '9') return DIG[ch - '0'];
    if (ch == '-') return DASH;
    return BLANK;
}
int textWidth(const char* s) { int n = (int)std::strlen(s); return n ? n * 4 - 1 : 0; }
void drawText(const char* s, int x, int y, int c) {
    for (int i = 0; s[i]; ++i) {
        const uint8_t* g = glyph(s[i]);
        for (int r = 0; r < 5; ++r) for (int k = 0; k < 3; ++k)
            if ((g[r] >> (2 - k)) & 1) plot(x + i * 4 + k, y + r, c);
    }
}
void drawNumber(int v, int x, int y, int c) { char b[16]; snprintf(b, sizeof(b), "%d", v); drawText(b, x, y, c); }
void drawHeaderLine() { for (int x = 0; x < W; ++x) plot(x, FY - 1, 1); }


const int DX[4] = {0, 1, 0, -1}, DY[4] = {-1, 0, 1, 0};   
struct Snake { int x[W * FH], y[W * FH], len, dir, next, fx, fy, score; bool dead; };

bool onSnake(const Snake& s, int x, int y) {
    for (int i = 0; i < s.len; ++i) if (s.x[i] == x && s.y[i] == y) return true;
    return false;
}
void placeFood(Snake& s) {
    for (int tries = 0; tries < 1000; ++tries) {
        s.fx = std::rand() % W; s.fy = std::rand() % FH;
        if (!onSnake(s, s.fx, s.fy)) return;
    }
}
void snakeInit(Snake& s) {
    s.len = 3; s.dir = s.next = 1; s.score = 0; s.dead = false;
    for (int i = 0; i < 3; ++i) { s.x[i] = 20 - i; s.y[i] = FH / 2; }
    placeFood(s);
}
void snakeTurn(Snake& s, int d) { if (d != (s.dir + 2) % 4) s.next = d; }  
void snakeStep(Snake& s) {
    if (s.dead) return;
    s.dir = s.next;
    int nx = s.x[0] + DX[s.dir], ny = s.y[0] + DY[s.dir];
    if (nx < 0 || nx >= W || ny < 0 || ny >= FH) { s.dead = true; return; }  
    bool grow = (nx == s.fx && ny == s.fy);
    int check = grow ? s.len : s.len - 1;         
    for (int i = 0; i < check; ++i) if (s.x[i] == nx && s.y[i] == ny) { s.dead = true; return; }
    int n = grow ? s.len + 1 : s.len;
    for (int i = n - 1; i > 0; --i) { s.x[i] = s.x[i - 1]; s.y[i] = s.y[i - 1]; }
    s.x[0] = nx; s.y[0] = ny; s.len = n;
    if (grow) { ++s.score; placeFood(s); }
}
void snakeDraw(const Snake& s) {
    clearScreen(); drawHeaderLine();
    drawText("SNAKE", 1, 1, 1); drawNumber(s.score, W - 4 * 3 - 1, 1, 2);
    plot(s.fx, s.fy + FY, 3);
    for (int i = s.len - 1; i >= 0; --i) plot(s.x[i], s.y[i] + FY, i == 0 ? 2 : 1);
}


const int PH = 5, WIN = 5;                                  
struct Pong { int bx, by, vx, vy, py, cy, ps, cs; bool over; };
int clampPaddle(int y) { return y < 0 ? 0 : (y > FH - PH ? FH - PH : y); }
void serve(Pong& p, int dir) { p.bx = W / 2; p.by = std::rand() % FH; p.vx = dir; p.vy = (std::rand() % 2) ? 1 : -1; }
void pongInit(Pong& p) { p.py = p.cy = FH / 2 - PH / 2; p.ps = p.cs = 0; p.over = false; serve(p, 1); }
void pongStep(Pong& p) {
    if (p.over) return;
    int nx = p.bx + p.vx, ny = p.by + p.vy;
    if (ny < 0 || ny >= FH) { p.vy = -p.vy; ny = p.by + p.vy; }       
    if (p.vx < 0 && nx <= 1 && ny >= p.py && ny < p.py + PH) {           
        p.vx = 1; nx = 2; int off = ny - (p.py + PH / 2); if (off) p.vy = off < 0 ? -1 : 1;
    } else if (p.vx > 0 && nx >= W - 2 && ny >= p.cy && ny < p.cy + PH) {
        p.vx = -1; nx = W - 3; int off = ny - (p.cy + PH / 2); if (off) p.vy = off < 0 ? -1 : 1;
    } else if (nx < 0)  { ++p.cs; serve(p, 1);  p.over = p.cs >= WIN; return; }  
    else if (nx >= W)   { ++p.ps; serve(p, -1); p.over = p.ps >= WIN; return; }  
    p.bx = nx; p.by = ny;
}
void pongCpu(Pong& p, int t) {                  
    if (t % 3 == 0) return;
    int target = p.by - PH / 2;
    if (p.cy < target) ++p.cy; else if (p.cy > target) --p.cy;
    p.cy = clampPaddle(p.cy);
}
void pongDraw(const Pong& p) {
    clearScreen(); drawHeaderLine();
    drawText("YOU", 1, 1, 1); drawNumber(p.ps, 14, 1, 2);
    drawText("CPU", 21, 1, 1); drawNumber(p.cs, W - 5, 1, 2);
    for (int y = 0; y < FH; y += 2) plot(W / 2, FY + y, 1);
    fillRect(1, FY + p.py, 1, PH, 1); fillRect(W - 2, FY + p.cy, 1, PH, 1);
    plot(p.bx, p.by + FY, 2);
}
void overlay(const char* msg) {
    fillRect(1, 9, W - 2, 12, 0);
    drawText(msg, (W - textWidth(msg)) / 2, 10, 2);
    drawText("R AGAIN", (W - textWidth("R AGAIN")) / 2, 15, 1);
}


int passed = 0, failed = 0;
#define CHECK(name, cond) do { if (cond) { ++passed; std::printf("  PASS  %s\n", name); } \
                               else { ++failed; std::printf("  FAIL  %s\n", name); } } while (0)

int runTests() {
    std::printf("-- Font / PPU --\n");
    bool all = true;
    for (int c = 'A'; c <= 'Z'; ++c) { int n = 0; for (int r = 0; r < 5; ++r) n += glyph(c)[r] != 0; all = all && n > 0; }
    for (int c = '0'; c <= '9'; ++c) { int n = 0; for (int r = 0; r < 5; ++r) n += glyph(c)[r] != 0; all = all && n > 0; }
    CHECK("every letter and digit has a glyph", all);
    clearScreen(); drawText("A", 0, 0, 1);
    CHECK("letter A lights 10 pixels", countLit() == 10);
    clearScreen(); drawText("a", 0, 0, 1);
    CHECK("lowercase draws same as uppercase", countLit() == 10);
    clearScreen(); drawText("#", 0, 0, 1);
    CHECK("unknown character draws nothing", countLit() == 0);
    CHECK("text width of PIXEL is 19", textWidth("PIXEL") == 19 && textWidth("") == 0);
    clearScreen(); drawText("HELLO", -100, -100, 1); plot(-1, 0, 1); plot(W, H, 1);
    CHECK("drawing off-screen is safe", countLit() == 0);
    clearScreen(); fillRect(W - 2, H - 2, 10, 10, 1);
    CHECK("fillRect is clipped to the screen", countLit() == 4);
    clearScreen(); plot(3, 4, 2);
    CHECK("getPixel reads back what plot wrote", getPixel(3, 4) == 2 && getPixel(-5, 0) == 0);

    std::printf("-- Snake --\n");
    Snake s; snakeInit(s);
    CHECK("snake starts with length 3, score 0", s.len == 3 && s.score == 0 && !s.dead);
    CHECK("food does not start on the snake", !onSnake(s, s.fx, s.fy));
    s.fx = 0; s.fy = 0; snakeStep(s);
    CHECK("snake moves right by 1", s.x[0] == 21 && s.x[2] == 19 && s.len == 3);
    snakeInit(s); snakeTurn(s, 3); snakeStep(s);
    CHECK("180-degree turn is ignored", s.x[0] == 21 && !s.dead);
    snakeInit(s); snakeTurn(s, 0); snakeStep(s);
    CHECK("turning up moves the head up", s.x[0] == 20 && s.y[0] == FH / 2 - 1);
    snakeInit(s); s.fx = 21; s.fy = FH / 2; snakeStep(s);
    CHECK("eating food grows snake and scores", s.len == 4 && s.score == 1);
    CHECK("new food is placed off the snake", !onSnake(s, s.fx, s.fy));
    snakeInit(s); s.x[0] = W - 1; s.fx = 0; s.fy = 0; snakeStep(s);
    CHECK("hitting the wall kills the snake", s.dead);
    s.len = 5; s.dead = false; s.dir = s.next = 3;
    int bx[5] = {5,5,4,4,4}, by[5] = {5,6,6,5,4};
    for (int i = 0; i < 5; ++i) { s.x[i] = bx[i]; s.y[i] = by[i]; }
    s.fx = 30; s.fy = 10; snakeStep(s);
    CHECK("hitting its own body kills the snake", s.dead);
    s.len = 4; s.dead = false; s.dir = s.next = 2;
    int cx[4] = {5,6,6,5}, cy[4] = {5,5,6,6};
    for (int i = 0; i < 4; ++i) { s.x[i] = cx[i]; s.y[i] = cy[i]; }
    snakeStep(s);
    CHECK("moving into the vacating tail is safe", !s.dead && s.x[0] == 5 && s.y[0] == 6);
    bool ok = true; snakeInit(s);
    for (int i = 0; i < 20000; ++i) {
        snakeTurn(s, std::rand() % 4); snakeStep(s);
        if (s.dead) snakeInit(s);
        ok = ok && s.x[0] >= 0 && s.x[0] < W && s.y[0] >= 0 && s.y[0] < FH && s.len <= W * FH;
    }
    CHECK("20000 random snake steps stay in bounds", ok);

    std::printf("-- Pong --\n");
    Pong p; pongInit(p);
    CHECK("pong starts 0-0 and not over", p.ps == 0 && p.cs == 0 && !p.over);
    p.bx = 20; p.by = 0; p.vx = 1; p.vy = -1; pongStep(p);
    CHECK("ball bounces off the top wall", p.vy == 1 && p.by == 1);
    p.bx = 20; p.by = FH - 1; p.vx = 1; p.vy = 1; pongStep(p);
    CHECK("ball bounces off the bottom wall", p.vy == -1 && p.by == FH - 2);
    p.py = 5; p.bx = 2; p.by = 6; p.vx = -1; p.vy = 1; pongStep(p);
    CHECK("player paddle returns the ball", p.vx == 1 && p.bx == 2);
    p.cy = 5; p.bx = W - 3; p.by = 6; p.vx = 1; p.vy = 1; pongStep(p);
    CHECK("CPU paddle returns the ball", p.vx == -1 && p.bx == W - 3);
    pongInit(p); p.py = 12; p.bx = 0; p.by = 2; p.vx = -1; p.vy = 1; pongStep(p);
    CHECK("missing the ball gives CPU a point", p.cs == 1 && p.bx == W / 2);
    p.cy = 12; p.bx = W - 1; p.by = 2; p.vx = 1; p.vy = 1; pongStep(p);
    CHECK("CPU missing gives player a point", p.ps == 1);
    p.cs = WIN - 1; p.py = 12; p.bx = 0; p.by = 2; p.vx = -1; pongStep(p);
    CHECK("first to 5 ends the game", p.over && p.cs == WIN);
    CHECK("paddle is clamped to the field", clampPaddle(-3) == 0 && clampPaddle(99) == FH - PH);
    ok = true; pongInit(p);
    for (int i = 0; i < 20000; ++i) {
        pongCpu(p, i); pongStep(p); if (p.over) pongInit(p);
        ok = ok && p.bx >= 0 && p.bx < W && p.by >= 0 && p.by < FH && p.cy >= 0 && p.cy <= FH - PH;
    }
    CHECK("20000 pong ticks keep ball and paddle in bounds", ok);

    std::printf("\nResult: %d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}


#ifdef _WIN32
void present(const char* footer) {
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), {0, 0});
    static const char* G[4] = {". ", "##", "@@", "()"};
    std::string out;
    for (int y = 0; y < H; ++y) { for (int x = 0; x < W; ++x) out += G[screen[y][x] & 3]; out += "\n"; }
    out += footer; out += "                                                  \n";
    std::fputs(out.c_str(), stdout);
}
int readKey() {
    int c = _getch();
    if (c == 0 || c == 224) { c = _getch(); return c == 72 ? 'W' : c == 80 ? 'S' : c == 75 ? 'A' : c == 77 ? 'D' : 0; }
    return std::toupper(c);
}
int arcade() {
    enum { MENU, SNAKE, PONG } mode = MENU;
    Snake sn; Pong pg; snakeInit(sn); pongInit(pg);
    int frame = 0;
    std::system("cls");
    for (;;) {
        while (_kbhit()) {
            int k = readKey();
            if (mode == MENU) {
                if (k == '1') { mode = SNAKE; snakeInit(sn); }
                else if (k == '2') { mode = PONG; pongInit(pg); }
                else if (k == 'Q') return 0;
            } else if (k == 'M') mode = MENU;
            else if (mode == SNAKE) {
                if (k == 'W') snakeTurn(sn, 0); else if (k == 'D') snakeTurn(sn, 1);
                else if (k == 'S') snakeTurn(sn, 2); else if (k == 'A') snakeTurn(sn, 3);
                else if (k == 'R' && sn.dead) snakeInit(sn);
            } else {
                if (k == 'W') pg.py = clampPaddle(pg.py - 2); else if (k == 'S') pg.py = clampPaddle(pg.py + 2);
                else if (k == 'R' && pg.over) pongInit(pg);
            }
        }
        ++frame;
        if (mode == SNAKE && frame % 3 == 0) snakeStep(sn);
        if (mode == PONG && frame % 2 == 0) { pongCpu(pg, frame / 2); pongStep(pg); }
        if (mode == MENU) {
            clearScreen(); drawHeaderLine();
            drawText("PIXEL 8", (W - textWidth("PIXEL 8")) / 2, 1, 2);
            drawText("1 SNAKE", 8, 10, 1); drawText("2 PONG", 8, 16, 1);
            present("[1] Snake   [2] Pong   [Q] Quit");
        } else if (mode == SNAKE) {
            snakeDraw(sn); if (sn.dead) overlay("GAME OVER");
            present("W A S D / arrows = turn   M = menu");
        } else {
            pongDraw(pg); if (pg.over) overlay(pg.ps >= WIN ? "YOU WIN" : "CPU WINS");
            present("W / S or Up / Down = paddle   M = menu");
        }
        Sleep(50);
    }
}
#endif

int main(int argc, char** argv) {
    std::srand((unsigned)std::time(nullptr));
    if (argc > 1 && std::strcmp(argv[1], "test") == 0) return runTests();
#ifdef _WIN32
    return arcade();
#else
    std::printf("Playing needs Windows. Run with 'test' instead.\n");
    return 0;
#endif
}
