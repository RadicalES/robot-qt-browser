#ifndef VIRTUALKEYBOARDPANEL_H
#define VIRTUALKEYBOARDPANEL_H

#include <QQuickWidget>
#include <QQuickItem>
#include <QMetaProperty>
#include <QDebug>

// Hosts QtQuick.VirtualKeyboard's InputPanel inside the widgets shell.
//
// Debian's Qt 6 QtVirtualKeyboard has no desktop integration compiled in, so
// nothing creates a keyboard window on its own: the input context calls
// showInputPanel() and the panel simply does not exist. The application has to
// host InputPanel itself — the same workaround the Qt 5 QML overlay used.
//
// Show/hide follows the QML root's keyboardActive property, which mirrors
// InputPanel.active. QInputMethod::visibleChanged looks like the obvious signal
// to use but does not track this panel, and keying off it left the keyboard
// failing to pop up when a web input field took focus.
class VirtualKeyboardPanel : public QQuickWidget {
    Q_OBJECT

public:
    explicit VirtualKeyboardPanel(QWidget* parent = nullptr)
        : QQuickWidget(parent)
    {
        // The widget takes its size from the QML root, whose height tracks the
        // keyboard and collapses to zero when inactive. Sizing the other way
        // round would squash the key rows.
        setResizeMode(QQuickWidget::SizeViewToRootObject);
        setAttribute(Qt::WA_AlwaysStackOnTop);
        // Never take Qt focus. A QQuickWidget grabs focus when clicked, which
        // would pull it off the web view being typed into — the input panel
        // then deactivates and the keyboard vanishes on the first keypress.
        setFocusPolicy(Qt::NoFocus);
        setClearColor(Qt::transparent);
        setSource(QUrl(QStringLiteral("qrc:/qml/virtualkeyboard.qml")));
        hide();

        // Bind to the QML property's notify signal through the meta-object.
        // A SIGNAL("keyboardActiveChanged()") string does not match the signal
        // QML generates, so that connection silently fails and the keyboard
        // never appears.
        QQuickItem* root = rootObject();
        if (!root) {
            qWarning() << "VirtualKeyboardPanel: QML root failed to load —"
                       << "no on-screen keyboard";
            return;
        }
        const QMetaObject* rootMeta = root->metaObject();
        const int propIndex = rootMeta->indexOfProperty("keyboardActive");
        const int slotIndex =
            metaObject()->indexOfSlot("onKeyboardActiveChanged()");
        if (propIndex < 0 || slotIndex < 0
            || !rootMeta->property(propIndex).hasNotifySignal()) {
            qWarning() << "VirtualKeyboardPanel: cannot track keyboardActive —"
                       << "no on-screen keyboard";
            return;
        }
        connect(root, rootMeta->property(propIndex).notifySignal(),
                this, metaObject()->method(slotIndex));
    }

    // Match the keyboard to the window width. InputPanel derives its height
    // from this, so the widget resizes to suit.
    void setPanelWidth(int width)
    {
        if (QQuickItem* root = rootObject())
            root->setWidth(width);
    }

private slots:
    void onKeyboardActiveChanged()
    {
        QQuickItem* root = rootObject();
        setVisible(root && root->property("keyboardActive").toBool());
    }
};

#endif
