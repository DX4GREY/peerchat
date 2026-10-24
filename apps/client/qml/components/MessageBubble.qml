import QtQuick
import QtQuick.Layouts
import ".."

Item {
    id: root
    property string textContent: ""
    property string timeStr: ""
    property string statusStr: ""
    property bool isOutgoing: false
    property string mediaPath: ""

    implicitWidth: parent ? parent.width : 300
    implicitHeight: bubbleContainer.implicitHeight + 8

    Rectangle {
        id: bubbleContainer
        anchors.right: isOutgoing ? parent.right : undefined
        anchors.left: isOutgoing ? undefined : parent.left
        anchors.margins: 12
        width: Math.min(contentColumn.implicitWidth + 24, root.width * 0.75)
        height: contentColumn.implicitHeight + 16
        radius: Theme.radiusMedium

        color: isOutgoing ? Theme.bubbleOutgoing : Theme.bubbleIncoming

        ColumnLayout {
            id: contentColumn
            anchors.fill: parent
            anchors.margins: 8
            spacing: 4

            // Optional Image Preview if media is attached
            Image {
                visible: mediaPath !== ""
                source: mediaPath !== "" ? "file://" + mediaPath : ""
                fillMode: Image.PreserveAspectFit
                Layout.maximumWidth: 260
                Layout.maximumHeight: 200
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: root.textContent
                color: isOutgoing ? Theme.bubbleTextOutgoing : Theme.bubbleTextIncoming
                font.pixelSize: 14
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.alignment: isOutgoing ? Qt.AlignRight : Qt.AlignLeft
                spacing: 4

                Text {
                    text: root.timeStr
                    color: isOutgoing ? Qt.rgba(1, 1, 1, 0.7) : Theme.textMuted
                    font.pixelSize: 10
                }

                Text {
                    visible: isOutgoing
                    text: root.statusStr === "delivered" ? "✓✓" : "✓"
                    color: isOutgoing ? Qt.rgba(1, 1, 1, 0.8) : Theme.textMuted
                    font.pixelSize: 11
                    font.weight: Font.Bold
                }
            }
        }
    }
}
