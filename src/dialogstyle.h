#ifndef DIALOGSTYLE_H
#define DIALOGSTYLE_H

#include <QDialog>
#include <QGuiApplication>
#include <QScreen>
#include <QRect>
#include <QEvent>
#include <QObject>
#include <QString>
#include <QAbstractButton>
#include <QLineEdit>
#include <QList>
#include <QWidget>
#include <QGuiApplication>
#include <QInputMethod>

// Shared look and sizing for the kiosk dialogs.
//
// Qt's defaults are desktop-sized: 12-13px text and ~30px buttons. On a
// 720x1280 touch panel with no mouse, that is hard to read and harder to hit,
// and a dialog that sizes itself to its contents ends up as a postage stamp in
// the middle of the screen.
//
// Widgets should set only their colours locally; font size and padding come
// from here, so the whole set stays consistent. A child stylesheet merges with
// the dialog's, so a button that sets just "background: ..." keeps these sizes.
namespace DialogStyle {

// Chrome scale, 0 < s <= 1.
//
// The dialogs are sized for a 7" panel: 20-26px text and 34px-tall buttons,
// because a finger has to hit them. When a device profile is drawn smaller
// than 1:1 on a developer's screen, that text has to come with it — otherwise
// the dialogs read larger relative to the window than they do on the terminal,
// which is the one thing a device preview must not do.
//
// Set once at startup, before any dialog is built.
inline qreal& scaleRef()
{
    static qreal value = 1.0;
    return value;
}

inline void setScale(qreal scale)
{
    // A floor, because a dialog nobody can read is not a better preview.
    scaleRef() = qBound(qreal(0.4), scale, qreal(1.0));
}

inline qreal scale() { return scaleRef(); }

// No title bar on any dialog, anywhere.
//
// These are modal dialogs that already carry their own Close, so minimise,
// maximise and close are duplicated at best. On a terminal they are worse than
// that: there is no window management and no way to reach a minimised dialog
// again, so the buttons offer an operator a way to lose the dialog and no way
// back. On a PC they are simply redundant.
//
// Applied by widthToScreen() and sizeToScreen(), which every dialog already
// calls, so a new dialog cannot forget to ask.
inline void applyWindowFlags(QDialog* dialog)
{
    dialog->setWindowFlags(dialog->windowFlags() | Qt::FramelessWindowHint);
}

// A device pixel value, at the current scale.
inline int px(int deviceValue)
{
    return qMax(1, int(qRound(deviceValue * scale())));
}

// The palette the Info dialog established: blue for the neutral/confirming
// action, orange for one that interrupts service, red for destructive.
namespace Colour {
inline QString primary()   { return QStringLiteral("background: #42a5f5; color: white;"); }
inline QString warning()   { return QStringLiteral("background: #ffa726; color: white;"); }
inline QString danger()    { return QStringLiteral("background: #ef5350; color: white;"); }
inline QString neutral()   { return QStringLiteral("background: white; color: #333;"
                                                   "border: 1px solid #999;"); }
}

inline QString sheet()
{
    return QString(
        "QLabel { font-size: %1px; }"
        "QPushButton { font-size: %1px; padding: %2px %3px; border-radius: 6px;"
        "              min-height: %4px; min-width: %5px; border: none; }"
        "QLineEdit { font-size: %6px; padding: %7px; border: 1px solid #bbb;"
        "            border-radius: 6px; }"
        "QRadioButton { font-size: %8px; padding: %7px 0px; }"
        "QRadioButton::indicator { width: %9px; height: %9px; }"
        "QListWidget { font-size: %1px; background: white; }"
        "QListWidget::item { padding: %10px %11px; }")
        .arg(px(20)).arg(px(14)).arg(px(24)).arg(px(34)).arg(px(90))
        .arg(px(22)).arg(px(12)).arg(px(26)).arg(px(36)).arg(px(16))
        .arg(px(8));
}

// What a dialog should size and position itself against.
//
// On a terminal that is the screen: the browser IS the screen, and a dialog
// across 94% of it is right. Off a terminal the browser is a window among
// others, and sizing to the screen gave an 1800px-wide dialog over an 800px
// window, spilling past it on both sides. The window a dialog belongs to is
// the honest reference in that case.
inline QRect hostRect(const QDialog* dialog)
{
    const QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
    const QWidget* parent = dialog->parentWidget();
    if (!parent)
        return screen;

    const QWidget* host = parent->window();
    if (!host || !host->isVisible())
        return screen;
    // Fullscreen means the kiosk: window and screen are the same thing, and
    // the screen is the more stable of the two to measure.
    if (host->isFullScreen() || host->isMaximized())
        return screen;

    return host->geometry();
}

// Re-applies the sizing when the dialog is shown.
//
// The dialogs are built at startup, before the main window exists on screen,
// so asking then which window they belong to gets the wrong answer — and these
// are long-lived objects that get shown again later, by which time the window
// may have been resized or moved to another screen. Measuring at show time is
// the only moment the answer is true.
//
// No Q_OBJECT: it overrides a virtual and needs no meta-object.
class HostSizeGuard : public QObject {
public:
    HostSizeGuard(QDialog* dialog, qreal widthFraction, qreal heightFraction)
        : QObject(dialog), m_width(widthFraction), m_height(heightFraction)
    {
        dialog->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    qreal m_width;
    qreal m_height;
};

// Width across most of the host, height left to the content. Use this for
// dialogs that are a few lines plus buttons — forcing a height fraction on
// those leaves a large empty gap above the buttons.
//
// Fractions rather than fixed pixels, so the dialogs keep working when the
// panel changes or the window is resized.
inline void widthToScreen(QDialog* dialog, qreal widthFraction)
{
    const QRect host = hostRect(dialog);
    dialog->setMinimumWidth(int(host.width() * widthFraction));
    dialog->setMaximumWidth(host.width());
    applyWindowFlags(dialog);
    new HostSizeGuard(dialog, widthFraction, 0.0);
}

// Width and height. Only for dialogs with a scrolling list that genuinely
// wants the room, such as the WiFi network picker.
inline void sizeToScreen(QDialog* dialog, qreal widthFraction, qreal heightFraction)
{
    const QRect host = hostRect(dialog);
    dialog->setMinimumSize(int(host.width() * widthFraction),
                           int(host.height() * heightFraction));
    dialog->setMaximumSize(host.width(), host.height());
    applyWindowFlags(dialog);
    new HostSizeGuard(dialog, widthFraction, heightFraction);
}

// Nothing in a kiosk dialog takes focus except its text fields.
//
// These dialogs are created once and reused, so any widget that takes focus
// keeps it after the dialog hides. The input method then stays pointed at a
// hidden dialog and the on-screen keyboard never appears again — observed
// first with a button, then with the WiFi network list, then with the dialog
// widget itself once the buttons were excluded. Allowing focus only where it
// is genuinely needed closes off the whole class.
//
// Controls still respond to taps: click handling does not require focus.
inline void takeNoFocusExceptFields(QDialog* dialog)
{
    dialog->setFocusPolicy(Qt::NoFocus);
    const QList<QWidget*> widgets = dialog->findChildren<QWidget*>();
    for (QWidget* widget : widgets) {
        if (qobject_cast<QLineEdit*>(widget))
            continue;   // the keyboard types into these
        widget->setFocusPolicy(Qt::NoFocus);
    }
}

// Centre a dialog on the window it belongs to, or on the screen in a kiosk.
// Qt's own centring is on the parent, which is unhelpful once a dialog has
// been clamped to the screen: it can still end up straddling an edge with half
// its controls unreachable. Call from showEvent, after the size is settled.
inline void centerOnScreen(QDialog* dialog)
{
    const QRect host = hostRect(dialog);
    QRect target(QPoint(0, 0), dialog->size());
    target.moveCenter(host.center());

    // Never off the screen, whatever the host says: a window dragged half off
    // the desktop must not take its dialogs with it.
    const QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
    if (target.right() > screen.right())   target.moveRight(screen.right());
    if (target.bottom() > screen.bottom()) target.moveBottom(screen.bottom());
    if (target.left() < screen.left())     target.moveLeft(screen.left());
    if (target.top() < screen.top())       target.moveTop(screen.top());

    dialog->move(target.topLeft());
}

inline bool HostSizeGuard::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::Show) {
        QDialog* dialog = qobject_cast<QDialog*>(watched);
        if (dialog) {
            const QRect host = hostRect(dialog);
            dialog->setMaximumWidth(host.width());
            dialog->setMinimumWidth(int(host.width() * m_width));
            if (m_height > 0.0) {
                dialog->setMaximumHeight(host.height());
                dialog->setMinimumHeight(int(host.height() * m_height));
            }
            dialog->adjustSize();
            centerOnScreen(dialog);
        }
    }
    return QObject::eventFilter(watched, event);
}

// Close the on-screen keyboard before opening a dialog.
//
// A dialog that opens over a raised keyboard leaves the input method in a
// state the panel never recovers from: the panel deactivates because focus
// moved, but the context is not told, so the next tap on a web input produces
// no showInputPanel() and the keyboard stays down for good. Committing first
// keeps any half-typed preedit text.
inline void closeKeyboard()
{
    QInputMethod* inputMethod = QGuiApplication::inputMethod();
    inputMethod->commit();
    inputMethod->hide();
}

} // namespace DialogStyle

#endif
