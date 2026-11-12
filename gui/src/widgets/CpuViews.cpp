#include "widgets/CpuViews.h"

#include <QFontMetrics>
#include <QHelpEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>
#include <cmath>

#include "Theme.h"

// ---------------------------------------------------------------- BitRow
// cells 20 x 24, gap 2, gap 6 between the nibbles; the 9th cell sits left of the MSB with a gap 6

namespace { const int CW = 20, CH = 24; }

BitRow::BitRow(QWidget* parent) : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

QSize BitRow::sizeHint() const
{
    return QSize(cellX(0) + CW, CH);
}

int BitRow::cellX(int bit) const
{
    // bit 8 (carry) first, then 7..0
    int x = 0;
    if (ninth) { if (bit == 8) return 0; x = CW + 6; }
    const int pos = 7 - bit;              // 0 for the MSB
    return x + pos * (CW + 2) + (pos >= 4 ? 4 : 0);
}

int BitRow::cellAt(int x) const
{
    for (int b = ninth ? 8 : 7; b >= 0; b--)
        if (x >= cellX(b) && x < cellX(b) + CW) return b;
    return -1;
}

void BitRow::setValue(int v)
{
    if (v == val) return;
    val = v;
    update();
}

void BitRow::setEditable(bool e)
{
    editable = e;
    setCursor(e ? Qt::PointingHandCursor : Qt::ArrowCursor);
}

void BitRow::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setFont(Theme::pixel(8));
    for (int b = 0; b <= (ninth ? 8 : 7); b++)
    {
        const bool on = (val >> b) & 1;
        const QRect r(cellX(b), 0, CW, CH);
        QColor face = on ? (b == 8 ? Theme::red : Theme::green) : Theme::bgRaised;
        p.fillRect(r, face);
        p.setPen(on ? Theme::bgVoid : Theme::textDim);
        p.drawText(r, Qt::AlignCenter, b == 8 ? "C" : on ? "1" : "0");
    }
}

void BitRow::mousePressEvent(QMouseEvent* e)
{
    if (!editable) return;
    const int b = cellAt((int)e->position().x());
    if (b < 0 || b > 7) return;
    val ^= (1 << b);
    update();
    emit valueChanged(val);
}

bool BitRow::event(QEvent* e)
{
    if (e->type() == QEvent::ToolTip)
    {
        QHelpEvent* h = static_cast<QHelpEvent*>(e);
        const int b = cellAt(h->pos().x());
        if (b >= 0) QToolTip::showText(h->globalPos(), b == 8 ? "256: the 9th bit, does not fit in a byte"
                                                               : QString("bit %1 = %2").arg(b).arg(1 << b), this);
        else QToolTip::hideText();
        return true;
    }
    return QWidget::event(e);
}

// ---------------------------------------------------------------- RegisterView

namespace { const int RW = 72, RH = 52, RG = 8; }

RegisterView::RegisterView(int cols, QWidget* parent) : QWidget(parent), columns(cols)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    clock.start();
    changedAt.fill(-100000);
    fade.setSingleShot(true);
    connect(&fade, &QTimer::timeout, this, [this] { update(); });
}

QSize RegisterView::sizeHint() const
{
    const int rows = 8 / columns;
    return QSize(std::max(columns * RW + (columns - 1) * RG, 204), rows * (RH + RG) + 26);
}

void RegisterView::clear()
{
    blank = true;
    regs.fill(0);
    flags = 0;
    changedAt.fill(-100000);
    update();
}

void RegisterView::setRegisters(const std::array<Byte, 8>& r, Byte f)
{
    setRegisters(r, f, blank ? r : regs);
}

void RegisterView::setRegisters(const std::array<Byte, 8>& r, Byte f, const std::array<Byte, 8>& before)
{
    const qint64 now = clock.elapsed();
    for (int i = 0; i < 8; i++)
        if (r[i] != before[i]) changedAt[i] = now;
    regs = r;
    flags = f;
    blank = false;
    fade.start(620);
    update();
}

void RegisterView::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    const qint64 now = clock.elapsed();
    for (int i = 0; i < 8; i++)
    {
        const QRect r((i % columns) * (RW + RG), (i / columns) * (RH + RG), RW, RH);
        const bool glow = now - changedAt[i] < 600;
        Theme::drawBox(p, r, Theme::bgRaised, glow ? Theme::amber : Theme::line);
        p.setFont(Theme::label());
        p.setPen(Theme::textMid);
        p.drawText(r.adjusted(8, 4, 0, 0), Qt::AlignLeft | Qt::AlignTop, QString("R%1").arg(i));
        if (blank) continue;
        p.setFont(Theme::pixel(16));
        p.setPen(glow ? Theme::amber : Theme::textHi);
        p.drawText(r.adjusted(8, 18, 0, 0), Qt::AlignLeft | Qt::AlignTop,
                   QString("%1").arg(regs[i], 2, 16, QChar('0')).toUpper());
        p.setFont(Theme::mono(12));
        p.setPen(Theme::textDim);
        p.drawText(r.adjusted(0, 18, -8, 0), Qt::AlignRight | Qt::AlignTop, QString::number(regs[i]));
    }

    // flags: Z C N V (bit 0..3)
    const int y = (8 / columns) * (RH + RG) + 4;
    p.setFont(Theme::label());
    p.setPen(Theme::textMid);
    p.drawText(QRect(0, y, 60, 18), Qt::AlignVCenter | Qt::AlignLeft, "FLAGS");
    const char* names[4] = {"Z", "C", "N", "V"};
    int x = 60;
    for (int b = 0; b < 4; b++)
    {
        const bool on = !blank && ((flags >> b) & 1);
        p.fillRect(x, y + 4, 10, 10, on ? Theme::greenDeep : Theme::shadow);
        p.fillRect(x + 2, y + 6, 6, 6, on ? Theme::green : Theme::ledOff);
        p.setPen(on ? Theme::green : Theme::textDim);
        p.drawText(QRect(x + 14, y, 16, 18), Qt::AlignVCenter | Qt::AlignLeft, names[b]);
        x += 36;
    }
}

// ---------------------------------------------------------------- VramView

VramView::VramView(QWidget* parent) : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void VramView::setFrame(const std::array<Byte, 512>& px)
{
    pixels = px;
    update();
}

void VramView::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    const qreal dpr = devicePixelRatioF();
    const int frame = bezel ? Theme::BEZEL + 4 : 0;

    // the largest whole number of physical pixels per screen pixel that fits
    int phys = (int)std::floor(std::min((width() - 2 * frame) * dpr / 32, (height() - 2 * frame) * dpr / 16));
    if (phys < 1) phys = 1;
    const qreal w = 32 * phys / dpr, h = 16 * phys / dpr;
    const int left = (int)((width() - w) / 2), top = (int)((height() - h) / 2);
    const QRect inner(left, top, (int)std::ceil(w), (int)std::ceil(h));
    if (bezel) Theme::drawBezel(p, inner, power);

    // paint in physical pixels so every screen pixel is exactly phys x phys
    p.save();
    p.scale(1 / dpr, 1 / dpr);
    const int ox = (int)std::round(left * dpr), oy = (int)std::round(top * dpr);
    p.fillRect(ox, oy, 32 * phys, 16 * phys, Theme::screenOff);
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 32; x++)
            if (Byte v = pixels[y * 32 + x]) p.fillRect(ox + x * phys, oy + y * phys, phys, phys, Theme::vramColour(v));
    if (phys >= 8)   // the pixel grid, 1 physical pixel
    {
        for (int x = 1; x < 32; x++) p.fillRect(ox + x * phys, oy, 1, 16 * phys, Theme::screenGrid);
        for (int y = 1; y < 16; y++) p.fillRect(ox, oy + y * phys, 32 * phys, 1, Theme::screenGrid);
    }
    p.restore();
    if (Prefs::scanlines && phys >= 6) Theme::drawScanlines(p, inner);
}

// ---------------------------------------------------------------- ErrorList

ErrorList::ErrorList(QWidget* parent) : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

void ErrorList::setErrors(const QVector<AsmError>& e)
{
    errors = e;
    updateGeometry();
    update();
}

QSize ErrorList::sizeHint() const
{
    return QSize(360, std::min((int)errors.size(), 10) * 26 + 4);
}

void ErrorList::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    const int rows = std::min((int)errors.size(), height() / 26);
    for (int i = 0; i < rows; i++)
    {
        const AsmError& e = errors[i];
        const int y = i * 26;
        if (i == rows - 1 && rows < errors.size())
        {
            p.setFont(Theme::mono(13));
            p.setPen(Theme::textMid);
            p.drawText(QRect(0, y, width(), 24), Qt::AlignVCenter, QString("... and %1 more").arg(errors.size() - i));
            break;
        }
        p.setFont(Theme::pixel(8));
        p.setPen(Theme::red);
        p.drawText(QRect(0, y, 80, 24), Qt::AlignVCenter | Qt::AlignLeft, e.line > 0 ? QString("LINE %1").arg(e.line) : "FILE");
        p.setFont(Theme::mono(13));
        p.setPen(Theme::textHi);
        const QString msg = QFontMetrics(p.font()).elidedText(e.message, Qt::ElideRight, width() - 88);
        p.drawText(QRect(88, y, width() - 88, 24), Qt::AlignVCenter | Qt::AlignLeft, msg);
    }
}
