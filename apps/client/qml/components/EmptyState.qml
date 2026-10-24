import QtQuick
import QtQuick.Layouts
import ".."

Item {
    id: root
    property string title: "No conversation selected"
    property string subtitle: "Select a contact from the list or add a new peer to start messaging securely."
    property string iconText: "💬"

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 12
        width: Math.min(parent.width * 0.8, 360)

        Text {
            text: root.iconText
            font.pixelSize: 48
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: root.title
            color: Theme.textPrimary
            font.pixelSize: 18
            font.weight: Font.Bold
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: root.subtitle
            color: Theme.textSecondary
            font.pixelSize: 13
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
        }
    }
}
