// HomeScreen.h - the launcher (plan 7, 10.2): program cards on the left, a preview of the focused
// card on the right, Help/About/Settings/Exit at the bottom.
#ifndef HOMESCREEN_H
#define HOMESCREEN_H

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

    QVector<ProgramCard*> cards;
    PreviewPane* preview;
    int focused = 0;
};

#endif
