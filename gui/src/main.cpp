#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QIcon>
#include <QPalette>
#include <cstdlib>
#include <ctime>

#include "MainWindow.h"
#include "Theme.h"
#include "art/SpriteCache.h"

namespace
{
    void loadFonts()
    {
        for (const char* f : {":/fonts/PressStart2P-Regular.ttf", ":/fonts/JetBrainsMono-Regular.ttf",
                              ":/fonts/JetBrainsMono-Bold.ttf"})
            if (QFontDatabase::addApplicationFont(f) < 0) qWarning("font failed to load: %s", f);
    }

    // dark palette, so native bits (file dialog contents, context menus) are not white
    QPalette darkPalette()
    {
        QPalette p;
        p.setColor(QPalette::Window, Theme::bgBase);
        p.setColor(QPalette::WindowText, Theme::textHi);
        p.setColor(QPalette::Base, Theme::bgBase);
        p.setColor(QPalette::AlternateBase, Theme::rowAlt);
        p.setColor(QPalette::Text, Theme::textHi);
        p.setColor(QPalette::Button, Theme::bgRaised);
        p.setColor(QPalette::ButtonText, Theme::textHi);
        p.setColor(QPalette::Highlight, Theme::amber);
        p.setColor(QPalette::HighlightedText, Theme::amberInk);
        p.setColor(QPalette::ToolTipBase, Theme::bgPanel);
        p.setColor(QPalette::ToolTipText, Theme::textHi);
        p.setColor(QPalette::PlaceholderText, Theme::textDim);
        p.setColor(QPalette::Disabled, QPalette::Text, Theme::textDim);
        p.setColor(QPalette::Disabled, QPalette::WindowText, Theme::textDim);
        p.setColor(QPalette::Disabled, QPalette::ButtonText, Theme::textDim);
        return p;
    }

    QIcon appIcon()
    {
        QIcon icon;
        for (int k : {1, 2, 3, 4, 8, 16}) icon.addPixmap(SpriteCache::pixmap(SpriteId::Logo, k));
        return icon;
    }
}

int main(int argc, char* argv[])
{
    std::srand((unsigned)std::time(nullptr));

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("NULL Pointers");
    QCoreApplication::setApplicationName("PIXEL-8");
    QCoreApplication::setApplicationVersion(APP_VERSION);

    app.setStyle("Fusion");   // the base style that obeys QSS and the palette the same way everywhere
    loadFonts();
    app.setPalette(darkPalette());
    app.setFont(Theme::mono(14));
    QFile qss(":/theme/theme.qss");
    if (qss.open(QIODevice::ReadOnly)) app.setStyleSheet(QString::fromUtf8(qss.readAll()));
    app.setWindowIcon(appIcon());
    Prefs::load();

    MainWindow window;
    window.show();

    // --shots DIR: save a PNG of the window and quit (for the report and for checking without a screen)
    const int at = app.arguments().indexOf("--shots");
    if (at > 0 && at + 1 < app.arguments().size())
    {
        window.resize(1280, 800);
        QApplication::processEvents();
        window.grab().save(app.arguments().at(at + 1) + "/window.png");
        return 0;
    }
    return app.exec();
}
