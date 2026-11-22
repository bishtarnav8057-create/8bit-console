#include "art/SpriteCache.h"

#include <QHash>
#include <QImage>
#include <QPaintDevice>
#include <QPainter>
#include <QTransform>
#include <cmath>

namespace SpriteCache
{
    QPixmap pixmap(SpriteId id, int physPerPixel, int rotation, Swap swap, const QColor& tint, qreal dpr)
    {
        static QHash<QString, QPixmap> cache;
        if (physPerPixel < 1) physPerPixel = 1;
        rotation &= 3;
        const QString key = QString("%1/%2/%3/%4/%5/%6").arg((int)id).arg(physPerPixel).arg(rotation)
                                .arg((int)swap).arg(tint.isValid() ? tint.rgba() : 0).arg(dpr);
        auto it = cache.constFind(key);
        if (it != cache.constEnd()) return *it;

        const Sprite& s = sprite(id);
        QImage img(s.w, s.h, QImage::Format_ARGB32);
        img.fill(Qt::transparent);
        for (int y = 0; y < s.h; y++)
        {
            for (int x = 0; x < s.w; x++)
            {
                char c = s.rows[y][x];
                if (c == 'w' && tint.isValid()) { img.setPixel(x, y, tint.rgba()); continue; }
                long rgb = paletteRgb(swapChar(c, swap));
                if (rgb >= 0) img.setPixel(x, y, 0xFF000000u | (unsigned)rgb);
            }
        }
        if (rotation) img = img.transformed(QTransform().rotate(90 * rotation));   // exact for 90 degrees

        QPixmap pm = QPixmap::fromImage(img.scaled(img.width() * physPerPixel, img.height() * physPerPixel,
                                                   Qt::IgnoreAspectRatio, Qt::FastTransformation));
        pm.setDevicePixelRatio(dpr);
        cache.insert(key, pm);
        return pm;
    }

    void drawPhys(QPainter& p, SpriteId id, QPointF topLeft, int physPerPixel, int rotation, Swap swap,
                  const QColor& tint)
    {
        const qreal dpr = p.device() ? p.device()->devicePixelRatioF() : 1.0;
        // snap to a whole physical pixel so nothing is resampled
        QPointF snapped(std::round(topLeft.x() * dpr) / dpr, std::round(topLeft.y() * dpr) / dpr);
        p.drawPixmap(snapped, pixmap(id, physPerPixel, rotation, swap, tint, dpr));
    }

    void draw(QPainter& p, SpriteId id, QPointF topLeft, int scale, int rotation, Swap swap, const QColor& tint)
    {
        const qreal dpr = p.device() ? p.device()->devicePixelRatioF() : 1.0;
        drawPhys(p, id, topLeft, std::max(1, (int)std::floor(scale * dpr + 0.01)), rotation, swap, tint);
    }
}
