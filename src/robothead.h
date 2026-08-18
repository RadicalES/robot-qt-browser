#ifndef ROBOTHEAD_H
#define ROBOTHEAD_H

#include <QPixmap>
#include <QPainter>
#include <QColor>

// The Robot mascot head, drawn rather than shipped as an asset.
//
// robot-scada-client ships this as four 48px pixmaps (robot-head-{ok,warn,
// error,off}.png) for its desktop tray. The kiosk needs the same mark at two
// very different sizes — 48px in the toolbar, larger in the Info dialog — so it
// is drawn here instead: crisp at any size, tinted per state from one place,
// and no fifth copy of the artwork to keep in step.
namespace RobotHead {

// Geometry is authored on a 48x48 grid and scaled, so proportions hold.
inline QPixmap pixmap(int size, const QColor& colour)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(size / 48.0, size / 48.0);
    p.setPen(Qt::NoPen);

    // Antennae: stem plus tip, one either side.
    p.setBrush(colour);
    p.drawRoundedRect(QRectF(13.0, 7.0, 2.6, 7.0), 1.3, 1.3);
    p.drawRoundedRect(QRectF(32.4, 7.0, 2.6, 7.0), 1.3, 1.3);
    p.drawEllipse(QPointF(14.3, 6.2), 2.6, 2.6);
    p.drawEllipse(QPointF(33.7, 6.2), 2.6, 2.6);

    // Head.
    p.drawRoundedRect(QRectF(7.0, 12.0, 34.0, 30.0), 5.0, 5.0);

    // Face: eyes and mouth knocked out in a light tone, which reads on every
    // state colour and on both the grey toolbar and a light dialog.
    p.setBrush(QColor("#f5f5f5"));
    p.drawEllipse(QPointF(17.5, 24.0), 4.6, 4.6);
    p.drawEllipse(QPointF(30.5, 24.0), 4.6, 4.6);
    p.drawRoundedRect(QRectF(14.5, 33.0, 19.0, 5.0), 2.0, 2.0);

    return pm;
}

} // namespace RobotHead

#endif
