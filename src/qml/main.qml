import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    anchors.fill: parent

    // Expose popup state to C++ overlay event filter
    property bool popupOpen: wifiPopup.visible || rebootPopup.visible || infoPopup.visible

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
        anchors.bottom: parent.bottom
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

    // Virtual keyboard — loaded dynamically to avoid crash when module is missing
    Loader {
        id: vkbLoader
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        source: "VirtualKeyboardPanel.qml"
        onStatusChanged: {
            if (status === Loader.Error)
                console.warn("Virtual keyboard not available")
        }
        onLoaded: {
            // Move bottom bar above keyboard when active
            bottomBar.anchors.bottom = undefined
            bottomBar.anchors.bottomMargin = 0
        }
    }

    states: State {
        name: "vkbActive"
        when: vkbLoader.item && vkbLoader.item.active
        AnchorChanges {
            target: bottomBar
            anchors.bottom: vkbLoader.top
        }
    }
}
