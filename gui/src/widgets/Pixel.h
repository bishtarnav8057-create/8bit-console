// Pixel.h - the small shared widgets of the design system (plan 5.6). They know nothing about the CPU
// or the games. All are painted in code with Theme colours, square corners and hard shadows.
#ifndef PIXEL_H
#define PIXEL_H

#include <QFrame>
#include <QPushButton>
#include <QStringList>
#include <QVector>
#include <QWidget>

#include "art/Sprites.h"

class QHBoxLayout;
class QLabel;
class QVBoxLayout;

class PixelButton : public QPushButton
{
    Q_OBJECT
public:
    enum Kind { Primary, Secondary, Ghost, Danger };
    enum Height { Small = 32, Normal = 40, Hero = 56 };

    explicit PixelButton(const QString& text, Kind kind = Secondary, QWidget* parent = nullptr);
    void setKind(Kind k) { kind = k; update(); }
    void setHeightKind(Height h) { height = h; updateGeometry(); }
    void setGlyph(SpriteId id, int rotation = 0) { glyph = (int)id; glyphRot = rotation; updateGeometry(); }
    void setTrailingGlyph(SpriteId id) { trailing = (int)id; updateGeometry(); }
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent*) override;

private:
    Kind kind;
    Height height = Normal;
    int glyph = -1, glyphRot = 0, trailing = -1;
};

// A notched panel with an optional title row (amber square + label). Put children in body().
class Panel : public QFrame
{
public:
    explicit Panel(const QString& title = QString(), QWidget* parent = nullptr);
    QVBoxLayout* body() { return box; }
    void setTitle(const QString& t);
    void setColours(const QColor& face, const QColor& border);
    void setTitleAccent(const QColor& c) { accent = c; update(); }

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QVBoxLayout* box;
    QString title;
    QColor face, border, accent;
};

class KeyCap : public QWidget
{
public:
    explicit KeyCap(const QString& key, QWidget* parent = nullptr);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QString key;
};

class Led : public QWidget
{
public:
    explicit Led(const QColor& onColour, QWidget* parent = nullptr);
    void setOn(bool on);
    void setOnColour(const QColor& c) { colour = c; update(); }
    bool isOn() const { return lit; }
    QSize sizeHint() const override { return QSize(10, 10); }
    static void paintLed(class QPainter& p, int x, int y, bool on, const QColor& colour);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QColor colour;
    bool lit = false;
};

// LED + caps text, e.g. "RUNNING" in green. Used in screen headers.
class StatusChip : public QWidget
{
public:
    explicit StatusChip(QWidget* parent = nullptr);
    void setState(const QString& text, const QColor& colour, bool ledOn = true);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QString text;
    QColor colour;
    bool ledOn = true;
};

class SpriteView : public QWidget
{
public:
    SpriteView(SpriteId id, int scale, QWidget* parent = nullptr);
    void setSprite(SpriteId id) { sprite = id; update(); }
    void setSwap(Swap s) { swap = s; update(); }
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent*) override;

private:
    SpriteId sprite;
    int scale;
    Swap swap = Swap::None;
};

// 64 px bar at the top of every screen: back button + ESC, title, and room on the right (right()).
class ScreenHeader : public QWidget
{
    Q_OBJECT
public:
    explicit ScreenHeader(const QString& title, QWidget* parent = nullptr, const QString& backText = "MENU");
    void setTitle(const QString& t);
    QHBoxLayout* right() { return rightBox; }

signals:
    void back();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QLabel* titleLabel;
    QHBoxLayout* rightBox;
};

// 36 px bar at the bottom: key hints on the left, optional widgets on the right (right()).
class FooterHints : public QWidget
{
public:
    struct Hint { QStringList keys; QString text; };

    explicit FooterHints(QWidget* parent = nullptr);
    void setHints(const QVector<Hint>& h);
    QHBoxLayout* right() { return rightBox; }
    static int paintHints(class QPainter& p, int x, int y, const QVector<Hint>& hints);   // returns end x

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QVector<Hint> hints;
    QHBoxLayout* rightBox;
};

#endif
