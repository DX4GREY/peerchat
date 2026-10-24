import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import ".."

Rectangle {
    id: root
    color: Theme.bgBase

    ScrollView {
        anchors.fill: parent
        anchors.margins: 20
        contentWidth: Math.min(width, 500)

        ColumnLayout {
            width: Math.min(parent.width, 500)
            spacing: 20

            Text {
                text: "Settings & Identity"
                color: Theme.textPrimary
                font.pixelSize: 20
                font.weight: Font.Bold
            }

            // Identity Section
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: identityCol.implicitHeight + 24
                radius: Theme.radiusMedium
                color: Theme.bgSurface
                border.color: Theme.bgBorder

                ColumnLayout {
                    id: identityCol
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10

                    Text {
                        text: "MY IDENTITY"
                        color: Theme.textMuted
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                    }

                    TextField {
                        id: nameInput
                        Layout.fillWidth: true
                        text: chatController.myDisplayName
                        placeholderText: "Display Name"
                        color: Theme.textPrimary
                    }

                    Button {
                        text: "Update Display Name"
                        onClicked: chatController.myDisplayName = nameInput.text
                    }

                    Text {
                        text: "Cryptographic Peer ID:"
                        color: Theme.textSecondary
                        font.pixelSize: 12
                    }

                    TextField {
                        Layout.fillWidth: true
                        readOnly: true
                        text: chatController.myPeerId
                        font.family: "Monospace"
                        font.pixelSize: 11
                        color: Theme.textMuted
                    }
                }
            }

            // Relay Section
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: relayCol.implicitHeight + 24
                radius: Theme.radiusMedium
                color: Theme.bgSurface
                border.color: Theme.bgBorder

                ColumnLayout {
                    id: relayCol
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10

                    Text {
                        text: "RELAY FALLBACK CONFIGURATION"
                        color: Theme.textMuted
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                    }

                    TextField {
                        id: relayHost
                        Layout.fillWidth: true
                        placeholderText: "Relay Host (e.g. 127.0.0.1 or relay.local)"
                        color: Theme.textPrimary
                    }

                    TextField {
                        id: relayPort
                        Layout.fillWidth: true
                        placeholderText: "Relay Port (e.g. 42424)"
                        color: Theme.textPrimary
                    }

                    TextField {
                        id: relayToken
                        Layout.fillWidth: true
                        placeholderText: "Relay Auth Token (optional)"
                        color: Theme.textPrimary
                    }

                    Button {
                        text: "Connect to Relay"
                        onClicked: {
                            var p = parseInt(relayPort.text) || 42424
                            chatController.configureRelay(relayHost.text, p, relayToken.text)
                        }
                    }
                }
            }

            // Appearance & About
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: appCol.implicitHeight + 24
                radius: Theme.radiusMedium
                color: Theme.bgSurface
                border.color: Theme.bgBorder

                ColumnLayout {
                    id: appCol
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12

                    Text {
                        text: "APPEARANCE & SYSTEM"
                        color: Theme.textMuted
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                    }

                    Button {
                        text: chatController.isDarkTheme ? "Switch to Light Theme" : "Switch to Dark Theme"
                        onClicked: chatController.toggleTheme()
                    }

                    Text {
                        text: "PeerChat v0.1.0\nSecure cross-platform P2P communication built with C++20 and Qt 6."
                        color: Theme.textMuted
                        font.pixelSize: 12
                    }
                }
            }
        }
    }
}
