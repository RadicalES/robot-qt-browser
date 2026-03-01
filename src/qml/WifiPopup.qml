import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Popup {
    id: popup
    anchors.centerIn: parent
    width: Math.min(parent.width * 0.85, 420)
    height: Math.min(parent.height * 0.8, 520)
    modal: true; focus: true
    closePolicy: Popup.CloseOnEscape

    property string selectedSsid: ""
    property bool showPassword: false
    property bool showForget: false

    onOpened: {
        selectedSsid = "";
        showPassword = false;
        showForget = false;
        passwordField.text = "";
        networkController.scan();
    }

    background: Rectangle {
        color: "#f0f0f0"
        border.color: "#888"; border.width: 1
        radius: 6
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        // Header
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "WiFi"
                font.pixelSize: 18; font.bold: true
                Layout.fillWidth: true
            }
            ToolButton {
                text: "\u2715"
                font.pixelSize: 16
                onClicked: popup.close()
            }
        }

        // Connection status
        Rectangle {
            Layout.fillWidth: true
            height: statusCol.height + 12
            color: networkController.connected ? "#dff0d8" : "#f2dede"
            radius: 4

            ColumnLayout {
                id: statusCol
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: parent.top; anchors.margins: 6
                spacing: 2

                Text {
                    text: networkController.connected
                          ? "Connected: " + networkController.ssid
                          : "Not connected"
                    font.pixelSize: 13; font.bold: true
                }
                Text {
                    text: networkController.connected
                          ? "IP: " + networkController.ipAddress
                          : ""
                    font.pixelSize: 12
                    visible: networkController.connected
                }
            }
        }

        // Scan header
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "Available Networks"
                font.pixelSize: 14; font.bold: true
                Layout.fillWidth: true
            }
            Button {
                text: networkController.scanning ? "Scanning..." : "Scan"
                enabled: !networkController.scanning
                font.pixelSize: 12
                onClicked: networkController.scan()
            }
        }

        // Network list
        ListView {
            id: networkList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: networkController.networks

            delegate: Rectangle {
                width: networkList.width
                height: 44
                color: modelData.ssid === popup.selectedSsid ? "#d0e0f0"
                     : modelData.connected ? "#e8f5e9"
                     : index % 2 === 0 ? "#ffffff" : "#f8f8f8"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8; anchors.rightMargin: 8
                    spacing: 8

                    // Connected indicator
                    Text {
                        text: modelData.connected ? "\u2713" : ""
                        font.pixelSize: 16; font.bold: true
                        color: "#4caf50"
                        Layout.preferredWidth: 18
                    }

                    // SSID
                    Text {
                        text: modelData.ssid
                        font.pixelSize: 14
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    // Signal bars
                    Row {
                        spacing: 1
                        Layout.preferredWidth: 24
                        Repeater {
                            model: 4
                            Rectangle {
                                width: 4
                                height: 6 + index * 4
                                anchors.bottom: parent.bottom
                                color: {
                                    var level = Math.floor(modelData.signal / 25);
                                    return index < level ? "#4caf50" : "#ccc";
                                }
                            }
                        }
                    }

                    // Security
                    Text {
                        text: modelData.security
                        font.pixelSize: 11
                        color: "#666"
                        Layout.preferredWidth: 50
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (modelData.connected) {
                            popup.selectedSsid = modelData.ssid;
                            popup.showForget = true;
                            popup.showPassword = false;
                        } else if (modelData.security === "Open") {
                            networkController.connectToNetwork(modelData.ssid, "");
                        } else if (modelData.saved) {
                            networkController.connectToNetwork(modelData.ssid, "");
                        } else {
                            popup.selectedSsid = modelData.ssid;
                            popup.showPassword = true;
                            popup.showForget = false;
                            passwordField.text = "";
                            passwordField.forceActiveFocus();
                        }
                    }
                }
            }

            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        }

        // Password input
        RowLayout {
            Layout.fillWidth: true
            visible: popup.showPassword
            spacing: 6

            Text {
                text: popup.selectedSsid + ":"
                font.pixelSize: 12
                Layout.preferredWidth: 80
                elide: Text.ElideRight
            }
            TextField {
                id: passwordField
                Layout.fillWidth: true
                placeholderText: "Password"
                echoMode: TextInput.Password
                font.pixelSize: 14
                onAccepted: connectBtn.clicked()
            }
            Button {
                id: connectBtn
                text: "Connect"
                font.pixelSize: 12
                enabled: passwordField.text.length >= 8
                onClicked: {
                    networkController.connectToNetwork(popup.selectedSsid,
                                                       passwordField.text);
                    popup.showPassword = false;
                    passwordField.text = "";
                }
            }
            Button {
                text: "Cancel"
                font.pixelSize: 12
                onClicked: {
                    popup.showPassword = false;
                    popup.selectedSsid = "";
                    passwordField.text = "";
                }
            }
        }

        // Forget confirmation
        RowLayout {
            Layout.fillWidth: true
            visible: popup.showForget
            spacing: 6

            Text {
                text: "Forget \"" + popup.selectedSsid + "\"?"
                font.pixelSize: 12
                Layout.fillWidth: true
            }
            Button {
                text: "Forget"
                font.pixelSize: 12
                onClicked: {
                    networkController.forgetNetwork(popup.selectedSsid);
                    popup.showForget = false;
                    popup.selectedSsid = "";
                }
            }
            Button {
                text: "Disconnect"
                font.pixelSize: 12
                onClicked: {
                    networkController.disconnectWifi();
                    popup.showForget = false;
                    popup.selectedSsid = "";
                }
            }
            Button {
                text: "Cancel"
                font.pixelSize: 12
                onClicked: {
                    popup.showForget = false;
                    popup.selectedSsid = "";
                }
            }
        }

        // Error display
        Text {
            text: networkController.error
            color: "red"
            font.pixelSize: 11
            visible: networkController.error !== ""
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }

        // Footer
        RowLayout {
            Layout.fillWidth: true
            Button {
                text: "Restart WiFi"
                font.pixelSize: 12
                onClicked: {
                    networkController.restartWifi();
                    popup.close();
                }
            }
            Item { Layout.fillWidth: true }
            Button {
                text: "Close"
                font.pixelSize: 12
                onClicked: popup.close()
            }
        }
    }
}
