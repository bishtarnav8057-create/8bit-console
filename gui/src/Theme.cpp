#include "Theme.h"

#include <QFontMetrics>
#include <QLabel>
#include <QPainter>
#include <QSettings>

#include "art/SpriteCache.h"

namespace Theme
{
    QColor vramColour(int value)
    {
        switch (value)
        {
        case 0: return screenOff;
        case 2: return amber;
        case 3: return red;
        case 4: return cyan;
        case 5: return textHi;
        case 6: return textDim;
        case 7: return greenDeep;
        default: return green;   // 1, and 8..255: "any non-zero = on" programs still work
        }
    }

    QFont pixel(int px)
    {
        QFont f;
        f.setFamilies({"Press Start 2P", "Consolas"});
        f.setPixelSize(px);
        f.setStyleStrategy(QFont::NoAntialias);
        f.setHintingPreference(QFont::PreferNoHinting);
        return f;
    }

    QFont mono(int px, bool bold)
    {
        QFont f;
        f.setFamilies({"JetBrains Mono", "Consolas"});
        f.setStyleHint(QFont::Monospace);
        f.setPixelSize(px);
        f.setBold(bold);
        return f;
    }

    QFont button()
    {
        QFont f = mono(14, true);
        f.setCapitalization(QFont::AllUppercase);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1);
        return f;
    }

    QFont label()
    {
        QFont f = mono(12, true);
        f.setCapitalization(QFont::AllUppercase);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 2);
        return f;
    }

    void fillNotched(QPainter& p, const QRect& r, const QColor& c)
    {
        if (r.width() <= 4 || r.height() <= 4) { p.fillRect(r, c); return; }
        p.fillRect(r.x() + 2, r.y(), r.width() - 4, r.height(), c);
        p.fillRect(r.x(), r.y() + 2, r.width(), r.height() - 4, c);
    }

    void drawBox(QPainter& p, const QRect& r, const QColor& face, const QColor& border, int s)
    {
        if (s > 0) fillNotched(p, r.translated(s, s), shadow);
        fillNotched(p, r, border);
        p.fillRect(r.adjusted(2, 2, -2, -2), face);
    }

    namespace
    {
        // the single-char keys drawn as glyphs: up, right, down, left arrows and Enter
        bool glyphFor(const QString& key, SpriteId& id, int& rot)
        {
            if (key.size() != 1) return false;
            switch (key.at(0).unicode())
            {
            case 0x2191: id = SpriteId::Arrow; rot = 0; return true;
            case 0x2192: id = SpriteId::Arrow; rot = 1; return true;
            case 0x2193: id = SpriteId::Arrow; rot = 2; return true;
            case 0x2190: id = SpriteId::Arrow; rot = 3; return true;
            case 0x23CE: id = SpriteId::Enter; rot = 0; return true;
            default: return false;
            }
        }
    }

    int keyCapWidth(const QString& key)
    {
        SpriteId id; int rot;
        if (glyphFor(key, id, rot)) return 24;
        return std::max(24, QFontMetrics(pixel(8)).horizontalAdvance(key) + 12);
    }

    int drawKeyCap(QPainter& p, int x, int y, const QString& key)
    {
        const int w = keyCapWidth(key);
        fillNotched(p, QRect(x, y, w, KEYCAP_H), lineStrong);
        p.fillRect(x + 2, y + 2, w - 4, KEYCAP_H - 6, bgRaised);
        SpriteId id; int rot;
        if (glyphFor(key, id, rot))
        {
            SpriteCache::draw(p, id, QPointF(x + (w - 8) / 2, y + 4), 1, rot, Swap::None, textHi);
        }
        else
        {
            p.setFont(pixel(8));
            p.setPen(textHi);
            p.drawText(QRect(x, y + 2, w, KEYCAP_H - 6), Qt::AlignCenter, key);
        }
        return w;
    }

    void drawBezel(QPainter& p, const QRect& inner, bool powerOn)
    {
        const QRect outer = inner.adjusted(-BEZEL, -BEZEL, BEZEL, BEZEL);
        fillNotched(p, outer.adjusted(-2, -2, 2, 2), shadow);
        fillNotched(p, outer, bgPanel);
        p.fillRect(inner.adjusted(-2, -2, 2, 2), line);

        p.setFont(pixel(8));
        p.setPen(lineStrong);
        p.drawText(QRect(outer.left(), inner.bottom() + 3, outer.width(), BEZEL - 2), Qt::AlignCenter, "PIXEL-8");
        p.fillRect(outer.left() + 8, inner.bottom() + 7, 6, 6, powerOn ? green : ledOff);
    }

    void drawScanlines(QPainter& p, const QRect& r)
    {
        const QColor c(0, 0, 0, 70);
        for (int y = r.top() + 2; y <= r.bottom(); y += 3) p.fillRect(r.left(), y, r.width(), 1, c);
    }

    void setColour(QWidget* w, const QColor& c)
    {
        QPalette pal = w->palette();
        pal.setColor(QPalette::WindowText, c);
        pal.setColor(QPalette::Text, c);
        w->setPalette(pal);
    }

    QLabel* text(const QString& s, const QFont& f, const QColor& c, QWidget* parent)
    {
        QLabel* l = new QLabel(s, parent);
        l->setFont(f);
        setColour(l, c);
        return l;
    }
}

namespace Prefs
{
    void load()
    {
        QSettings s;
        scanlines = s.value("scanlines", true).toBool();
        animations = s.value("animations", true).toBool();
        bootScreen = s.value("bootScreen", true).toBool();
    }

    void save()
    {
        QSettings s;
        s.setValue("scanlines", scanlines);
        s.setValue("animations", animations);
        s.setValue("bootScreen", bootScreen);
    }
}
