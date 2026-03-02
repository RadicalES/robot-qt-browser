import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Popup {
    id: popup
    anchors.centerIn: parent
    width: Math.min(parent.width * 0.85, 360)
    modal: true; focus: true
    closePolicy: Popup.CloseOnEscape

    background: Rectangle {
        color: "#f0f0f0"
        border.color: "#888"; border.width: 1
        radius: 6
    }

    contentItem: ColumnLayout {
        spacing: 12

        // Header
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "System Info"
                font.pixelSize: 18; font.bold: true
                Layout.fillWidth: true
            }
            ToolButton {
                text: "\u2715"
                font.pixelSize: 16
                onClicked: popup.close()
            }
        }

        // System info
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: infoText.implicitHeight + 16
            color: "#ffffff"
            border.color: "#ddd"; border.width: 1
            radius: 4

            Text {
                id: infoText
                anchors.fill: parent
                anchors.margins: 8
                text: systemController.systemInfo()
                font.pixelSize: 13
                font.family: "monospace"
                wrapMode: Text.WordWrap
                lineHeight: 1.3
            }
        }

        // Actions
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: "Reset Defaults"
                font.pixelSize: 12
                onClicked: systemController.resetDefaults()
                background: Rectangle {
                    implicitWidth: 100; implicitHeight: 32
                    color: parent.down ? "#c62828" : parent.hovered ? "#e53935" : "#ef5350"
                    radius: 4
                }
                contentItem: Text {
                    text: parent.text; font: parent.font
                    color: "white"; horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Reboot"
                font.pixelSize: 12
                onClicked: rebootPopup.open()
                background: Rectangle {
                    implicitWidth: 80; implicitHeight: 32
                    color: parent.down ? "#e65100" : parent.hovered ? "#fb8c00" : "#ffa726"
                    radius: 4
                }
                contentItem: Text {
                    text: parent.text; font: parent.font
                    color: "white"; horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
            Button {
                text: "Close"
                font.pixelSize: 12
                onClicked: popup.close()
                background: Rectangle {
                    implicitWidth: 80; implicitHeight: 32
                    color: parent.down ? "#1565c0" : parent.hovered ? "#1e88e5" : "#42a5f5"
                    radius: 4
                }
                contentItem: Text {
                    text: parent.text; font: parent.font
                    color: "white"; horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
}
