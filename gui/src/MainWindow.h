// MainWindow.h - the application shell (plan 12): one QStackedWidget of screens, navigation,
// the dialogs (exit, about, settings), F1 / F11 and the pixel-wipe transition.
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QHash>
#include <QMainWindow>

#include "screens/Screen.h"

class QStackedWidget;
class ConsoleScreen;
class HelpScreen;
class PixelWipe;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    void go(ScreenId id);
    void saveShots(const QString& dir);     // --shots: a PNG of every screen

protected:
    void closeEvent(QCloseEvent* e) override;

private:
    Screen* current() const;
    void add(ScreenId id, Screen* s);
    void showHelp();
    void showAbout();
    void showSettings();
    void toggleFullScreen();

    QStackedWidget* stack;
    QHash<int, Screen*> screens;
    ScreenId currentId = ScreenId::Home;
    ScreenId beforeHelp = ScreenId::Home;
    HelpScreen* help;
    ConsoleScreen* consoleScreen;
    PixelWipe* wipe;
};

#endif
