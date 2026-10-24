import QtQuick
import QtQuick.Layouts
import ".."

Rectangle {
    id: root
    property string route: "Offline" // "Direct", "Relay", "Connecting", "Offline"

    implicitWidth: badgeRow.implicitWidth + 14
    implicitHeight: 22
    radius: Theme.radiusPill

    color: {
        switch (route) {
            case "Direct": return Qt.rgba(0.06, 0.72, 0.51, 0.15)
            case "Relay": return Qt.rgba(0.96, 0.62, 0.04, 0.15)
            case "Connecting": return Qt.rgba(0.05, 0.65, 0.91, 0.15)
            default: return Qt.rgba(0.42, 0.45, 0.50, 0.15)
        }
    }

    border.color: {
        switch (route) {
            case "Direct": return Theme.accentSuccess
            case "Relay": return Theme.accentWarning
            case "Connecting": return Theme.accentInfo
            default: return Theme.accentOffline
        }
    }
    border.width: 1

    RowLayout {
        id: badgeRow
        anchors.centerIn: parent
        spacing: 5

        Rectangle {
            width: 6
            height: 6
            radius: 3
            color: root.border.color
        }

        Text {
            text: root.route
            color: root.border.color
            font.pixelSize: 11
            font.weight: Font.DemiBold
        }
    }
}
