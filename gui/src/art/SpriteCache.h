// SpriteCache.h - sprite grid -> crisp QPixmap, cached. Scaling is by whole physical pixels with
// nearest-neighbour, so pixel art stays sharp at 125 % and 150 % Windows scaling (plan 9.3).
#ifndef SPRITECACHE_H
#define SPRITECACHE_H

#include <QColor>
#include <QPixmap>
#include <QPointF>

#include "art/Sprites.h"

class QPainter;

namespace SpriteCache
{
    // physPerPixel = physical pixels per sprite pixel. rotation in 90 degree steps clockwise.
    // tint (if valid) replaces the colour of every 'w' pixel (used for glyphs).
    QPixmap pixmap(SpriteId id, int physPerPixel, int rotation = 0, Swap swap = Swap::None,
                   const QColor& tint = QColor(), qreal dpr = 1.0);

    // Draws at a logical position with `scale` logical pixels per sprite pixel.
    void draw(QPainter& p, SpriteId id, QPointF topLeft, int scale, int rotation = 0,
              Swap swap = Swap::None, const QColor& tint = QColor());

    // Same, but the size of one sprite pixel is given in physical pixels (boards use this).
    void drawPhys(QPainter& p, SpriteId id, QPointF topLeft, int physPerPixel, int rotation = 0,
                  Swap swap = Swap::None, const QColor& tint = QColor());
}

#endif
