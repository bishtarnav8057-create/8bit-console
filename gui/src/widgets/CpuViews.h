// CpuViews.h - widgets that show machine state. They get plain data (bytes, arrays), never a Console
// or CPU object, so no widget can change the machine (plan 12.1).
#ifndef CPUVIEWS_H
#define CPUVIEWS_H

#include <QElapsedTimer>
#include <QTimer>
#include <QVector>
#include <QWidget>
#include <array>

#include "Programs.h"

// 8 bits, MSB on the left, optional 9th "carry" cell. Clickable when editable.
class BitRow : public QWidget
{
    Q_OBJECT
public:
    explicit BitRow(QWidget* parent = nullptr);
    void setValue(int v);                  // 0..511 (bit 8 only shown with setNinth)
    int value() const { return val; }
    void setEditable(bool e);
    void setNinth(bool show) { ninth = show; updateGeometry(); update(); }
    QSize sizeHint() const override;

signals:
    void valueChanged(int v);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* e) override;
    bool event(QEvent* e) override;

private:
    int cellAt(int x) const;               // bit number under x, or -1
    int cellX(int bit) const;
    int val = 0;
    bool editable = false, ninth = false;
};

// R0..R7 as cells (name, hex, decimal) in `columns` columns, then the Z C N V flag LEDs.
// A register that changed glows amber for 600 ms.
class RegisterView : public QWidget
{
public:
    explicit RegisterView(int columns = 4, QWidget* parent = nullptr, int cellWidth = 72);
    void setRegisters(const std::array<Byte, 8>& regs, Byte flags);
    // compare against `before` instead of the values shown now
    void setRegisters(const std::array<Byte, 8>& regs, Byte flags, const std::array<Byte, 8>& before);
    void clear();
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent*) override;

private:
    std::array<Byte, 8> regs{};
    Byte flags = 0;
    bool blank = true;
    std::array<qint64, 8> changedAt{};
    QElapsedTimer clock;
    QTimer fade;
    int columns, cellW;
};

// The PIXEL-8 screen: 32 x 16 VRAM bytes in a bezel, whole-number scale, palette of plan 5.3.
class VramView : public QWidget
{
public:
    explicit VramView(QWidget* parent = nullptr);
    void setFrame(const std::array<Byte, 512>& px);
    void setPower(bool on) { power = on; update(); }
    void setBezel(bool b) { bezel = b; update(); }
    QSize sizeHint() const override { return QSize(32 * 12 + 40, 16 * 12 + 40); }
    QSize minimumSizeHint() const override { return QSize(32 * 4 + 40, 16 * 4 + 40); }

protected:
    void paintEvent(QPaintEvent*) override;

private:
    std::array<Byte, 512> pixels{};
    bool power = true, bezel = true;
};

// "LINE 12  unknown register R9" rows, line number in red.
class ErrorList : public QWidget
{
public:
    explicit ErrorList(QWidget* parent = nullptr);
    void setErrors(const QVector<AsmError>& e);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QVector<AsmError> errors;
};

#endif
