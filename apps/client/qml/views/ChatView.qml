import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import ".."
import "../components"

Rectangle {
    id: root
    color: Theme.bgBase

    signal backRequested()

    FileDialog {
        id: fileDialog
        title: "Select Attachment to Send"
        onAccepted: {
            chatController.sendFile(fileDialog.selectedFile.toString())
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Conversation Header
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 64
            color: Theme.bgSurface
            border.color: Theme.bgBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12

                Button {
                    visible: root.width < 600
                    text: "←"
                    onClicked: root.backRequested()
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: chatController.activeContactName
                        color: Theme.textPrimary
                        font.pixelSize: 15
                        font.weight: Font.Bold
                        elide: Text.ElideRight
                    }
                    Text {
                        text: chatController.activeContactPeerId
                        color: Theme.textMuted
                        font.pixelSize: 11
                        font.family: "Monospace"
                        elide: Text.ElideMiddle
                    }
                }

                RouteBadge {
                    route: chatController.activeRouteType
                }
            }
        }

        // Messages List
        ListView {
            id: messageList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: chatController.messages
            spacing: 4

            delegate: MessageBubble {
                width: messageList.width
                textContent: model.content
                timeStr: model.timeFormatted
                statusStr: model.statusStr
                isOutgoing: model.isOutgoing
                mediaPath: model.mediaPath
            }

            onCountChanged: {
                Qt.callLater(() => {
                    messageList.positionViewAtEnd()
                })
            }
        }

        // Message Input Footer
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 64
            color: Theme.bgSurface
            border.color: Theme.bgBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                Button {
                    text: "📎"
                    ToolTip.visible: hovered
                    ToolTip.text: "Send File / Image"
                    onClicked: fileDialog.open()
                }

                TextField {
                    id: inputField
                    Layout.fillWidth: true
                    placeholderText: "Type a secure message..."
                    color: Theme.textPrimary
                    placeholderTextColor: Theme.textMuted
                    background: Rectangle {
                        color: Theme.bgElevated
                        radius: Theme.radiusMedium
                        border.color: inputField.activeFocus ? Theme.primary : Theme.bgBorder
                    }

                    onAccepted: {
                        if (text.trim().length > 0) {
                            chatController.sendMessage(text)
                            text = ""
                        }
                    }
                }

                Button {
                    text: "Send"
                    highlighted: true
                    enabled: inputField.text.trim().length > 0
                    onClicked: {
                        chatController.sendMessage(inputField.text)
                        inputField.text = ""
                    }
                }
            }
        }
    }
}
