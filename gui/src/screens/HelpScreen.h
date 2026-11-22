// HelpScreen.h - F1 from anywhere: controls for every screen, the key bridge and the memory map.
#ifndef HELPSCREEN_H
#define HELPSCREEN_H

#include "screens/Screen.h"

class QListWidget;
class QStackedWidget;

class HelpScreen : public Screen
{
    Q_OBJECT
public:
    explicit HelpScreen(QWidget* parent = nullptr);
    void showFor(ScreenId from);    // open on the section of the screen you came from
    void enter() override;

protected:
    void keyPressEvent(QKeyEvent* e) override;

private:
    QListWidget* list;
    QStackedWidget* pages;
};

#endif
