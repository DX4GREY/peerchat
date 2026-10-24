import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import ".."
import "../components"

Rectangle {
    id: root
    color: Theme.bgSurface
    border.color: Theme.bgBorder
    border.width: 1

    signal contactSelected(string peerId)
    signal openAddContactDialog()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header / Profile summary
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 64
            color: Theme.bgElevated
            border.color: Theme.bgBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                Rectangle {
                    width: 36
                    height: 36
                    radius: 18
                    color: Theme.primary

                    Text {
                        anchors.centerIn: parent
                        text: (chatController.myDisplayName.length > 0 ? chatController.myDisplayName.charAt(0) : "P").toUpperCase()
                        color: "#FFFFFF"
                        font.pixelSize: 16
                        font.weight: Font.Bold
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: chatController.myDisplayName
                        color: Theme.textPrimary
                        font.pixelSize: 14
                        font.weight: Font.Bold
                        elide: Text.ElideRight
                    }
                    Text {
                        text: "Port: " + chatController.listenPort
                        color: Theme.textMuted
                        font.pixelSize: 11
                    }
                }

                Button {
                    text: "+"
                    ToolTip.visible: hovered
                    ToolTip.text: "Add Contact"
                    onClicked: root.openAddContactDialog()
                }
            }
        }

        // Contact List
        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: chatController.contacts

            delegate: Rectangle {
                width: listView.width
                height: 60
                color: model.peerId === chatController.activeContactPeerId ? Theme.bgElevated : "transparent"

                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 1
                    color: Theme.bgBorder
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10

                    Rectangle {
                        width: 34
                        height: 34
                        radius: 17
                        color: Qt.rgba(0.23, 0.51, 0.96, 0.2)

                        Text {
                            anchors.centerIn: parent
                            text: (model.displayName.length > 0 ? model.displayName.charAt(0) : "?").toUpperCase()
                            color: Theme.primary
                            font.pixelSize: 14
                            font.weight: Font.Bold
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text {
                            text: model.displayName
                            color: Theme.textPrimary
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }
                        Text {
                            text: model.peerId.substring(0, 16) + "..."
                            color: Theme.textMuted
                            font.pixelSize: 11
                            font.family: "Monospace"
                        }
                    }

                    RouteBadge {
                        route: model.routeType
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        chatController.selectContact(model.peerId)
                        root.contactSelected(model.peerId)
                    }
                }
            }
        }
    }
}
