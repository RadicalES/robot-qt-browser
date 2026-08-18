#ifndef DIALOGSTYLE_H
#define DIALOGSTYLE_H

#include <QDialog>
#include <QGuiApplication>
#include <QScreen>
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
    return QStringLiteral(
        "QLabel { font-size: 20px; }"
        "QPushButton { font-size: 20px; padding: 14px 24px; border-radius: 6px;"
        "              min-height: 34px; min-width: 90px; border: none; }"
        "QLineEdit { font-size: 22px; padding: 12px; border: 1px solid #bbb;"
        "            border-radius: 6px; }"
        "QRadioButton { font-size: 26px; padding: 12px 0px; }"
        "QRadioButton::indicator { width: 36px; height: 36px; }"
        "QListWidget { font-size: 20px; background: white; }"
        "QListWidget::item { padding: 16px 8px; }");
}

// Width across most of the screen, height left to the content. Use this for
// dialogs that are a few lines plus buttons — forcing a height fraction on
// those leaves a large empty gap above the buttons.
//
// Fractions are of the screen rather than fixed pixels, so the dialogs keep
// working if the panel changes.
inline void widthToScreen(QDialog* dialog, qreal widthFraction)
{
    const QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
    dialog->setMinimumWidth(int(screen.width() * widthFraction));
    dialog->setMaximumWidth(screen.width());
}

// Width and height. Only for dialogs with a scrolling list that genuinely
// wants the room, such as the WiFi network picker.
inline void sizeToScreen(QDialog* dialog, qreal widthFraction, qreal heightFraction)
{
    const QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
    dialog->setMinimumSize(int(screen.width() * widthFraction),
                           int(screen.height() * heightFraction));
    dialog->setMaximumSize(screen.width(), screen.height());
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

// Centre a dialog on the screen. Qt centres on the parent, which is unhelpful
// once a dialog has been clamped to the screen: it can still end up straddling
// an edge with half its controls unreachable. Call from showEvent, after the
// size is settled.
inline void centerOnScreen(QDialog* dialog)
{
    const QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
    dialog->move(screen.center() - dialog->rect().center());
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
