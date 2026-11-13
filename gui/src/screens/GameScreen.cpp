#include "screens/GameScreen.h"

#include <QFontMetrics>
#include <QKeyEvent>
#include <QPainter>
#include <QSettings>
#include <QVBoxLayout>

#include "Theme.h"
#include "model/SnakeSegments.h"
#include "widgets/OverlayMenu.h"
#include "widgets/Pixel.h"

namespace
{
    const int SNAKE_MS = 130;    // time between two snake moves
    const int PONG_MS  = 70;     // time between two ball moves
    const int SERVE_TICKS = 7;   // about 500 ms before the next serve

    const QString UP(QChar(0x2191)), DOWN(QChar(0x2193)), LEFT(QChar(0x2190)), RIGHT(QChar(0x2192)),
        ENTER(QChar(0x23CE));

    QString pad4(int n) { return QString("%1").arg(n, 4, 10, QChar('0')); }
}

// ---------------------------------------------------------------- HudView
// A row of text pieces in the header: labels in the caps label font, numbers in the pixel font.
class HudView : public QWidget
{
public:
    struct Piece { QString text; QColor colour; bool number; bool hidden = false; int gapAfter = 8; };

    explicit HudView(QWidget* parent) : QWidget(parent) { setFixedHeight(40); }
    void setPieces(const QVector<Piece>& p) { pieces = p; updateGeometry(); update(); }
    QSize sizeHint() const override
    {
        int w = 0;
        for (const Piece& p : pieces) w += pieceWidth(p) + p.gapAfter;
        return QSize(w, 40);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        int x = 0;
        for (const Piece& pc : pieces)
        {
            p.setFont(font(pc));
            p.setPen(pc.colour);
            if (!pc.hidden) p.drawText(QRect(x, 0, pieceWidth(pc) + 2, height()), Qt::AlignVCenter | Qt::AlignLeft, pc.text);
            x += pieceWidth(pc) + pc.gapAfter;
        }
    }

private:
    static QFont font(const Piece& p) { return p.number ? Theme::pixel(16) : Theme::label(); }
    static int pieceWidth(const Piece& p) { return QFontMetrics(font(p)).horizontalAdvance(p.text); }
    QVector<Piece> pieces;
};

// ---------------------------------------------------------------- tiles

QVector<GameBoard::Tile> GameScreen::snakeTiles(const Games::Snake& s, bool dead, bool lit)
{
    QVector<GameBoard::Tile> tiles;
    const Games::Point f = s.food();
    tiles.append({f.x, f.y, SpriteId::Chip, 0, lit ? Swap::None : Swap::Dim});
    const std::vector<Games::Point>& body = s.body();
    const Swap swap = dead ? Swap::Dead : Swap::None;
    for (int i = (int)body.size() - 1; i >= 0; i--)   // tail first, so the head is drawn on top
    {
        const SnakeSegments::Segment seg = SnakeSegments::at(body, i);
        static const SpriteId ids[] = {SpriteId::SnakeHead, SpriteId::SnakeBody, SpriteId::SnakeCorner, SpriteId::SnakeTail};
        tiles.append({body[i].x, body[i].y, ids[seg.kind], seg.rotation, swap});
    }
    return tiles;
}

QVector<GameBoard::Tile> GameScreen::pongTiles(const Games::Pong& g)
{
    QVector<GameBoard::Tile> tiles;
    const int n = Games::Pong::PADDLE;
    for (int i = 0; i < n; i++)
    {
        // YOU: drawn as is. COM: turned 180 degrees (shade on the outside) and recoloured cyan.
        const SpriteId you = i == 0 ? SpriteId::PadTop : i == n - 1 ? SpriteId::PadBot : SpriteId::PadMid;
        const SpriteId com = i == 0 ? SpriteId::PadBot : i == n - 1 ? SpriteId::PadTop : SpriteId::PadMid;
        tiles.append({1, g.playerY() + i, you, 0, Swap::None});
        tiles.append({Games::W - 2, g.cpuY() + i, com, 2, Swap::Com});
    }
    tiles.append({g.ballX(), g.ballY(), SpriteId::Ball, 0, Swap::None});
    return tiles;
}

// ---------------------------------------------------------------- GameScreen

GameScreen::GameScreen(QWidget* parent) : Screen(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    best = QSettings().value("snake/best", 0).toInt();

    QVBoxLayout* page = new QVBoxLayout(this);
    page->setContentsMargins(0, 0, 0, 0);
    page->setSpacing(0);
    header = new ScreenHeader("SNAKE", this);
    connect(header, &ScreenHeader::back, this, [this] { if (state == PLAYING) pause(); else quit(); });
    hud = new HudView(header);
    header->right()->addWidget(hud);
    page->addWidget(header);

    board = new GameBoard(this);
    QVBoxLayout* middle = new QVBoxLayout();
    middle->setContentsMargins(24, 20, 24, 20);
    middle->addWidget(board);
    page->addLayout(middle, 1);
    overlay = new OverlayMenu(board);
    connect(overlay, &OverlayMenu::chosen, this, &GameScreen::chose);

    footer = new FooterHints(this);
    page->addWidget(footer);

    timer.setTimerType(Qt::PreciseTimer);
    connect(&timer, &QTimer::timeout, this, &GameScreen::tick);
    chipBlink.setInterval(400);
    connect(&chipBlink, &QTimer::timeout, this, [this] {
        chipLit = !chipLit;
        if (mode == SNAKE && !snake.dead()) draw();
    });
    flash.setInterval(60);
    connect(&flash, &QTimer::timeout, this, [this] {   // the death flash: on, off, on, off, then GAME OVER
        flashStep++;
        board->setFlash(flashStep % 2 == 1);
        if (flashStep >= 4) { flash.stop(); gameOver(); }
    });

    setMode(SNAKE);
}

int GameScreen::interval() const
{
    return mode == SNAKE ? SNAKE_MS : PONG_MS;
}

void GameScreen::setMode(Mode m)
{
    mode = m;
    board->setStyle(m == SNAKE ? GameBoard::Style::Snake : GameBoard::Style::Pong);
    header->setTitle(m == SNAKE ? "SNAKE" : "PONG");
    if (m == SNAKE)
        footer->setHints({{{UP, DOWN, LEFT, RIGHT}, "MOVE"}, {{"P"}, "PAUSE"}, {{"R"}, "RESTART"}, {{"ESC"}, "MENU"}});
    else
        footer->setHints({{{UP, DOWN}, "PADDLE"}, {{"P"}, "PAUSE"}, {{"R"}, "RESTART"}, {{"ESC"}, "MENU"}});
    restart();
}

// ---- state machine

void GameScreen::restart()
{
    timer.stop();
    flash.stop();
    board->setFlash(false);
    upHeld = downHeld = false;
    tickCount = serveWait = scorer = 0;
    newBest = false;
    if (mode == SNAKE) snake.reset(); else pong.reset();
    state = READY;
    draw();
    const QString name = mode == SNAKE ? "SNAKE" : "PONG";
    QVector<FooterHints::Hint> keys = mode == SNAKE
        ? QVector<FooterHints::Hint>{{{UP, DOWN, LEFT, RIGHT}, "MOVE"}, {{"P"}, "PAUSE"}}
        : QVector<FooterHints::Hint>{{{UP, DOWN}, "PADDLE"}, {{"P"}, "PAUSE"}};
    overlay->open(name, Theme::textHi,
                  {{"PRESS ENTER TO START", Theme::amber, false, true},
                   {mode == SNAKE ? "Eat the byte chips. Don't hit the walls." : "First to 5. You are green, COM is cyan.",
                    Theme::textMid},
                   {"Gameplay uses the keyboard.", Theme::textDim}},
                  {"START"}, keys);
}

void GameScreen::start()
{
    if (state != READY) return;
    state = PLAYING;
    overlay->hide();
    setFocus();
    timer.start(interval());
}

void GameScreen::pause()
{
    if (state != PLAYING) return;
    timer.stop();
    upHeld = downHeld = false;
    state = PAUSED;
    overlay->open("PAUSED", Theme::amber, {}, {"RESUME", "RESTART", "QUIT TO MENU"},
                  {{{UP, DOWN}, "CHOOSE"}, {{ENTER}, "OK"}});
}

void GameScreen::resume()
{
    if (state != PAUSED) return;
    state = PLAYING;
    overlay->hide();
    setFocus();
    timer.start(interval());
}

void GameScreen::quit()
{
    restart();
    emit back();
}

void GameScreen::gameOver()
{
    timer.stop();
    board->setFlash(false);
    state = OVER;
    draw();
    if (mode == SNAKE)
    {
        if (snake.score() > best)
        {
            best = snake.score();
            newBest = true;
            QSettings().setValue("snake/best", best);
        }
        QVector<OverlayMenu::Line> lines = {{"SCORE " + pad4(snake.score()), Theme::textHi, true}};
        if (newBest) lines.append({"NEW BEST!", Theme::amber, true, true});
        draw();
        overlay->open("GAME OVER", Theme::red, lines, {"PLAY AGAIN", "MENU"});
    }
    else
    {
        const bool won = pong.playerScore() >= Games::Pong::WIN;
        overlay->open(won ? "YOU WIN" : "COM WINS", won ? Theme::green : Theme::cyan,
                      {{QString("%1 : %2").arg(pong.playerScore()).arg(pong.cpuScore()), Theme::textHi, true}},
                      {"PLAY AGAIN", "MENU"});
    }
}

void GameScreen::chose(int item)
{
    if (state == READY) start();
    else if (state == PAUSED)
    {
        if (item == 0) resume();
        else if (item == 1) restart();
        else quit();
    }
    else if (state == OVER)
    {
        if (item == 0) restart();
        else quit();
    }
}

// ---- one move

void GameScreen::tick()
{
    tickCount++;
    if (mode == SNAKE)
    {
        snake.step();
        if (snake.dead())
        {
            timer.stop();
            draw();
            if (Prefs::animations) { flashStep = 0; flash.start(); }
            else gameOver();
            return;
        }
    }
    else
    {
        if (upHeld) pong.movePlayer(-1);
        if (downHeld) pong.movePlayer(1);
        if (serveWait > 0) serveWait--;   // after a point the ball waits; the rules are not changed
        else
        {
            const int ps = pong.playerScore(), cs = pong.cpuScore();
            pong.cpuMove(tickCount);
            pong.step();
            if (pong.playerScore() != ps) { scorer = 1; serveWait = SERVE_TICKS; }
            if (pong.cpuScore() != cs) { scorer = 2; serveWait = SERVE_TICKS; }
        }
        if (pong.over()) { draw(); gameOver(); return; }
    }
    draw();
}

void GameScreen::playTicks(int n)
{
    start();
    timer.stop();
    for (int i = 0; i < n && state == PLAYING; i++) tick();
    timer.stop();
    flash.stop();
}

void GameScreen::draw()
{
    if (mode == SNAKE)
    {
        board->setTiles(snakeTiles(snake, snake.dead(), chipLit));
        hud->setPieces({{"SCORE", Theme::textMid, false}, {pad4(snake.score()), Theme::textHi, true, false, 24},
                        {"LENGTH", Theme::textMid, false}, {QString::number(snake.body().size()), Theme::textHi, true, false, 24},
                        {"BEST", Theme::textMid, false}, {pad4(std::max(best, snake.score())), Theme::amber, true, false, 0}});
    }
    else
    {
        board->setTiles(pongTiles(pong));
        const bool blink = serveWait > 0 && serveWait % 2 == 1;   // the scorer's digit blinks
        hud->setPieces({{"YOU", Theme::green, false}, {QString::number(pong.playerScore()), Theme::green, true, blink && scorer == 1},
                        {":", Theme::textMid, true}, {QString::number(pong.cpuScore()), Theme::cyan, true, blink && scorer == 2},
                        {"COM", Theme::cyan, false, false, 32},
                        {QString("FIRST TO %1").arg(Games::Pong::WIN), Theme::textDim, false, false, 0}});
    }
}

// ---- keys

void GameScreen::keyPressEvent(QKeyEvent* event)
{
    const int key = event->key();
    switch (key)
    {
    case Qt::Key_Escape:
        if (state == PLAYING) pause(); else if (state == PAUSED) resume(); else quit();
        return;
    case Qt::Key_P: case Qt::Key_Space:
        if (event->isAutoRepeat()) return;
        if (state == PLAYING) pause();
        else if (state == PAUSED) resume();
        else if (state == READY && key == Qt::Key_Space) start();
        return;
    case Qt::Key_Return: case Qt::Key_Enter:
        if (state == READY) start();
        return;
    case Qt::Key_R:
        if (state != READY) restart();
        return;
    default: break;
    }

    if (state != PLAYING) return;
    if (mode == SNAKE)
    {
        if (key == Qt::Key_Up || key == Qt::Key_W) snake.turn(Games::UP);
        else if (key == Qt::Key_Right || key == Qt::Key_D) snake.turn(Games::RIGHT);
        else if (key == Qt::Key_Down || key == Qt::Key_S) snake.turn(Games::DOWN);
        else if (key == Qt::Key_Left || key == Qt::Key_A) snake.turn(Games::LEFT);
    }
    else
    {
        if (key == Qt::Key_Up || key == Qt::Key_W) upHeld = true;
        else if (key == Qt::Key_Down || key == Qt::Key_S) downHeld = true;
    }
}

void GameScreen::keyReleaseEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat()) return;      // the OS repeats held keys; ignore those fake releases
    const int key = event->key();
    if (key == Qt::Key_Up || key == Qt::Key_W) upHeld = false;
    else if (key == Qt::Key_Down || key == Qt::Key_S) downHeld = false;
}

void GameScreen::focusOutEvent(QFocusEvent*)
{
    upHeld = downHeld = false;               // Alt+Tab while holding a key must not leave the paddle moving
}

// ---- life cycle

void GameScreen::enter()
{
    chipBlink.start();
    if (overlay->isVisible()) overlay->setFocus(); else setFocus();
}

void GameScreen::leave()
{
    pause();
    chipBlink.stop();
}

void GameScreen::suspend()
{
    pause();
}

bool GameScreen::hasProgressToLose() const
{
    return state == PLAYING || state == PAUSED;
}
