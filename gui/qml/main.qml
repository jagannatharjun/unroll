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

    Component.onCompleted: FileBrowser.window = root
    Component.onDestruction: FileBrowser.window = null

    ViewController {
        id: controller

        onShowPreview: (data) => previewView.previewdata = data

        Component.onCompleted:
            controller.openUrl("file:///C:/Users/prince/Pictures/")
    }

    HistoryController {
        id: history

        view: controller

        onResetFocus: function (row, column) {
            // reset current preview
            previewRow = -1
            previewView.active = false

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

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            focus: true

            z : 1

            handle: Rectangle {
                implicitWidth: 4
                implicitHeight: 4

                color: SplitHandle.pressed ? palette.highlight
                       : (SplitHandle.hovered ? palette.button : palette.base)
            }

            TableViewExt {
                id: tableView

                SplitView.fillWidth: true
                SplitView.fillHeight: true
                focus: true

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

                PreviewView {
                    id: previewView

                    anchors.fill: parent
                }
            }
        }

        Keys.onPressed: function (event) {
            if (event.key === Qt.Key_Z || event.key === Qt.Key_N)
                tableView.forceActiveFocus(Qt.TabFocusReason)
            else if (event.key === Qt.Key_X || event.key === Qt.Key_M)
                previewView.forceActiveFocus(Qt.TabFocusReason)
            else if (event.key === Qt.Key_Refresh || event.key === Qt.Key_F5)
                controller.refresh()
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
