#ifndef ACTIVITYWATCH_H
#define ACTIVITYWATCH_H

#include <QEvent>
#include <QObject>
#include <QTimer>
#include <QWidget>

// Restarts the idle timer whenever a person does something.
//
// Installed on the application rather than on a widget, because the work
// happens in the page: a QWebEngineView delivers its input to a child widget
// of its own making, and a filter on the window would see almost none of it —
// a terminal would lock while somebody was using it, which is the one failure
// this feature cannot have.
//
// Only real input counts. Timers, paints and network replies are not a person,
// and a page that repaints on a poll would hold the terminal unlocked all day.
class ActivityWatch : public QObject {
    Q_OBJECT
public:
    ActivityWatch(QTimer* idleTimer, QWidget* lockScreen, QObject* parent = nullptr)
        : QObject(parent), m_idleTimer(idleTimer), m_lockScreen(lockScreen)
    {
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseMove:
        case QEvent::Wheel:
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
            // Typing a worker code is activity, but it must not postpone a
            // lock that is already showing: the timer is stopped while locked
            // and restarted on sign-on, so anything happening on the lock
            // screen is deliberately ignored.
            if (!m_lockScreen || !m_lockScreen->isVisible())
                m_idleTimer->start();
            break;
        default:
            break;
        }

        return QObject::eventFilter(watched, event);
    }

private:
    QTimer* m_idleTimer;
    QWidget* m_lockScreen;
};

#endif
