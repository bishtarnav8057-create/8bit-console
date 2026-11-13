#include "GameBoard.h"

#include <QPainter>
#include <algorithm>
#include <cmath>

#include "Theme.h"
#include "art/SpriteCache.h"

GameBoard::GameBoard(QWidget* parent) : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

int GameBoard::physCell() const
{
    // the biggest cell that fits, in physical pixels and a whole multiple of 8 so the 8 x 8 tiles
    // scale by a whole number (plan 9.3)
    const qreal dpr = devicePixelRatioF();
    const int frame = bezel ? Theme::BEZEL + 4 : 0;
    const int fit = (int)std::min((width() - 2 * frame) * dpr / Games::W, (height() - 2 * frame) * dpr / Games::H);
    return std::max(8, fit / 8 * 8);
}

QRect GameBoard::screenRect() const
{
    const qreal cell = physCell() / devicePixelRatioF();
    const int w = (int)std::ceil(cell * Games::W), h = (int)std::ceil(cell * Games::H);
    return QRect((width() - w) / 2, (height() - h) / 2, w, h);
}

void GameBoard::resizeEvent(QResizeEvent*)
{
    for (QWidget* child : findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly)) child->setGeometry(screenRect());
}

void GameBoard::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    const qreal dpr = devicePixelRatioF();
    const int phys = physCell();
    const qreal cell = phys / dpr;
    const QRect r = screenRect();
    const bool snake = style == Style::Snake;

    if (bezel) Theme::drawBezel(p, r);
    p.fillRect(r, snake ? Theme::boardSnake : Theme::boardPong);

    // a dot in the middle of every cell, so the grid can be read without lines
    const int px = phys / 8;                      // physical pixels per sprite pixel
    const qreal dot = std::max(1, px) / dpr;
    const QColor dotColour = snake ? Theme::dotSnake : Theme::dotPong;
    for (int y = 0; y < Games::H; y++)
        for (int x = 0; x < Games::W; x++)
            p.fillRect(QRectF(r.x() + (x + 0.5) * cell - dot / 2, r.y() + (y + 0.5) * cell - dot / 2, dot, dot), dotColour);

    if (!snake)   // dashed net down the middle
    {
        const qreal nx = r.x() + (Games::W / 2 + 0.5) * cell - dot;
        for (int y = 0; y < Games::H; y += 2)
            p.fillRect(QRectF(nx, r.y() + y * cell + cell / 4, 2 * dot, cell / 2), Theme::line);
    }

    for (const Tile& t : tiles)
        SpriteCache::drawPhys(p, t.id, QPointF(r.x() + t.x * cell, r.y() + t.y * cell), px, t.rotation, t.swap);

    if (Prefs::scanlines) Theme::drawScanlines(p, r);
    if (flash) p.fillRect(r, QColor(Theme::red.red(), Theme::red.green(), Theme::red.blue(), 64));
}
