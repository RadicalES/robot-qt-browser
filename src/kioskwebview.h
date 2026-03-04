#ifndef KIOSKWEBVIEW_H
#define KIOSKWEBVIEW_H

#include <qwebview.h>
#include <QMouseEvent>

// QWebView subclass that suppresses drag-and-drop on linuxfb.
// Debian's QtWebKit 5.212 has ENABLE_DRAG_SUPPORT=ON, which causes
// broken drag images on linuxfb (pixel artifacts, no-go cursor).
//
// Strategy: intercept mouse-move events in event() and suppress them
// when a button is held down. This prevents WebCore's EventHandler
// from detecting a drag gesture. Text selection is not needed in kiosk.
class KioskWebView : public QWebView {
    Q_OBJECT
public:
    explicit KioskWebView(QWidget* parent = nullptr) : QWebView(parent) {
        setAcceptDrops(false);
    }

protected:
    bool event(QEvent* e) override {
        switch (e->type()) {
        case QEvent::MouseMove: {
            QMouseEvent* me = static_cast<QMouseEvent*>(e);
            if (me->buttons() != Qt::NoButton) {
                e->accept();
                return true; // eat mouse-move-with-button → no drag
            }
            break;
        }
        case QEvent::DragEnter:
        case QEvent::DragMove:
        case QEvent::DragLeave:
        case QEvent::Drop:
            return true; // eat all drag events
        default:
            break;
        }
        return QWebView::event(e);
    }
};

#endif
