#include "widgets/OverlayMenu.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>

#include "Theme.h"

namespace
{
    const int PAD = 24, TITLE_H = 40, LINE_H = 24, BIG_H = 30, ITEM_H = 36, HINT_H = 22;
}

OverlayMenu::OverlayMenu(QWidget* parent) : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    hide();
    blink.setInterval(450);
    connect(&blink, &QTimer::timeout, this, [this] { blinkOn = !blinkOn; update(); });
}

void OverlayMenu::open(const QString& t, const QColor& tc, const QVector<Line>& l, const QStringList& it,
                       const QVector<FooterHints::Hint>& h)
{
    title = t;
    titleColour = tc;
    lines = l;
    items = it;
    hints = h;
    selected = 0;
    blinkOn = true;
    blink.setInterval(Prefs::animations ? 450 : 900);   // blinks stay, at half speed, with animations off
    blink.start();
    show();
    raise();
    setFocus();
    update();
}

QRect OverlayMenu::panelRect() const
{
    int h = PAD + TITLE_H;
    for (const Line& l : lines) h += l.big ? BIG_H : LINE_H;
    if (!items.isEmpty()) h += 12 + items.size() * ITEM_H;
    if (!hints.isEmpty()) h += 16 + HINT_H;
    h += PAD;
    const int w = std::min(400, width() - 24);
    return QRect((width() - w) / 2, (height() - h) / 2, w, h);
}

QRect OverlayMenu::itemRect(int i) const
{
    const QRect p = panelRect();
    int y = p.y() + PAD + TITLE_H;
    for (const Line& l : lines) y += l.big ? BIG_H : LINE_H;
    y += 12 + i * ITEM_H;
    return QRect(p.x() + 16, y, p.width() - 32, ITEM_H - 4);
}

int OverlayMenu::itemAt(const QPoint& pos) const
{
    for (int i = 0; i < items.size(); i++)
        if (itemRect(i).contains(pos)) return i;
    return -1;
}

void OverlayMenu::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(Theme::bgVoid.red(), Theme::bgVoid.green(), Theme::bgVoid.blue(), 140));   // 55 %

    const QRect r = panelRect();
    Theme::fillNotched(p, r.translated(4, 4), Theme::shadow);
    Theme::fillNotched(p, r, Theme::lineStrong);
    QColor face = Theme::bgPanel;
    face.setAlpha(240);   // 94 %
    p.fillRect(r.adjusted(2, 2, -2, -2), face);

    int y = r.y() + PAD;
    p.setFont(Theme::pixel(24));
    p.setPen(titleColour);
    p.drawText(QRect(r.x(), y, r.width(), TITLE_H - 8), Qt::AlignHCenter | Qt::AlignTop, title);
    y += TITLE_H;

    for (const Line& l : lines)
    {
        const int lh = l.big ? BIG_H : LINE_H;
        if (!l.blink || blinkOn)
        {
            p.setFont(l.big ? Theme::pixel(16) : Theme::mono(14));
            p.setPen(l.colour);
            p.drawText(QRect(r.x(), y, r.width(), lh), Qt::AlignCenter, l.text);
        }
        y += lh;
    }

    if (!items.isEmpty()) y += 12;
    for (int i = 0; i < items.size(); i++)
    {
        const QRect ir = itemRect(i);
        const bool sel = i == selected;
        if (sel)
        {
            p.fillRect(ir, Theme::bgRaised);
            p.fillRect(ir.x(), ir.y(), 2, ir.height(), Theme::amber);
        }
        p.setFont(Theme::button());
        p.setPen(sel ? Theme::amber : Theme::textMid);
        p.drawText(ir.adjusted(36, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, items[i]);
        if (sel)
        {
            p.setFont(Theme::pixel(8));
            p.drawText(ir.adjusted(16, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, ">");
        }
        y += ITEM_H;
    }

    if (!hints.isEmpty())
    {
        y += 16;
        const int w = FooterHints::hintsWidth(hints);
        FooterHints::paintHints(p, r.x() + (r.width() - w) / 2, y, hints);
    }
}

void OverlayMenu::keyPressEvent(QKeyEvent* e)
{
    switch (e->key())
    {
    case Qt::Key_Up: case Qt::Key_W:
        if (!items.isEmpty()) { selected = (selected + items.size() - 1) % items.size(); update(); }
        return;
    case Qt::Key_Down: case Qt::Key_S:
        if (!items.isEmpty()) { selected = (selected + 1) % items.size(); update(); }
        return;
    case Qt::Key_Return: case Qt::Key_Enter:
        if (!items.isEmpty()) emit chosen(selected);
        return;
    default:
        e->ignore();   // P, Space, R, Esc: the screen decides
    }
}

void OverlayMenu::mouseMoveEvent(QMouseEvent* e)
{
    const int i = itemAt(e->position().toPoint());
    if (i >= 0 && i != selected) { selected = i; update(); }
    setCursor(i >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
}

void OverlayMenu::mouseReleaseEvent(QMouseEvent* e)
{
    const int i = itemAt(e->position().toPoint());
    if (i >= 0) emit chosen(i);
}
