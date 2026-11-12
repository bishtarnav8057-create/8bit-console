#include "MainWindow.h"

#include <QAction>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>

#include "CalculatorTab.h"
#include "GameTab.h"

MainWindow::MainWindow()
{
    setWindowTitle("PIXEL-8 Console");

    calculatorTab = new CalculatorTab(this);
    gameTab = new GameTab(this);

    QTabWidget* tabs = new QTabWidget(this);
    tabs->addTab(calculatorTab, "Calculator");
    tabs->addTab(gameTab, "Arcade (Snake + Pong)");
    tabs->tabBar()->setFocusPolicy(Qt::NoFocus);     // so the arrow keys belong to the game
    setCentralWidget(tabs);

    QMenu* file = menuBar()->addMenu("&File");
    QAction* quit = file->addAction("E&xit");
    connect(quit, SIGNAL(triggered()), this, SLOT(close()));

    QMenu* help = menuBar()->addMenu("&Help");
    QAction* about = help->addAction("&About");
    connect(about, SIGNAL(triggered()), this, SLOT(about()));

    statusBar()->showMessage("Team NULL Pointers - PIXEL-8 8-bit emulator");
    resize(900, 720);
}

void MainWindow::about()
{
    QMessageBox::about(this, "About PIXEL-8 Console",
        "PIXEL-8: an educational 8-bit CPU emulator.\n\n"
        "Calculator tab: the answer is worked out by our CPU running calc.asm.\n"
        "Arcade tab: Snake and Pong.\n\n"
        "Team NULL Pointers - Graphic Era (Deemed to be University)");
}
