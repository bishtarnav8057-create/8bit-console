// GameScreen.h - Snake or Pong on one screen (plan 8.3): READY -> PLAYING -> PAUSED / OVER, a HUD in the
// header, sprites on the board, a pause menu on Esc. The rules stay in Games.h, unchanged.
#ifndef GAMESCREEN_H
#define GAMESCREEN_H

#include <QTimer>

#include "GameBoard.h"
#include "Games.h"
#include "screens/Screen.h"

class FooterHints;
class HudView;
class OverlayMenu;
class ScreenHeader;

class GameScreen : public Screen
{
    Q_OBJECT
public:
    enum Mode { SNAKE, PONG };

    explicit GameScreen(QWidget* parent = nullptr);
    void setMode(Mode m);            // always starts again in READY
    void enter() override;
    void leave() override;
    void suspend() override;
    bool hasProgressToLose() const override;

    // tiles for a game position (also used by the previews on Home)
    static QVector<GameBoard::Tile> snakeTiles(const Games::Snake& s, bool dead, bool chipLit);
    static QVector<GameBoard::Tile> pongTiles(const Games::Pong& p);

    void playTicks(int n);           // for --shots: start and run n ticks without the timer
    void pauseForShots() { pause(); }

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private:
    enum State { READY, PLAYING, PAUSED, OVER };

    void tick();
    void start();
    void pause();
    void resume();
    void restart();
    void quit();
    void gameOver();
    void chose(int item);
    void draw();
    int interval() const;

    Mode mode = SNAKE;
    State state = READY;
    bool upHeld = false, downHeld = false;
    int tickCount = 0;
    int serveWait = 0;               // Pong: ticks to wait after a point before the ball moves
    int scorer = 0;                  // 1 = you, 2 = COM: whose digit blinks
    int best = 0;
    bool newBest = false;
    bool chipLit = true;
    int flashStep = 0;

    Games::Snake snake;
    Games::Pong pong;

    ScreenHeader* header;
    HudView* hud;
    GameBoard* board;
    OverlayMenu* overlay;
    FooterHints* footer;
    QTimer timer, chipBlink, flash;
};

#endif
