// GameBoard.h - a widget that draws a grid of coloured squares. It knows nothing about Snake or Pong.
#ifndef GAMEBOARD_H
#define GAMEBOARD_H

#include <QString>
#include <QWidget>
#include <vector>

#include "Games.h"

class GameBoard : public QWidget
{
public:
    // colour numbers used by setCell()
    enum Colour { EMPTY = 0, SNAKE_BODY, SNAKE_HEAD, FOOD, WHITE, GREY };

    explicit GameBoard(QWidget* parent = nullptr);

    void clear();
    void setCell(int x, int y, int colour);
    void setMessage(const QString& big, const QString& small = QString());   // "" hides it

    QSize sizeHint() const override { return QSize(Games::W * 20, Games::H * 20); }
    QSize minimumSizeHint() const override { return QSize(Games::W * 10, Games::H * 10); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::vector<int> cells;
    QString bigText, smallText;
};

#endif
