#include "GameBoard.h"

#include <QColor>
#include <QFont>
#include <QPainter>
#include <algorithm>

GameBoard::GameBoard(QWidget* parent)
    : QWidget(parent), cells(Games::W * Games::H, EMPTY)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void GameBoard::clear()
{
    std::fill(cells.begin(), cells.end(), (int)EMPTY);
}

void GameBoard::setCell(int x, int y, int colour)
{
    if (x < 0 || x >= Games::W || y < 0 || y >= Games::H) return;
    cells[y * Games::W + x] = colour;
}

void GameBoard::setMessage(const QString& big, const QString& small)
{
    bigText = big;
    smallText = small;
}

void GameBoard::paintEvent(QPaintEvent*)
{
    QPainter p(this);

    // the biggest square cell that lets the whole board fit
    int cell = std::min(width() / Games::W, height() / Games::H);
    if (cell < 1) cell = 1;
    int boardW = cell * Games::W;
    int boardH = cell * Games::H;
    int left = (width() - boardW) / 2;
    int top = (height() - boardH) / 2;

    p.fillRect(rect(), QColor(40, 40, 40));
    p.fillRect(left, top, boardW, boardH, QColor(15, 20, 15));

    const QColor colours[6] = {
        QColor(15, 20, 15),      // EMPTY
        QColor(60, 190, 80),     // SNAKE_BODY
        QColor(170, 255, 120),   // SNAKE_HEAD
        QColor(230, 60, 60),     // FOOD
        QColor(240, 240, 240),   // WHITE
        QColor(70, 80, 70)       // GREY
    };

    for (int y = 0; y < Games::H; y++)
    {
        for (int x = 0; x < Games::W; x++)
        {
            int c = cells[y * Games::W + x];
            if (c == EMPTY) continue;
            p.fillRect(left + x * cell + 1, top + y * cell + 1, cell - 2, cell - 2, colours[c]);
        }
    }

    if (!bigText.isEmpty())
    {
        p.fillRect(left, top + boardH / 3, boardW, boardH / 3, QColor(0, 0, 0, 190));

        QFont big = font();
        big.setPixelSize(std::max(14, boardH / 10));
        big.setBold(true);
        p.setFont(big);
        p.setPen(Qt::white);
        p.drawText(QRect(left, top + boardH / 3, boardW, boardH / 6), Qt::AlignCenter, bigText);

        QFont small = font();
        small.setPixelSize(std::max(10, boardH / 25));
        p.setFont(small);
        p.setPen(QColor(200, 200, 200));
        p.drawText(QRect(left, top + boardH / 2, boardW, boardH / 6), Qt::AlignCenter, smallText);
    }
}
