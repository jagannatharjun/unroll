import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import filebrowser 0.1
import "preview" as Preview

ApplicationWindow {
    id: root

    width: 1200
    height: 800
    visible: true
    title: qsTr("File Browser")

    // row of which we are showing the preview of
    // useful in changing the preview when model changes or resetFocus
    property int previewRow: -1

    property var previewdata: controller.invalidPreviewData()

    Component.onCompleted: FileBrowser.window = root
    Component.onDestruction: FileBrowser.window = null

    ViewController {
        id: controller

        onShowPreview: (data) => root.previewdata = data

        Component.onCompleted:
            controller.openUrl("file:///C:/Users/prince/Pictures/")
    }

    HistoryController {
        id: history

        view: controller

        onResetFocus: function (row, column) {
            // reset current preview
            previewRow = -1
            root.previewdata = controller.invalidPreviewData()

            var index = controller.model.index(row, column)
            selectionModel.setCurrentIndex(index, ItemSelectionModel.ClearAndSelect)

        }
    }

    ItemSelectionModel {
        id: selectionModel
        onCurrentChanged: {
            if (root.previewRow === -1 || root.previewRow !== currentIndex.row) {
                controller.setPreview(currentIndex.row)
                root.previewRow = currentIndex.row
            }

            history.updateCurrentIndex(currentIndex.row, currentIndex.column)
        }
    }

    FolderDialog {
        id: folderDialog

        onAccepted: {
            controller.openUrl(folderDialog.selectedFolder)
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

    menuBar: AppMenuBar {
        enableBack: history.canMoveBack

        onOpenFolder: folderDialog.open()
        onBack: history.pop()
        onAppExit: root.close()
    }

    ColumnLayout {

        anchors.fill: parent
        spacing: 0

        Pane {
            Layout.fillWidth: true

            // this is to cover outoff view items from TableView
            z: 2

            background: Rectangle {
                color: palette.window
            }

            contentItem: RowLayout {

                PathEdit {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    path: controller.path

                    onRequestPath: function (path) {
                        controller.openPath(path)
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 200
                    Layout.fillHeight: true

                    border.width: 1
                    border.color: searchEdit.visualFocus || searchEdit.activeFocus ? palette.highlight : palette.light
                    color: palette.dark

                    Behavior on border.color {
                        ColorAnimation {
                            duration: 80
                        }
                    }

                    TextInput {
                        id: searchEdit

                        anchors {
                            left: parent.left
                            right: parent.right
                            verticalCenter: parent.verticalCenter
                            margins: 3
                        }

                        color: palette.text

                        onTextChanged: controller.model.setFilterWildcard(text)
                    }
                }
            }
        }

        MainView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            focus: true

            z : 1

            model: controller.model // FIXME: on qt 5.15.2, app crashes whenever content of model changes
            modelSortOrder: controller.model.sortOrder
            modelSortColumn: controller.model.sortColumn

            previewdata: root.previewdata

            selectionModel: selectionModel

            onActionAtIndex: (row) => controller.openRow(row)

            Keys.onPressed: function (event) {
                if (event.key === Qt.Key_Refresh || event.key === Qt.Key_F5) {
                    controller.refresh()
                }
            }
        }
    }

    // place it over everything so no one can steal it
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.BackButton | Qt.ForwardButton
        onClicked: function (mouse) {
            if (mouse.button & Qt.BackButton)
                history.pop()
            else if (mouse.button & Qt.ForwardButton)
                history.forward()
        }
    }
}
