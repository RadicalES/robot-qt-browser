#ifndef VIRTUALKEYBOARDPANEL_H
#define VIRTUALKEYBOARDPANEL_H

#include <QQuickWidget>
#include <QQuickItem>
#include <QGuiApplication>
#include <QInputMethod>

// Hosts QtQuick.VirtualKeyboard's InputPanel inside the widgets shell.
//
// Debian's Qt 6 QtVirtualKeyboard has no desktop integration compiled in, so
// nothing creates a keyboard window on its own: the input context calls
// showInputPanel() and the panel simply does not exist. The application has to
// host InputPanel itself — the same workaround the Qt 5 QML overlay used.
//
// Shown and hidden from QInputMethod::visibleChanged, which is what
// showInputPanel()/hideInputPanel() drive, so focusing a web form field raises
// the keyboard and blurring it drops the keyboard away again.
class VirtualKeyboardPanel : public QQuickWidget {
    Q_OBJECT

public:
    explicit VirtualKeyboardPanel(QWidget* parent = nullptr)
        : QQuickWidget(parent)
    {
        // The widget takes its size from the QML root, whose height tracks the
        // keyboard. Sizing the other way round would squash the key rows.
        setResizeMode(QQuickWidget::SizeViewToRootObject);
        setAttribute(Qt::WA_AlwaysStackOnTop);
        setClearColor(Qt::transparent);
        setSource(QUrl(QStringLiteral("qrc:/qml/virtualkeyboard.qml")));
        hide();

        connect(QGuiApplication::inputMethod(), &QInputMethod::visibleChanged,
                this, &VirtualKeyboardPanel::onInputMethodVisibleChanged);
    }

    // Match the keyboard to the window width. InputPanel derives its height
    // from this, so the widget resizes to suit.
    void setPanelWidth(int width)
    {
        if (QQuickItem* root = rootObject())
            root->setWidth(width);
    }

private slots:
    void onInputMethodVisibleChanged()
    {
        setVisible(QGuiApplication::inputMethod()->isVisible());
    }
};

#endif
