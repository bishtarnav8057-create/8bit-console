// Theme.h - the design tokens (plan section 5). This header and assets/theme/theme.qss are the only
// places with colour codes. Plus the three user settings (Prefs) and small paint helpers.
#ifndef THEME_H
#define THEME_H

#include <QColor>
#include <QFont>
#include <QRect>
#include <QString>

class QPainter;
class QLabel;
class QWidget;

namespace Theme
{
    inline const QColor bgVoid{0x0B, 0x0C, 0x14};
    inline const QColor bgBase{0x12, 0x14, 0x1F};
    inline const QColor bgPanel{0x1A, 0x1D, 0x2E};
    inline const QColor bgRaised{0x24, 0x28, 0x40};
    inline const QColor bgRaisedHover{0x2C, 0x31, 0x50};
    inline const QColor rowAlt{0x15, 0x18, 0x27};
    inline const QColor line{0x2E, 0x34, 0x52};
    inline const QColor lineStrong{0x45, 0x4C, 0x73};
    inline const QColor shadow{0x07, 0x08, 0x0D};
    inline const QColor textHi{0xEC, 0xE8, 0xD8};
    inline const QColor textMid{0xA9, 0xAD, 0xC6};
    inline const QColor textDim{0x80, 0x85, 0x9F};
    inline const QColor green{0x7D, 0xF0, 0x9B};
    inline const QColor greenDeep{0x2E, 0x8A, 0x4C};
    inline const QColor greenBadge{0x12, 0x30, 0x1F};
    inline const QColor amber{0xFF, 0xB5, 0x47};
    inline const QColor amberHover{0xFF, 0xC4, 0x6B};
    inline const QColor amberBorder{0xC9, 0x8A, 0x2E};
    inline const QColor amberInk{0x1A, 0x12, 0x06};
    inline const QColor red{0xFF, 0x6B, 0x78};
    inline const QColor redBorder{0xC4, 0x45, 0x52};
    inline const QColor redInk{0x1A, 0x06, 0x09};
    inline const QColor cyan{0x62, 0xD2, 0xFF};
    inline const QColor ledOff{0x1E, 0x22, 0x33};
    inline const QColor screenOff{0x08, 0x13, 0x0E};
    inline const QColor screenGrid{0x10, 0x26, 0x1A};
    inline const QColor boardSnake{0x0D, 0x17, 0x12};
    inline const QColor dotSnake{0x17, 0x30, 0x24};
    inline const QColor boardPong{0x0E, 0x10, 0x20};
    inline const QColor dotPong{0x1A, 0x1D, 0x33};
    inline const QColor dim{0x07, 0x08, 0x0D, 184};          // dialog backdrop, 72 %

    // PIXEL-8 screen palette, VRAM byte -> colour (plan 5.3). 8..255 draw like 1.
    QColor vramColour(int value);

    // Fonts. Press Start 2P only at 8, 16, 24 or 32 px.
    QFont pixel(int px);
    QFont mono(int px, bool bold = false);
    QFont button();       // JBM Bold 14, caps, spacing 1
    QFont label();        // JBM Bold 12, caps, spacing 2

    // Painting. Every box has 2 x 2 px notched corners and an optional hard shadow (+s, +s).
    void fillNotched(QPainter& p, const QRect& r, const QColor& c);
    void drawBox(QPainter& p, const QRect& r, const QColor& face, const QColor& border, int shadow = 0);
    // A keyboard key. Single arrows and the Enter sign are drawn as pixel glyphs. Returns the width.
    int drawKeyCap(QPainter& p, int x, int y, const QString& key);
    int keyCapWidth(const QString& key);
    const int KEYCAP_H = 22;
    // The console bezel around a screen/board ("inner"): 16 px frame, PIXEL-8 label, power LED.
    void drawBezel(QPainter& p, const QRect& inner, bool powerOn = true);
    const int BEZEL = 16;
    // Dark scanlines over a rect: one dim line every 2 physical pixels... every 3 logical px.
    void drawScanlines(QPainter& p, const QRect& r);

    // A label with a font and colour (through the palette; QSS has no QLabel rule).
    QLabel* text(const QString& s, const QFont& f, const QColor& c, QWidget* parent = nullptr);
    void setColour(QWidget* w, const QColor& c);
}

// The three settings of the Settings dialog, kept in QSettings. Read once at start-up.
namespace Prefs
{
    inline bool scanlines = true;
    inline bool animations = true;
    inline bool bootScreen = true;
    void load();
    void save();
}

#endif
