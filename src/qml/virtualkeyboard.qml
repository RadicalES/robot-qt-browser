// Virtual keyboard host for the widgets UI.
//
// Debian's Qt 6 QtVirtualKeyboard is built WITHOUT desktop integration, so the
// platform input context calls showInputPanel() and nothing appears unless the
// application hosts InputPanel itself. The Qt 5 build did this from the QML
// overlay (VirtualKeyboardPanel.qml, loaded by main.qml); this is the same
// trick, hosted in a QQuickWidget now that the shell is pure widgets.
//
// Visibility is driven by InputPanel.active — the same binding the Qt 5 panel
// used. QInputMethod::visibleChanged is NOT a reliable substitute: it does not
// track this panel's state, so keying off it left the keyboard failing to pop
// up when a web field took focus.
//
// The root Item collapses to zero height when the keyboard is inactive, so with
// QQuickWidget's SizeViewToRootObject the widget takes no layout space until
// the keyboard is actually wanted.

import QtQuick
import QtQuick.VirtualKeyboard

Item {
    id: root

    // Mirrored to C++ so the widget can show/hide itself in the layout.
    property bool keyboardActive: inputPanel.active

    // Width is set from C++ to the window width; height follows the keyboard.
    // Height is unconditional: collapsing it to zero when inactive left the
    // QQuickWidget's size hint stuck at zero, so the keyboard never grew back.
    // The widget is hidden from C++ instead.
    width: 800
    height: inputPanel.height

    InputPanel {
        id: inputPanel
        width: root.width
        anchors.bottom: parent.bottom
        visible: active
    }
}
