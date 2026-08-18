#ifndef ROBOTHEAD_H
#define ROBOTHEAD_H

#include <QPixmap>
#include <QIcon>

// The Robot mascot head, in the three states the kiosk can be in.
//
// Traced from the 48px tray pixmaps robot-scada-client ships so the kiosk and
// the desktop show the same mark. Kept as SVG because the toolbar wants 48px
// and the dialogs want more, and the pixmaps have no size to spare — the SVG
// icon engine renders at whatever size is asked for.
namespace RobotHead {

enum Variant {
    Off,    // no server or service detected
    Warn,   // terminal not provisioned
    Ok      // talking to a SCADA server
};

inline QPixmap pixmap(int size, Variant variant)
{
    const char* name = variant == Ok   ? ":/images/robot-head-ok.svg"
                     : variant == Warn ? ":/images/robot-head-warn.svg"
                                       : ":/images/robot-head-off.svg";
    return QIcon(QString::fromLatin1(name)).pixmap(size, size);
}

} // namespace RobotHead

#endif
