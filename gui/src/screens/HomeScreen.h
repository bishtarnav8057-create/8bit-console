// HomeScreen.h - the launcher (plan 7, 10.2): program cards on the left, a preview of the focused
// card on the right, Help/About/Settings/Exit at the bottom.
#ifndef HOMESCREEN_H
#define HOMESCREEN_H

#include <QPoint>
#include <QVector>

#include "art/Sprites.h"
#include "screens/Screen.h"

class ProgramCard;
class PreviewPane;

class HomeScreen : public Screen
{
    Q_OBJECT
public:
    explicit HomeScreen(QWidget* parent = nullptr);
    void enter() override;
    void leave() override;
    void suspend() override;
    void runPreview(int card, int steps);    // for --shots

signals:
    void helpRequested();
    void aboutRequested();
    void settingsRequested();
    void exitRequested();

protected:
    void keyPressEvent(QKeyEvent* e) override;

private:
    void focusCard(int i);
    void launch(int i);
    int cardInRow(int step) const;           // up/down: +1 row below, -1 row above (wraps)

    QVector<ProgramCard*> cards;
    QVector<QPoint> cells;                   // each card's place on screen: x = column, y = row across sections
    PreviewPane* preview;
    int focused = 0;
};

#endif
