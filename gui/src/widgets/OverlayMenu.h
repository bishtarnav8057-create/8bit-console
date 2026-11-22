// OverlayMenu.h - the READY / PAUSED / GAME OVER card over a game board (plan 8.3, 10.6, 10.7).
// A real widget, so its items work with the mouse too. Up/Down choose, Enter picks; other keys go on
// to the screen underneath.
#ifndef OVERLAYMENU_H
#define OVERLAYMENU_H

#include <QStringList>
#include <QTimer>
#include <QVector>
#include <QWidget>

#include "widgets/Pixel.h"

class OverlayMenu : public QWidget
{
    Q_OBJECT
public:
    struct Line
    {
        QString text;
        QColor colour;
        bool big = false;     // Press Start 16 instead of the mono body font
        bool blink = false;
    };

    explicit OverlayMenu(QWidget* parent);
    void open(const QString& title, const QColor& titleColour, const QVector<Line>& lines,
              const QStringList& items, const QVector<FooterHints::Hint>& hints = {});

signals:
    void chosen(int index);

protected:
    void paintEvent(QPaintEvent*) override;
    void keyPressEvent(QKeyEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private:
    QRect panelRect() const;
    QRect itemRect(int i) const;
    int itemAt(const QPoint& pos) const;

    QString title;
    QColor titleColour;
    QVector<Line> lines;
    QStringList items;
    QVector<FooterHints::Hint> hints;
    int selected = 0;
    bool blinkOn = true;
    QTimer blink;
};

#endif
