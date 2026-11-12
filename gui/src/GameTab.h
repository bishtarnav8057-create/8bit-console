// GameTab.h - the "Game" tab: pick Snake or Pong, play it with the keyboard.
#ifndef GAMETAB_H
#define GAMETAB_H

#include <QWidget>

#include "Games.h"

class GameBoard;
class QLabel;
class QPushButton;
class QTimer;

class GameTab : public QWidget
{
    Q_OBJECT

public:
    explicit GameTab(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private slots:
    void tick();
    void startSnake();
    void startPong();
    void togglePause();
    void restart();
    void backToMenu();

private:
    enum Mode { MENU, SNAKE, PONG };

    void start(Mode newMode);
    void draw();

    Mode mode;
    bool paused;
    bool upHeld, downHeld;      // Pong paddle keys
    int tickCount;

    Games::Snake snake;
    Games::Pong pong;

    GameBoard* board;
    QLabel* titleLabel;
    QLabel* scoreLabel;
    QLabel* hintLabel;
    QPushButton* pauseButton;
    QPushButton* restartButton;
    QPushButton* menuButton;
    QTimer* timer;
};

#endif
