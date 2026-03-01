import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Popup {
    id: popup
    anchors.centerIn: parent
    width: 300; height: 200
    modal: true; focus: true

    background: Rectangle {
        color: "#e0e0e0"
        border.color: "black"; border.width: 2
        radius: 4
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        Text {
            text: systemController.systemInfo()
            font.pixelSize: 12
            font.family: "monospace"
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 8

            Button {
                text: "Reset Defaults"
                onClicked: systemController.resetDefaults()
            }
            Button {
                text: "Reboot"
                onClicked: rebootPopup.open()
            }
            Button {
                text: "OK"
                onClicked: popup.close()
            }
        }
    }
}
