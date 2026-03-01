import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: bar
    color: "#2b2b2b"

    RowLayout {
        anchors.fill: parent
        anchors.margins: 2
        spacing: 4

        ToolButton {
            icon.source: "qrc:/images/home.png"
            icon.width: 34; icon.height: 34
            onClicked: webPageController.loadLocal()
            Layout.preferredWidth: 40; Layout.preferredHeight: 40
        }

        ToolButton {
            icon.source: "qrc:/images/store.png"
            icon.width: 34; icon.height: 34
            onClicked: webPageController.loadRemote()
            Layout.preferredWidth: 40; Layout.preferredHeight: 40
        }

        ToolButton {
            icon.source: "qrc:/images/back.png"
            icon.width: 34; icon.height: 34
            onClicked: webPageController.goBack()
            Layout.preferredWidth: 40; Layout.preferredHeight: 40
        }

        Item { Layout.fillWidth: true }

        Image {
            id: wifiIcon
            source: {
                var level = wpaController.signalLevel;
                if (level < 0) return "qrc:/images/wifi-off.png";
                return "qrc:/images/wifi-" + level + ".png";
            }
            sourceSize.width: 34; sourceSize.height: 34
            Layout.preferredWidth: 34; Layout.preferredHeight: 34

            MouseArea {
                anchors.fill: parent
                onClicked: wifiPopup.open()
            }
        }

        Text {
            id: clock
            color: "white"
            font.pixelSize: 16
            font.family: "monospace"
            Layout.preferredWidth: 50
            horizontalAlignment: Text.AlignHCenter

            Timer {
                interval: 1000; running: true; repeat: true
                onTriggered: {
                    var d = new Date();
                    var h = ("0" + d.getHours()).slice(-2);
                    var m = ("0" + d.getMinutes()).slice(-2);
                    var sep = d.getSeconds() % 2 === 0 ? ":" : " ";
                    clock.text = h + sep + m;
                }
            }

            Component.onCompleted: {
                var d = new Date();
                var h = ("0" + d.getHours()).slice(-2);
                var m = ("0" + d.getMinutes()).slice(-2);
                text = h + ":" + m;
            }
        }

        ToolButton {
            icon.source: "qrc:/images/info.png"
            icon.width: 34; icon.height: 34
            onClicked: infoPopup.open()
            Layout.preferredWidth: 40; Layout.preferredHeight: 40
        }
    }
}
