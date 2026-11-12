#include <QApplication>
#include <cstdlib>
#include <ctime>

#include "MainWindow.h"

int main(int argc, char* argv[])
{
    std::srand((unsigned)std::time(nullptr));

    QApplication app(argc, argv);
    app.setStyle("Fusion");

    MainWindow window;
    window.show();
    return app.exec();
}
