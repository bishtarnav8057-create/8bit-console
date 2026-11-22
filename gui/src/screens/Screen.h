// Screen.h - what MainWindow needs from every screen (plan 12.3). The only new abstraction.
#ifndef SCREEN_H
#define SCREEN_H

#include <QWidget>

enum class ScreenId { Boot, Home, Calculator, Console, Snake, Pong, Help };

class Screen : public QWidget
{
    Q_OBJECT
public:
    using QWidget::QWidget;
    virtual void enter() {}                                   // became visible: take focus, start what runs
    virtual void leave() {}                                   // about to be hidden: stop timers, release keys
    virtual void suspend() {}                                 // a dialog is opening: pause
    virtual bool hasProgressToLose() const { return false; }  // changes the exit dialog wording

signals:
    void navigate(ScreenId target);
    void back();                                              // Esc / the header's back button
};

#endif
