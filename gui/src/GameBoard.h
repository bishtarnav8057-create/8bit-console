// GameBoard.h - draws a 30 x 20 board of 8 x 8 sprite tiles in a console bezel. It knows nothing about
// Snake or Pong: the screen gives it tiles. Child widgets (the overlays) are kept over the board area.
#ifndef GAMEBOARD_H
#define GAMEBOARD_H

#include <QVector>
#include <QWidget>

#include "Games.h"
#include "art/Sprites.h"

class GameBoard : public QWidget
{
public:
    struct Tile
    {
        int x, y;
        SpriteId id;
        int rotation = 0;
        Swap swap = Swap::None;
    };
    enum class Style { Snake, Pong };

    explicit GameBoard(QWidget* parent = nullptr);

    void setStyle(Style s) { style = s; update(); }
    void setTiles(const QVector<Tile>& t) { tiles = t; update(); }
    void setFlash(bool on) { flash = on; update(); }
    void setBezel(bool on) { bezel = on; update(); }
    QRect screenRect() const;     // the board itself, in widget coordinates

    QSize sizeHint() const override { return QSize(Games::W * 24 + 40, Games::H * 24 + 40); }
    QSize minimumSizeHint() const override { return QSize(Games::W * 8, Games::H * 8); }

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    int physCell() const;         // physical pixels per cell, a multiple of 8

    QVector<Tile> tiles;
    Style style = Style::Snake;
    bool flash = false, bezel = true;
};

#endif
