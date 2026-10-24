import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import ".."
import "../components"

Rectangle {
    id: root
    color: Theme.bgBase

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "File Transfers Queue"
                color: Theme.textPrimary
                font.pixelSize: 18
                font.weight: Font.Bold
            }
        }

        ListView {
            id: transferList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: chatController.transfers
            spacing: 8

            delegate: TransferItem {
                width: transferList.width
                transferId: model.transferId
                fileName: model.fileName
                progress: model.progress
                status: model.status
                isOutgoing: model.isOutgoing
                sizeStr: model.sizeStr
            }
        }
    }
}
