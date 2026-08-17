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

import QtQuick
import QtQuick.VirtualKeyboard
import QtQuick.VirtualKeyboard.Settings

Item {
    id: root

    // Mirrored to C++ so the widget can show/hide itself in the layout.
    property bool keyboardActive: inputPanel.active

    // Keyboard height as a fraction of the panel width. The style's own
    // 2560x800 design ratio yields rows too short to hit on a touch screen at
    // this resolution, so the design height is overridden.
    //
    // Qt gives every layout the same panel height, so this is the single knob
    // for how tall the keyboard is: raise it for taller keys, lower it for a
    // shorter keypad. 0.465 suits the three-row letter layout (the digit row
    // was removed); it was 0.62 when that layout had four rows.
    property real heightRatio: 0.465

    // Width is set from C++ to the window width; height follows the keyboard,
    // collapsing to zero when it is not wanted.
    //
    // The widget is never hidden — hiding a QQuickWidget stops its scene
    // updating, and the keyboard then came back with no height at all even
    // though the input method had asked for it.
    width: 800
    height: inputPanel.active ? inputPanel.height : 0

    Component.onCompleted: {
        // The T420 terminal's keyboard: red lettering on dark keys. Shipped in
        // the package and found via the import path added in C++. Falls back to
        // the default style with a warning if the style is missing.
        VirtualKeyboardSettings.styleName = "robot"
        VirtualKeyboardSettings.locale = "en_GB"
    }

    InputPanel {
        id: inputPanel
        width: root.width
        anchors.bottom: parent.bottom
        visible: active

        // Stretch the key rows to a usable height. keyboardDesignHeight is the
        // reference the style scales against, so lowering it relative to the
        // design width makes each row taller for the same panel width.
        Binding {
            target: inputPanel.keyboard.style
            property: "keyboardDesignHeight"
            value: inputPanel.keyboard.style.keyboardDesignWidth * root.heightRatio
        }
    }
}
