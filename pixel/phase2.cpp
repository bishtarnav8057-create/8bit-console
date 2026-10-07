
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <ctime>
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#endif

const int W = 32, H = 16, SPR = 8;                 
uint8_t screen[H][W];                               
const uint8_t PLAYER[8] = {0x18,0x3C,0x7E,0xDB,0xFF,0x7E,0x24,0x42};
const uint8_t ROCK[8]   = {0x3C,0x7E,0xFF,0xFF,0xFF,0xFF,0x7E,0x3C};
struct Rock { int x, y; bool on; };
const int MAXR = 3;

void clearScreen() { std::memset(screen, 0, sizeof(screen)); }

int countLit() {                                    
    int n = 0;
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) n += screen[y][x];
    return n;
}

bool drawSprite(const uint8_t* s, int x, int y) {  
    bool hit = false;
    for (int r = 0; r < SPR; ++r)
        for (int c = 0; c < SPR; ++c) {
            if (!((s[r] >> (7 - c)) & 1)) continue;
            int px = x + c, py = y + r;
            if (px < 0 || px >= W || py < 0 || py >= H) continue;   
            if (screen[py][px]) { screen[py][px] = 0; hit = true; }
            else screen[py][px] = 1;
        }
    return hit;
}

int clampX(int x) { return x < 0 ? 0 : (x > W - SPR ? W - SPR : x); }

bool drawScene(int playerX, const Rock* rocks) {     
    clearScreen();
    drawSprite(PLAYER, playerX, H - SPR);
    bool hit = false;
    for (int i = 0; i < MAXR; ++i) if (rocks[i].on) hit |= drawSprite(ROCK, rocks[i].x, rocks[i].y);
    return hit;
}

int stepRocks(Rock* rocks) {                       
    int pts = 0;
    for (int i = 0; i < MAXR; ++i)
        if (rocks[i].on && ++rocks[i].y >= H) { rocks[i].on = false; ++pts; }
    return pts;
}

void spawnRock(Rock* rocks) {                       
    for (int i = 0; i < MAXR; ++i)
        if (!rocks[i].on) { rocks[i] = {std::rand() % (W - SPR + 1), -SPR + 1, true}; return; }
}


int passed = 0, failed = 0;
#define CHECK(name, cond) do { if (cond) { ++passed; std::printf("  PASS  %s\n", name); } \
                               else { ++failed; std::printf("  FAIL  %s\n", name); } } while (0)

int runTests() {
    std::printf("Running tests...\n");
    clearScreen();                       CHECK("clear leaves 0 lit pixels", countLit() == 0);
    drawSprite(PLAYER, 0, 0);            CHECK("player sprite lights 36 pixels", countLit() == 36);
    clearScreen(); drawSprite(ROCK, 0, 0); CHECK("rock sprite lights 52 pixels", countLit() == 52);
    clearScreen(); drawSprite(PLAYER, 4, 4);
    bool again = drawSprite(PLAYER, 4, 4);
    CHECK("drawing twice reports collision", again);
    CHECK("drawing twice erases it (XOR)", countLit() == 0);
    clearScreen(); drawSprite(PLAYER, 0, 0);
    CHECK("sprites far apart: no collision", !drawSprite(ROCK, 20, 8));
    clearScreen(); drawSprite(PLAYER, 10, 8);
    CHECK("overlapping sprites: collision", drawSprite(ROCK, 12, 6));
    clearScreen();
    CHECK("sprite off-screen does not crash", !drawSprite(ROCK, -20, -20) && countLit() == 0);
    clearScreen(); drawSprite(ROCK, -4, 0);
    CHECK("half off-screen sprite is clipped", countLit() > 0 && countLit() < 52);
    CHECK("clampX keeps player on screen", clampX(-5) == 0 && clampX(99) == W - SPR && clampX(10) == 10);
    Rock r[MAXR] = {};
    r[0] = {0, 0, true};
    CHECK("rock falls 1 row per step", stepRocks(r) == 0 && r[0].y == 1);
    r[0].y = H - 1;
    CHECK("rock leaving bottom scores 1", stepRocks(r) == 1 && !r[0].on);
    Rock a[MAXR] = {}; a[0] = {12, 8, true};
    CHECK("scene: rock on player = hit", drawScene(12, a));
    a[0] = {24, 0, true};
    CHECK("scene: rock far away = no hit", !drawScene(0, a));
    Rock s[MAXR] = {}; spawnRock(s);
    CHECK("spawn creates a rock inside lanes", s[0].on && s[0].x >= 0 && s[0].x <= W - SPR);
    std::printf("\nResult: %d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}

#ifdef _WIN32
void draw(int score, bool over) {
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), {0, 0});
    std::string out;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) out += screen[y][x] ? "##" : "..";
        out += "\n";
    }
    std::printf("%sScore: %d   %s\n[A/D or arrows = move, Q = quit]      \n", out.c_str(), score,
                over ? "GAME OVER (press R)" : "                   ");
}
int play() {
    system("cls");
    int px = 12, score = 0, frame = 0; bool over = false;
    Rock rocks[MAXR] = {};
    for (;;) {
        while (_kbhit()) {
            int c = _getch();
            if (c == 0 || c == 224) { c = _getch(); px += (c == 75) ? -2 : (c == 77 ? 2 : 0); }
            else if (c == 'a' || c == 'A') px -= 2;
            else if (c == 'd' || c == 'D') px += 2;
            else if (c == 'q' || c == 'Q') return 0;
            else if ((c == 'r' || c == 'R') && over) { std::memset(rocks, 0, sizeof(rocks)); score = 0; over = false; }
        }
        px = clampX(px);
        if (!over) {
            ++frame;
            if (frame % 2 == 0) score += stepRocks(rocks);
            if (frame % 10 == 0) spawnRock(rocks);
        }
        if (drawScene(px, rocks)) over = true;
        draw(score, over);
        Sleep(80);
    }
}
#endif

int main(int argc, char** argv) {
    std::srand((unsigned)std::time(nullptr));
    if (argc > 1 && std::strcmp(argv[1], "test") == 0) return runTests();
#ifdef _WIN32
    return play();
#else
    std::printf("Interactive mode needs Windows. Run with 'test' instead.\n");
    return 0;
#endif
}
