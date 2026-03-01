import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Popup {
    id: popup
    anchors.centerIn: parent
    width: 280; height: 140
    modal: true; focus: true

    background: Rectangle {
        color: "#ffcccc"
        border.color: "black"; border.width: 2
        radius: 4
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 16

        Text {
            text: "OK to restart?\nYour device will restart safely."
            font.pixelSize: 14
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 8

            Button {
                text: "Cancel"
                onClicked: popup.close()
            }
            Button {
                text: "Reboot"
                onClicked: {
                    systemController.reboot();
                    popup.close();
                }
            }
        }
    }
}
