// Virtual keyboard host for the widgets UI.
//
// Debian's Qt 6 QtVirtualKeyboard is built WITHOUT desktop integration, so the
// platform input context calls showInputPanel() and nothing appears unless the
// application hosts InputPanel itself. The Qt 5 build did this from the QML
// overlay (VirtualKeyboardPanel.qml, loaded by main.qml); this is the same
// trick, hosted in a QQuickWidget now that the shell is pure widgets.
//
// The wrapping Item exists so the widget can size itself to the keyboard:
// InputPanel derives its height from its width, and QQuickWidget's
// SizeViewToRootObject then gives the widget exactly that height.

import QtQuick
import QtQuick.VirtualKeyboard

Item {
    id: root

    // Width is set from C++ to the window width; height follows the keyboard.
    width: 800
    height: inputPanel.height

    InputPanel {
        id: inputPanel
        width: root.width
        anchors.bottom: parent.bottom
    }
}
