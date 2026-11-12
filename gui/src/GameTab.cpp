#include "GameTab.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include "GameBoard.h"

namespace
{
    const int SNAKE_MS = 130;    // time between two snake moves
    const int PONG_MS  = 70;     // time between two ball moves
}

GameTab::GameTab(QWidget* parent)
    : QWidget(parent), mode(MENU), paused(false), upHeld(false), downHeld(false), tickCount(0)
{
    setFocusPolicy(Qt::StrongFocus);

    titleLabel = new QLabel(this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    scoreLabel = new QLabel(this);
    QFont scoreFont = scoreLabel->font();
    scoreFont.setPointSize(13);
    scoreLabel->setFont(scoreFont);
    scoreLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QPushButton* snakeButton = new QPushButton("Snake", this);
    QPushButton* pongButton = new QPushButton("Pong", this);
    pauseButton = new QPushButton("Pause (P)", this);
    restartButton = new QPushButton("Restart (R)", this);
    menuButton = new QPushButton("Menu (M)", this);

    // buttons must not take the keyboard focus, or the arrow keys would stop reaching the game
    QPushButton* all[5] = {snakeButton, pongButton, pauseButton, restartButton, menuButton};
    for (int i = 0; i < 5; i++) all[i]->setFocusPolicy(Qt::NoFocus);

    board = new GameBoard(this);

    hintLabel = new QLabel(this);
    hintLabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout* top = new QHBoxLayout();
    top->addWidget(titleLabel);
    top->addStretch();
    top->addWidget(scoreLabel);

    QHBoxLayout* buttons = new QHBoxLayout();
    buttons->addWidget(snakeButton);
    buttons->addWidget(pongButton);
    buttons->addStretch();
    buttons->addWidget(pauseButton);
    buttons->addWidget(restartButton);
    buttons->addWidget(menuButton);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addLayout(top);
    layout->addLayout(buttons);
    layout->addWidget(board, 1);
    layout->addWidget(hintLabel);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(tick()));

    connect(snakeButton, SIGNAL(clicked()), this, SLOT(startSnake()));
    connect(pongButton, SIGNAL(clicked()), this, SLOT(startPong()));
    connect(pauseButton, SIGNAL(clicked()), this, SLOT(togglePause()));
    connect(restartButton, SIGNAL(clicked()), this, SLOT(restart()));
    connect(menuButton, SIGNAL(clicked()), this, SLOT(backToMenu()));

    backToMenu();
}

void GameTab::startSnake() { start(SNAKE); }
void GameTab::startPong()  { start(PONG); }

void GameTab::start(Mode newMode)
{
    mode = newMode;
    paused = false;
    upHeld = downHeld = false;
    tickCount = 0;

    if (mode == SNAKE)
    {
        snake.reset();
        timer->start(SNAKE_MS);
    }
    else
    {
        pong.reset();
        timer->start(PONG_MS);
    }
    setFocus();
    draw();
}

void GameTab::backToMenu()
{
    mode = MENU;
    paused = false;
    timer->stop();
    setFocus();
    draw();
}

void GameTab::restart()
{
    if (mode != MENU) start(mode);
}

void GameTab::togglePause()
{
    if (mode == MENU) return;
    if (mode == SNAKE && snake.dead()) return;
    if (mode == PONG && pong.over()) return;

    paused = !paused;
    if (paused) timer->stop();
    else timer->start(mode == SNAKE ? SNAKE_MS : PONG_MS);
    setFocus();
    draw();
}

void GameTab::tick()
{
    tickCount++;

    if (mode == SNAKE)
    {
        snake.step();
    }
    else if (mode == PONG)
    {
        if (upHeld) pong.movePlayer(-1);
        if (downHeld) pong.movePlayer(1);
        pong.cpuMove(tickCount);
        pong.step();
    }

    // when a game ends the timer stops; R starts it again
    if ((mode == SNAKE && snake.dead()) || (mode == PONG && pong.over())) timer->stop();

    draw();
}

void GameTab::draw()
{
    board->clear();
    board->setMessage("");

    if (mode == MENU)
    {
        titleLabel->setText("PIXEL-8 Arcade");
        scoreLabel->setText("");
        hintLabel->setText("Pick a game with the buttons above.");
        board->setMessage("PIXEL-8 ARCADE", "Choose Snake or Pong");
    }
    else if (mode == SNAKE)
    {
        titleLabel->setText("Snake");
        scoreLabel->setText(QString("Score: %1    Length: %2").arg(snake.score()).arg((int)snake.body().size()));
        hintLabel->setText("Arrow keys or W A S D = turn     P = pause     R = restart     M = menu");

        Games::Point f = snake.food();
        board->setCell(f.x, f.y, GameBoard::FOOD);
        const std::vector<Games::Point>& body = snake.body();
        for (size_t i = body.size(); i > 0; i--)
        {
            board->setCell(body[i - 1].x, body[i - 1].y, i == 1 ? GameBoard::SNAKE_HEAD : GameBoard::SNAKE_BODY);
        }

        if (snake.dead()) board->setMessage("GAME OVER", "Press R to play again");
    }
    else
    {
        titleLabel->setText("Pong");
        scoreLabel->setText(QString("You %1  :  %2 CPU    (first to %3)")
                            .arg(pong.playerScore()).arg(pong.cpuScore()).arg(Games::Pong::WIN));
        hintLabel->setText("W / S or Up / Down = move your paddle     P = pause     R = restart     M = menu");

        for (int y = 0; y < Games::H; y += 2) board->setCell(Games::W / 2, y, GameBoard::GREY);
        for (int i = 0; i < Games::Pong::PADDLE; i++)
        {
            board->setCell(1, pong.playerY() + i, GameBoard::WHITE);
            board->setCell(Games::W - 2, pong.cpuY() + i, GameBoard::WHITE);
        }
        board->setCell(pong.ballX(), pong.ballY(), GameBoard::FOOD);

        if (pong.over())
            board->setMessage(pong.playerScore() >= Games::Pong::WIN ? "YOU WIN!" : "CPU WINS", "Press R to play again");
    }

    if (paused) board->setMessage("PAUSED", "Press P to continue");
    pauseButton->setEnabled(mode != MENU);
    restartButton->setEnabled(mode != MENU);
    menuButton->setEnabled(mode != MENU);
    board->update();
}

void GameTab::keyPressEvent(QKeyEvent* event)
{
    int key = event->key();

    if (key == Qt::Key_P || key == Qt::Key_Space) { togglePause(); return; }
    if (key == Qt::Key_R) { restart(); return; }
    if (key == Qt::Key_M || key == Qt::Key_Escape) { backToMenu(); return; }

    if (mode == SNAKE && !paused)
    {
        if (key == Qt::Key_Up || key == Qt::Key_W) snake.turn(Games::UP);
        else if (key == Qt::Key_Right || key == Qt::Key_D) snake.turn(Games::RIGHT);
        else if (key == Qt::Key_Down || key == Qt::Key_S) snake.turn(Games::DOWN);
        else if (key == Qt::Key_Left || key == Qt::Key_A) snake.turn(Games::LEFT);
    }
    else if (mode == PONG)
    {
        if (key == Qt::Key_Up || key == Qt::Key_W) upHeld = true;
        else if (key == Qt::Key_Down || key == Qt::Key_S) downHeld = true;
    }
}

void GameTab::keyReleaseEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat()) return;      // the OS repeats held keys; ignore those fake releases

    int key = event->key();
    if (key == Qt::Key_Up || key == Qt::Key_W) upHeld = false;
    else if (key == Qt::Key_Down || key == Qt::Key_S) downHeld = false;
}

void GameTab::focusOutEvent(QFocusEvent*)
{
    upHeld = downHeld = false;               // Alt+Tab while holding a key must not leave the paddle moving
}

void GameTab::showEvent(QShowEvent*)
{
    setFocus();                              // so the keys work as soon as the tab is opened
}
