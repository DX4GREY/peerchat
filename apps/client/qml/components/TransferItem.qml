import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import ".."

Rectangle {
    id: root
    property string transferId: ""
    property string fileName: ""
    property double progress: 0.0
    property string status: ""
    property string sizeStr: ""
    property bool isOutgoing: true

    implicitWidth: parent ? parent.width : 320
    implicitHeight: 72
    radius: Theme.radiusMedium
    color: Theme.bgSurface
    border.color: Theme.bgBorder
    border.width: 1

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        Rectangle {
            width: 40
            height: 40
            radius: Theme.radiusSmall
            color: root.isOutgoing ? Qt.rgba(0.23, 0.51, 0.96, 0.15) : Qt.rgba(0.06, 0.72, 0.51, 0.15)

            Text {
                anchors.centerIn: parent
                text: root.isOutgoing ? "↑" : "↓"
                color: root.isOutgoing ? Theme.primary : Theme.accentSuccess
                font.pixelSize: 18
                font.weight: Font.Bold
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: root.fileName
                    color: Theme.textPrimary
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }
                Text {
                    text: root.sizeStr
                    color: Theme.textMuted
                    font.pixelSize: 11
                }
            }

            ProgressBar {
                Layout.fillWidth: true
                from: 0.0
                to: 1.0
                value: root.progress
            }

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: root.status.toUpperCase()
                    color: root.status === "completed" ? Theme.accentSuccess : (root.status === "failed" ? Theme.accentDanger : Theme.accentWarning)
                    font.pixelSize: 10
                    font.weight: Font.DemiBold
                }
            }
        }

        Button {
            visible: root.status === "offered" && !root.isOutgoing
            text: "Accept"
            onClicked: chatController.acceptTransfer(root.transferId)
        }

        Button {
            visible: root.status !== "completed" && root.status !== "failed" && root.status !== "cancelled"
            text: "✕"
            onClicked: chatController.cancelTransfer(root.transferId)
        }
    }
}
