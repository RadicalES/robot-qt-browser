import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.VirtualKeyboard 2.4
import QtQuick.VirtualKeyboard.Settings 2.0

InputPanel {
    id: inputPanel

    Component.onCompleted: {
        VirtualKeyboardSettings.layoutPath = "layouts"
        VirtualKeyboardSettings.locale = "en_GB"
        var inputModes = InputContext.inputEngine.inputModes        
    }

    signal activated(bool a)
    signal heightChanged(int h)

    onActiveChanged: {
        activated(active)
        this.y = 70
    }

    onHeightChanged: heightChanged(height)

    onWidthChanged: {
    }

    function setHeight(h) {
        this.height = h
    }

    function isInputModeSupported(inputMode) {
        return InputContext.inputEngine.inputModes.indexOf(inputMode) !== -1
    }

    function setInputMode(inputMode) {	
        var m = InputEngine.InputMode.Numeric
        if (!isInputModeSupported(m)) {
            return false
        }
        if (InputContext.inputEngine.inputMode !== m) {	    
            InputContext.inputEngine.inputMode = m
        }

        return true
    }

    property var kb: inputPanel.keyboard
    property var ie: InputContext.inputEngine

    Connections {
        target: InputContext.inputEngine

        onInputModeChanged: {		
        }

        onInputMethodChanged: {
        }
    }
}

