#ifndef DIALOGSTYLE_H
#define DIALOGSTYLE_H

#include <QDialog>
#include <QGuiApplication>
#include <QScreen>
#include <QString>

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

inline QString sheet()
{
    return QStringLiteral(
        "QLabel { font-size: 20px; }"
        "QPushButton { font-size: 20px; padding: 14px 24px; border-radius: 6px;"
        "              min-height: 34px; min-width: 120px; border: none; }"
        "QLineEdit { font-size: 22px; padding: 12px; border: 1px solid #bbb;"
        "            border-radius: 6px; }"
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
}

// Width and height. Only for dialogs with a scrolling list that genuinely
// wants the room, such as the WiFi network picker.
inline void sizeToScreen(QDialog* dialog, qreal widthFraction, qreal heightFraction)
{
    const QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
    dialog->setMinimumSize(int(screen.width() * widthFraction),
                           int(screen.height() * heightFraction));
}

} // namespace DialogStyle

#endif
