// PixelDialog.h - a modal dialog drawn inside the window: dims everything and shows a centred panel
// (plan 5.6). exec() runs a local event loop like QDialog::exec(). Esc / a click outside = cancel.
#ifndef PIXELDIALOG_H
#define PIXELDIALOG_H

#include <QEventLoop>
#include <QVector>
#include <QWidget>

#include "widgets/Pixel.h"

class QHBoxLayout;
class QVBoxLayout;

class PixelDialog : public QWidget
{
    Q_OBJECT
public:
    PixelDialog(QWidget* window, const QString& title, int width = 440);
    QVBoxLayout* body() { return content; }
    PixelButton* addButton(const QString& text, PixelButton::Kind kind, int result);
    void setCancelResult(int r) { cancelResult = r; }
    void setFocusButton(QWidget* w) { focusButton = w; }
    int exec();

    static bool isOpen();
    static bool confirmQuit(QWidget* window, bool progressLost);
    static void message(QWidget* window, const QString& title, const QString& text);

protected:
    void paintEvent(QPaintEvent*) override;
    void keyPressEvent(QKeyEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    bool eventFilter(QObject* o, QEvent* e) override;
    bool focusNextPrevChild(bool next) override;

private:
    void place(int percent);
    void finish(int r);

    Panel* panel;
    QVBoxLayout* content;
    QHBoxLayout* buttonRow;
    QVector<PixelButton*> buttons;
    QWidget* focusButton = nullptr;
    QEventLoop loop;
    int result = 0, cancelResult = 0, panelWidth;
};

#endif
