#ifndef OVERLAYEVENTFILTER_H
#define OVERLAYEVENTFILTER_H

#include <QObject>
#include <QEvent>
#include <QMouseEvent>
#include <QQuickWidget>
#include <QQuickItem>
#include <QWebView>
#include <QApplication>

// Forwards mouse/keyboard/wheel events from the QML overlay to the QWebView
// when they land in the transparent pass-through area.
// The QML overlay handles events only in the bottom bar (44px) and open popups.
class OverlayEventFilter : public QObject {
    Q_OBJECT
public:
    OverlayEventFilter(QQuickWidget* overlay, QWebView* webView, QObject* parent = nullptr)
        : QObject(parent), m_overlay(overlay), m_webView(webView) {}

protected:
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        if (obj != m_overlay)
            return false;

        switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove: {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            if (shouldForward(me->pos())) {
                QMouseEvent fwd(me->type(), me->pos(), me->globalPos(),
                                me->button(), me->buttons(), me->modifiers());
                QApplication::sendEvent(m_webView, &fwd);
                if (me->type() == QEvent::MouseButtonPress)
                    m_webView->setFocus();
                return true;
            }
            break;
        }
        case QEvent::KeyPress:
        case QEvent::KeyRelease: {
            if (m_webView->hasFocus()) {
                QApplication::sendEvent(m_webView, event);
                return true;
            }
            break;
        }
        case QEvent::Wheel: {
            QWheelEvent* we = static_cast<QWheelEvent*>(event);
            QPointF pos = we->position();
            if (shouldForward(pos.toPoint())) {
                QApplication::sendEvent(m_webView, event);
                return true;
            }
            break;
        }
        default:
            break;
        }
        return false;
    }

private:
    bool shouldForward(const QPoint& pos)
    {
        QQuickItem* root = m_overlay->rootObject();
        if (!root) return true;

        // Bottom bar region: QML handles it
        int barTop = m_overlay->height() - 44;
        if (pos.y() >= barTop)
            return false;

        // If any popup is open, QML handles all input
        if (root->property("popupOpen").toBool())
            return false;

        // Check what QML item is at this position
        QQuickItem* itemAtPos = root->childAt(pos.x(), pos.y());

        // If nothing or just the root — forward to webview
        if (!itemAtPos || itemAtPos == root)
            return true;

        // Check if the item is the transparent pass-through spacer (first child)
        QList<QQuickItem*> children = root->childItems();
        if (!children.isEmpty() && itemAtPos == children.first())
            return true;

        // Something else (virtual keyboard, etc.) — let QML handle it
        return false;
    }

    QQuickWidget* m_overlay;
    QWebView* m_webView;
};

#endif
