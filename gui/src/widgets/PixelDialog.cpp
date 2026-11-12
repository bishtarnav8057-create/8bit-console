#include "widgets/PixelDialog.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QPointer>
#include <QTimer>
#include <QVBoxLayout>

#include "Theme.h"

namespace
{
    int openCount = 0;
}

PixelDialog::PixelDialog(QWidget* window, const QString& title, int width)
    : QWidget(window), panelWidth(width)
{
    setFocusPolicy(Qt::StrongFocus);
    panel = new Panel(QString(), this);
    panel->setColours(Theme::bgPanel, Theme::lineStrong);
    QVBoxLayout* box = panel->body();
    box->setContentsMargins(24, 24, 24, 20);
    box->setSpacing(16);
    box->addWidget(Theme::text(title, Theme::pixel(16), Theme::textHi, panel));

    content = new QVBoxLayout();
    content->setSpacing(10);
    box->addLayout(content);

    buttonRow = new QHBoxLayout();
    buttonRow->setSpacing(8);
    buttonRow->addStretch(1);
    box->addSpacing(8);
    box->addLayout(buttonRow);

    window->installEventFilter(this);
    hide();
}

PixelButton* PixelDialog::addButton(const QString& text, PixelButton::Kind kind, int r)
{
    PixelButton* b = new PixelButton(text, kind, panel);
    buttonRow->addWidget(b);
    buttons.append(b);
    connect(b, &QPushButton::clicked, this, [this, r] { finish(r); });
    if (!focusButton) focusButton = b;
    return b;
}

bool PixelDialog::isOpen()
{
    return openCount > 0;
}

void PixelDialog::finish(int r)
{
    result = r;
    loop.quit();
}

int PixelDialog::exec()
{
    openCount++;
    setGeometry(parentWidget()->rect());
    raise();
    show();
    if (focusButton) focusButton->setFocus(); else setFocus();

    if (Prefs::animations)   // 3-step scale: 90 -> 96 -> 100 %
    {
        place(90);
        QPointer<PixelDialog> self(this);
        QTimer::singleShot(40, this, [self] { if (self) self->place(96); });
        QTimer::singleShot(80, this, [self] { if (self) self->place(100); });
    }
    else place(100);

    loop.exec();
    hide();
    openCount--;
    return result;
}

void PixelDialog::place(int percent)
{
    QSize s = panel->sizeHint();
    s.setWidth(std::max(panelWidth, s.width()));
    s = s * percent / 100;
    panel->setGeometry(QRect(QPoint((width() - s.width()) / 2, (height() - s.height()) / 2), s));
}

void PixelDialog::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), Theme::dim);
}

void PixelDialog::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) { finish(cancelResult); return; }
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)
    {
        if (PixelButton* b = qobject_cast<PixelButton*>(focusWidget())) { b->click(); return; }
    }
    if (e->key() == Qt::Key_Left || e->key() == Qt::Key_Right) { focusNextPrevChild(e->key() == Qt::Key_Right); return; }
    e->accept();   // nothing reaches the screen underneath
}

void PixelDialog::mousePressEvent(QMouseEvent* e)
{
    if (!panel->geometry().contains(e->position().toPoint())) finish(cancelResult);
}

bool PixelDialog::eventFilter(QObject* o, QEvent* e)
{
    if (o == parentWidget() && e->type() == QEvent::Resize && isVisible())
    {
        setGeometry(parentWidget()->rect());
        place(100);
    }
    return false;
}

// Tab and the arrow keys cycle through this dialog's own focusable children only
bool PixelDialog::focusNextPrevChild(bool next)
{
    QList<QWidget*> chain;
    for (QWidget* w : panel->findChildren<QWidget*>())
        if (w->focusPolicy() & Qt::TabFocus && w->isVisible() && w->isEnabled()) chain.append(w);
    if (chain.isEmpty()) return true;
    int i = chain.indexOf(focusWidget());
    i = (i + (next ? 1 : -1) + chain.size()) % chain.size();
    chain[i]->setFocus(Qt::TabFocusReason);
    return true;
}

bool PixelDialog::confirmQuit(QWidget* window, bool progressLost)
{
    PixelDialog d(window, "QUIT PIXEL-8?");
    QHBoxLayout* row = new QHBoxLayout();
    row->setSpacing(16);
    row->addWidget(new SpriteView(SpriteId::Power, 3), 0, Qt::AlignTop);
    QString text = "The machine will switch off.";
    if (progressLost) text += "\nYour game or program in progress will be lost.";
    QLabel* l = Theme::text(text, Theme::mono(14), Theme::textHi);
    l->setWordWrap(true);
    row->addWidget(l, 1);
    d.body()->addLayout(row);
    PixelButton* cancel = d.addButton("Cancel", PixelButton::Secondary, 0);
    d.addButton("Quit", PixelButton::Danger, 1);
    d.setFocusButton(cancel);
    return d.exec() == 1;
}

void PixelDialog::message(QWidget* window, const QString& title, const QString& text)
{
    PixelDialog d(window, title, 520);
    QLabel* l = Theme::text(text, Theme::mono(14), Theme::textHi);
    l->setWordWrap(true);
    l->setTextInteractionFlags(Qt::TextSelectableByMouse);
    d.body()->addWidget(l);
    d.addButton("OK", PixelButton::Primary, 0);
    d.exec();
}
