import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.VirtualKeyboard 2.15

Item {
    id: root
    anchors.fill: parent

    // Transparent area above the bar — mouse events pass through to QWebView underneath
    Item {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: bottomBar.top
    }

    BottomBar {
        id: bottomBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: inputPanel.active ? inputPanel.top : parent.bottom
        height: 44
    }

    WifiPopup {
        id: wifiPopup
    }

    RebootPopup {
        id: rebootPopup
    }

    InfoPopup {
        id: infoPopup
    }

    InputPanel {
        id: inputPanel
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: active
    }
}
