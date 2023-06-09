import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Dialogs
import Qt.labs.platform

import filebrowser 0.1
import "preview" as Preview

Window {
    id: root

    width: 780
    height: 480
    visible: true
    title: qsTr("File Browser")

    // row of which we are showing the preview of
    // useful in changing the preview when model changes or resetFocus
    property int previewRow: -1

    Component.onCompleted: FileBrowser.window = root
    Component.onDestruction: FileBrowser.window = null

    ViewController {
        id: controller

        onShowPreview: function (data) {
            previewloader.active = data.fileType() !== PreviewData.Unknown
            if (data.fileType() === PreviewData.ImageFile)
                previewloader.setSource("qrc:/preview/ImagePreview.qml", {"previewdata": data})
            else if (data.fileType() === PreviewData.VideoFile
                     || data.fileType() === PreviewData.AudioFile)
                previewloader.setSource("qrc:/preview/Player.qml", {"previewdata": data})
        }

        Component.onCompleted:
            controller.openUrl("file:///C:/Users/prince/Pictures/")
    }

    HistoryController {
        id: history

        view: controller

        onResetFocus: function (row, column) {
            // reset current preview
            previewRow = -1
            previewloader.active = false

            var index = tableView.model.index(row, column)
            tableView.selectionModel.setCurrentIndex(index, ItemSelectionModel.ClearAndSelect)

            // FIXME: TableView.positionView* doesn't work inside this function, use a timer
            viewplacer.row = row
            viewplacer.col = column
            viewplacer.start()
        }
    }

    Timer {
        id: viewplacer

        property var row
        property var col

        interval: 100
        onTriggered: {
            // positionViewAtRow doesn't take scrollbar into consideration so add offset to make sure row is visible
            let offset = row === 0 ? 0 : 30
            tableView.view.positionViewAtCell(Qt.point(col, row), TableView.Visible, Qt.point(offset, offset))
        }
    }

    FolderDialog {
        id: folderDialog

        onAccepted: {
            controller.openUrl(folderDialog.folder)
        }
    }

    Shortcut {
        sequence: StandardKey.Open
        onActivated: folderDialog.open()
    }

    Shortcut {
        sequences: [StandardKey.Back, StandardKey.Backspace]
        onActivated: history.pop()
    }

    SplitView {
        anchors.fill: parent

        focus: true

        TableViewExt {
            id: tableView

            SplitView.fillWidth: true
            SplitView.fillHeight: true

            model: controller.model // FIXME: on qt 5.15.2, app crashes whenever content of model changes

            modelSortOrder: controller.model.sortOrder
            modelSortColumn: controller.model.sortColumn

            selectionModel: ItemSelectionModel {
                onCurrentChanged: {
                    if (root.previewRow === -1 || root.previewRow !== currentIndex.row) {
                        controller.setPreview(currentIndex.row)
                        root.previewRow = currentIndex.row
                    }

                    history.updateCurrentIndex(currentIndex.row, currentIndex.column)
                }
            }

            onActionAtIndex: function (row) {
                controller.openRow(row)
            }
        }


        Pane {
            // tableview content can overflow, so wrap preview inside Pane,
            // so the extra content is not visible

            SplitView.preferredWidth: root.width / 2
            SplitView.fillWidth: true
            SplitView.fillHeight: true

            Loader {
                id: previewloader

                anchors.fill: parent
                asynchronous: true
            }

        }
    }

    // place it over everything so no one can steal it
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.BackButton
        onClicked: {
            history.pop()
        }
    }
}
