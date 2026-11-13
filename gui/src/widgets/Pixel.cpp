#include "widgets/Pixel.h"

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>

#include "Theme.h"
#include "art/SpriteCache.h"

// ---------------------------------------------------------------- PixelButton
// The widget rect = 3 px focus margin + face + 4 px shadow + 3 px margin.

PixelButton::PixelButton(const QString& text, Kind k, QWidget* parent)
    : QPushButton(text, parent), kind(k)
{
    setAttribute(Qt::WA_Hover);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

QSize PixelButton::sizeHint() const
{
    int w = QFontMetrics(Theme::button()).horizontalAdvance(text()) + 40;
    if (glyph >= 0) w += 16 + (text().isEmpty() ? 0 : 8);
    if (trailing >= 0) w += 16 + 8;
    if (text().isEmpty()) w = std::max(w - 24, height + 0);
    return QSize(w + 10, (int)height + 10);
}

void PixelButton::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    const bool hover = underMouse() && isEnabled();
    const bool down = isDown() && isEnabled();
    const bool checked = isCheckable() && isChecked();

    QRect face = rect().adjusted(3, 3, -7, -7);
    int shadow = (kind == Ghost) ? 0 : 4;
    if (down) { face.translate(2, 2); shadow = shadow ? 2 : 0; }

    QColor fill, border, ink;
    if (!isEnabled())
    {
        fill = Theme::bgPanel; border = Theme::line; ink = Theme::textDim; shadow = 0;
    }
    else if (checked)
    {
        fill = hover ? Theme::amberHover : Theme::amber; border = Theme::amberBorder; ink = Theme::amberInk;
    }
    else switch (kind)
    {
    case Primary:
        fill = hover ? Theme::amberHover : Theme::amber; border = Theme::amberBorder; ink = Theme::amberInk;
        break;
    case Danger:
        fill = Theme::red; border = hover ? Theme::textHi : Theme::redBorder; ink = Theme::redInk;
        break;
    case Ghost:
        fill = hover ? Theme::bgPanel : QColor(); border = fill; ink = hover ? Theme::textHi : Theme::textMid;
        break;
    default:
        fill = hover ? Theme::bgRaisedHover : Theme::bgRaised;
        border = hover ? Theme::amber : Theme::lineStrong;
        ink = Theme::textHi;
    }

    if (fill.isValid()) Theme::drawBox(p, face, fill, border, shadow);

    if (hasFocus())   // 2 px amber ring around everything
    {
        QRect r = rect();
        p.fillRect(r.left(), r.top(), r.width(), 2, Theme::amber);
        p.fillRect(r.left(), r.bottom() - 1, r.width(), 2, Theme::amber);
        p.fillRect(r.left(), r.top(), 2, r.height(), Theme::amber);
        p.fillRect(r.right() - 1, r.top(), 2, r.height(), Theme::amber);
    }

    // content: [glyph] text [trailing glyph], centred
    const QFont f = Theme::button();
    const int tw = QFontMetrics(f).horizontalAdvance(text());
    int total = tw;
    if (glyph >= 0) total += 16 + (text().isEmpty() ? 0 : 8);
    if (trailing >= 0) total += 24;
    int x = face.center().x() - total / 2 + 1;
    const int cy = face.center().y();
    if (glyph >= 0)
    {
        SpriteCache::draw(p, (SpriteId)glyph, QPointF(x, cy - 7), 2, glyphRot, Swap::None, ink);
        x += 16 + (text().isEmpty() ? 0 : 8);
    }
    p.setFont(f);
    p.setPen(ink);
    p.drawText(QRect(x, face.top(), tw + 4, face.height()), Qt::AlignVCenter | Qt::AlignLeft, text());
    x += tw + 8;
    if (trailing >= 0) SpriteCache::draw(p, (SpriteId)trailing, QPointF(x, cy - 7), 2, 0, Swap::None, ink);
}

// ---------------------------------------------------------------- Panel

Panel::Panel(const QString& t, QWidget* parent)
    : QFrame(parent), face(Theme::bgPanel), border(Theme::line), accent(Theme::amber)
{
    box = new QVBoxLayout(this);
    box->setSpacing(12);
    setTitle(t);
}

void Panel::setTitle(const QString& t)
{
    title = t;
    box->setContentsMargins(16, title.isEmpty() ? 16 : 44, 16, 16);
    update();
}

void Panel::setColours(const QColor& f, const QColor& b)
{
    face = f;
    border = b;
    update();
}

void Panel::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    Theme::drawBox(p, rect(), face, border);
    if (title.isEmpty()) return;
    p.fillRect(16, 20, 6, 6, accent);
    p.setFont(Theme::label());
    p.setPen(Theme::textMid);
    p.drawText(QRect(30, 12, width() - 46, 22), Qt::AlignVCenter | Qt::AlignLeft, title);
}

// ---------------------------------------------------------------- KeyCap

KeyCap::KeyCap(const QString& k, QWidget* parent) : QWidget(parent), key(k)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

QSize KeyCap::sizeHint() const
{
    return QSize(Theme::keyCapWidth(key), Theme::KEYCAP_H);
}

void KeyCap::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    Theme::drawKeyCap(p, 0, 0, key);
}

// ---------------------------------------------------------------- Led

Led::Led(const QColor& onColour, QWidget* parent) : QWidget(parent), colour(onColour)
{
    setFixedSize(10, 10);
}

void Led::setOn(bool on)
{
    if (on == lit) return;
    lit = on;
    update();
}

void Led::paintLed(QPainter& p, int x, int y, bool on, const QColor& c)
{
    p.fillRect(x, y, 10, 10, on ? c.darker(170) : Theme::shadow);
    p.fillRect(x + 2, y + 2, 6, 6, on ? c : Theme::ledOff);
}

void Led::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    paintLed(p, 0, 0, lit, colour);
}

// ---------------------------------------------------------------- StatusChip

StatusChip::StatusChip(QWidget* parent) : QWidget(parent), colour(Theme::textMid)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void StatusChip::setState(const QString& t, const QColor& c, bool on)
{
    text = t;
    colour = c;
    ledOn = on;
    updateGeometry();
    update();
}

QSize StatusChip::sizeHint() const
{
    return QSize(18 + QFontMetrics(Theme::label()).horizontalAdvance(text) + 4, 22);
}

void StatusChip::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    Led::paintLed(p, 0, 6, ledOn, colour);
    p.setFont(Theme::label());
    p.setPen(colour);
    p.drawText(QRect(18, 0, width() - 18, height()), Qt::AlignVCenter | Qt::AlignLeft, text);
}

// ---------------------------------------------------------------- SpriteView

SpriteView::SpriteView(SpriteId id, int s, QWidget* parent) : QWidget(parent), sprite(id), scale(s)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

QSize SpriteView::sizeHint() const
{
    const Sprite& s = ::sprite(sprite);
    return QSize(s.w * scale, s.h * scale);
}

void SpriteView::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    const Sprite& s = ::sprite(sprite);
    SpriteCache::draw(p, sprite, QPointF((width() - s.w * scale) / 2, (height() - s.h * scale) / 2), scale, 0, swap);
}

// ---------------------------------------------------------------- ScreenHeader

ScreenHeader::ScreenHeader(const QString& title, QWidget* parent, const QString& backText) : QWidget(parent)
{
    setFixedHeight(64);
    QHBoxLayout* row = new QHBoxLayout(this);
    row->setContentsMargins(12, 0, 24, 2);
    row->setSpacing(8);

    PixelButton* backButton = new PixelButton(backText, PixelButton::Ghost, this);
    backButton->setHeightKind(PixelButton::Small);
    backButton->setGlyph(SpriteId::Back);
    backButton->setFocusPolicy(Qt::NoFocus);     // keys stay with the screen; Esc does the same
    connect(backButton, &QPushButton::clicked, this, &ScreenHeader::back);
    row->addWidget(backButton);
    row->addWidget(new KeyCap("ESC", this));
    row->addSpacing(24);

    titleLabel = Theme::text(title, Theme::pixel(16), Theme::textHi, this);
    row->addWidget(titleLabel);
    row->addStretch(1);

    rightBox = new QHBoxLayout();
    rightBox->setSpacing(24);
    row->addLayout(rightBox);
}

void ScreenHeader::setTitle(const QString& t)
{
    titleLabel->setText(t);
}

void ScreenHeader::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), Theme::bgPanel);
    p.fillRect(0, height() - 2, width(), 2, Theme::line);
}

// ---------------------------------------------------------------- FooterHints

FooterHints::FooterHints(QWidget* parent) : QWidget(parent)
{
    setFixedHeight(44);
    rightBox = new QHBoxLayout(this);
    rightBox->setContentsMargins(0, 2, 16, 0);
    rightBox->setSpacing(4);
    rightBox->addStretch(1);
}

void FooterHints::setHints(const QVector<Hint>& h)
{
    hints = h;
    update();
}

int FooterHints::paintHints(QPainter& p, int x, int y, const QVector<Hint>& hints)
{
    for (const Hint& h : hints)
    {
        for (const QString& k : h.keys) x += Theme::drawKeyCap(p, x, y, k) + 4;
        x += 4;
        p.setFont(Theme::label());
        p.setPen(Theme::textMid);
        const int tw = QFontMetrics(Theme::label()).horizontalAdvance(h.text);
        p.drawText(QRect(x, y, tw + 4, Theme::KEYCAP_H - 4), Qt::AlignVCenter | Qt::AlignLeft, h.text);
        x += tw + 24;
    }
    return x;
}

int FooterHints::hintsWidth(const QVector<Hint>& hints)
{
    int x = 0;
    for (const Hint& h : hints)
    {
        for (const QString& k : h.keys) x += Theme::keyCapWidth(k) + 4;
        x += 4 + QFontMetrics(Theme::label()).horizontalAdvance(h.text) + 24;
    }
    return std::max(0, x - 24);
}

void FooterHints::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), Theme::bgBase);
    p.fillRect(0, 0, width(), 2, Theme::line);
    paintHints(p, 24, (height() - Theme::KEYCAP_H) / 2 + 1, hints);
}
