#ifndef PAGEFOCUSGUARD_H
#define PAGEFOCUSGUARD_H

#include <QApplication>
#include <QEvent>
#include <QPointer>
#include <QWidget>
#include <QDebug>

// Makes a tap on the web page always give the page keyboard focus.
//
// The on-screen keyboard only appears when the input method's focus object is
// the page's render widget. Anything else that takes focus and does not give it
// back — a dialog, a list inside one, the dialog widget itself — leaves taps on
// an input field querying the input method but never raising the keyboard.
//
// Rather than chase each widget that can strand focus, this reclaims it at the
// point of interaction: press anywhere on the page and the page gets focus,
// unless it already has it. Handles touch as well as mouse, since a touch panel
// delivers TouchBegin rather than MouseButtonPress.
class PageFocusGuard : public QObject {
    Q_OBJECT

public:
    PageFocusGuard(QWidget* view, QObject* parent = nullptr)
        : QObject(parent)
        , m_view(view)
    {
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::TouchBegin:
            reclaimFocus(qobject_cast<QWidget*>(watched));
            break;
        default:
            break;
        }
        return false;   // never consume: this only nudges focus
    }

private:
    void reclaimFocus(QWidget* target)
    {
        if (!m_view || !target)
            return;
        if (!belongsToPage(target))
            return;                     // tap landed on the toolbar or a dialog
        if (belongsToPage(QApplication::focusWidget()))
            return;                     // page already has focus, nothing to do

        qDebug() << "PageFocusGuard: reclaiming focus for the page from"
                << QApplication::focusWidget();
        m_view->setFocus(Qt::MouseFocusReason);
    }

    bool belongsToPage(QWidget* widget) const
    {
        return widget && m_view
            && (widget == m_view || m_view->isAncestorOf(widget));
    }

    QPointer<QWidget> m_view;
};

#endif
