import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import "views"
import "components"

ApplicationWindow {
    id: window
    width: 960
    height: 640
    minimumWidth: 380
    minimumHeight: 500
    visible: true
    title: "PeerChat - " + chatController.myDisplayName

    color: Theme.bgBase

    property int activeNavTab: 0 // 0: Chat, 1: Transfers, 2: Settings
    property bool isMobile: width < 680

    // Add Contact Dialog
    Dialog {
        id: addContactDialog
        title: "Add Remote Peer"
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        ColumnLayout {
            spacing: 10
            width: 320

            Text {
                text: "Peer ID (peer:ed25519:...):"
                color: Theme.textPrimary
                font.pixelSize: 12
            }
            TextField {
                id: newPeerId
                Layout.fillWidth: true
                placeholderText: "peer:ed25519:..."
                color: Theme.textPrimary
            }

            Text {
                text: "Display Name (optional):"
                color: Theme.textPrimary
                font.pixelSize: 12
            }
            TextField {
                id: newDisplayName
                Layout.fillWidth: true
                placeholderText: "Alice"
                color: Theme.textPrimary
            }

            Text {
                text: "Direct IP:Port (optional):"
                color: Theme.textPrimary
                font.pixelSize: 12
            }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: directIp
                    Layout.fillWidth: true
                    placeholderText: "192.168.1.10"
                    color: Theme.textPrimary
                }
                TextField {
                    id: directPort
                    width: 70
                    placeholderText: "42420"
                    color: Theme.textPrimary
                }
            }
        }

        onAccepted: {
            if (newPeerId.text.trim().length > 0) {
                chatController.addContact(newPeerId.text.trim(), newDisplayName.text.trim())
                if (directIp.text.trim().length > 0 && directPort.text.trim().length > 0) {
                    var p = parseInt(directPort.text.trim()) || 42420
                    chatController.connectDirect(newPeerId.text.trim(), directIp.text.trim(), p)
                }
                newPeerId.text = ""
                newDisplayName.text = ""
                directIp.text = ""
                directPort.text = ""
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Vertical Nav Rail
        Rectangle {
            Layout.preferredWidth: 64
            Layout.fillHeight: true
            color: Theme.bgSurface
            border.color: Theme.bgBorder
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.topMargin: 16
                anchors.bottomMargin: 16
                spacing: 16

                Rectangle {
                    width: 40
                    height: 40
                    radius: 20
                    color: Theme.primary
                    Layout.alignment: Qt.AlignHCenter

                    Text {
                        anchors.centerIn: parent
                        text: "PC"
                        color: "#FFFFFF"
                        font.pixelSize: 15
                        font.weight: Font.Bold
                    }
                }

                Item { Layout.fillHeight: true }

                // Chat Tab
                Button {
                    Layout.alignment: Qt.AlignHCenter
                    text: "💬"
                    highlighted: window.activeNavTab === 0
                    onClicked: window.activeNavTab = 0
                }

                // Transfers Tab
                Button {
                    Layout.alignment: Qt.AlignHCenter
                    text: "📁"
                    highlighted: window.activeNavTab === 1
                    onClicked: window.activeNavTab = 1
                }

                // Settings Tab
                Button {
                    Layout.alignment: Qt.AlignHCenter
                    text: "⚙"
                    highlighted: window.activeNavTab === 2
                    onClicked: window.activeNavTab = 2
                }

                Item { Layout.fillHeight: true }
            }
        }

        // Main Workspace
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: window.activeNavTab

            // Tab 0: Chat (Responsive Split or Mobile)
            Item {
                RowLayout {
                    anchors.fill: parent
                    spacing: 0

                    ContactsView {
                        Layout.preferredWidth: window.isMobile ? (chatController.activeContactPeerId === "" ? parent.width : 0) : 300
                        Layout.fillHeight: true
                        visible: !window.isMobile || chatController.activeContactPeerId === ""

                        onContactSelected: function(peerId) {
                            // Selected
                        }
                        onOpenAddContactDialog: {
                            addContactDialog.open()
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: !window.isMobile || chatController.activeContactPeerId !== ""

                        ChatView {
                            anchors.fill: parent
                            visible: chatController.activeContactPeerId !== ""
                            onBackRequested: {
                                chatController.selectContact("")
                            }
                        }

                        EmptyState {
                            anchors.fill: parent
                            visible: chatController.activeContactPeerId === ""
                        }
                    }
                }
            }

            // Tab 1: Transfers
            TransfersView {}

            // Tab 2: Settings
            SettingsView {}
        }
    }
}
