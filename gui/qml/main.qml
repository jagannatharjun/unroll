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

    property bool _previewCompleted: false

    Component.onCompleted: FileBrowser.window = root
    Component.onDestruction: FileBrowser.window = null

    function back() {
        // history.pop()
        controller.openParentPath()
    }

    function nextIndex(up) {
        const diff = up ? - 1 : 1
        const current = selectionModel.currentIndex
        const newRow = current.row + diff
        if (newRow < 0 || newRow >= controller.model.rowCount())
            return false

        const idx = controller.model.index(newRow, current.column)
        selectionModel.setCurrentIndex(idx, ItemSelectionModel.ClearAndSelect)
    }

    ViewController {
        id: controller

        fileBrowser: FileBrowser

        onShowPreview: (data) => root.previewdata = data
    }

    HistoryController {
        id: history

        view: controller

        preferences: Preferences

        onResetFocus: function (row, column) {
            // reset current preview
            previewRow = -1
            root.previewdata = controller.invalidPreviewData()
            _previewCompleted = false

            var index = controller.model.index(row, column)
            selectionModel.setCurrentIndex(index, ItemSelectionModel.ClearAndSelect)
        }
    }

    ItemSelectionModel {
        id: selectionModel

        function updateProgress(idx) {
            const previewProgress = mainView?.previewProgress()
            if (!!previewProgress) {
                controller.model.setData(idx, previewProgress, DirectorySystemModel.ProgressRole)
            }
        }

        function updateSeen(idx) {
            if (root._previewCompleted)
                controller.model.setData(idx, true, DirectorySystemModel.SeenRole)
        }

        onCurrentChanged: function (current, previous) {
            updateSeen(previous)

            if (root.previewRow === -1 || root.previewRow !== current.row) {
                updateProgress(previous)

                controller.setPreview(current.row)
                root.previewRow = current.row
            }

            history.updateCurrentIndex(current.row, current.column)

            root._previewCompleted = false
        }

        Component.onDestruction: {
            const idx = selectionModel.currentIndex
            updateProgress(idx)
            updateSeen(idx)
        }
    }

    FolderDialog {
        id: folderDialog

        onAccepted: {
            controller.openUrl(folderDialog.selectedFolder)
            Preferences.pushRecentUrl(folderDialog.selectedFolder)
        }
    }

    Shortcut {
        sequence: StandardKey.Open
        onActivated: folderDialog.open()
    }

    Shortcut {
        sequences: [StandardKey.Back, StandardKey.Backspace]
        onActivated: root.back()
    }

    menuBar: AppMenuBar {
        enableBack: history.canMoveBack

        onBrowseFolder: folderDialog.open()
        onOpenUrl: url => controller.openUrl(url)
        onBack: root.back()
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
            id: mainView

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

            onPreviewCompleted: {
                root._previewCompleted = true

                const previewType = root.previewdata.fileType()
                if ((previewType === PreviewData.AudioFile)
                        || (previewType === PreviewData.VideoFile))
                    root.nextIndex(false)
            }

            onPreviewed: {
                controller.model.setData(selectionModel.currentIndex, true, DirectorySystemModel.PreviewedRole)
            }

            Keys.onPressed: function (event) {
                if (event.accepted) return;

                switch (event.key) {
                    case Qt.Key_Refresh:
                    case Qt.Key_F5:
                        controller.refresh();
                        event.accepted = true;
                        break;

                    case Qt.Key_PageDown:
                    case Qt.Key_N:
                    case Qt.Key_P:
                    case Qt.Key_PageUp:
                        let isNext = (event.key === Qt.Key_PageUp || event.key === Qt.Key_P);
                        if (root.nextIndex(isNext)) {
                            event.accepted = true;
                        }
                        break;

                    case Qt.Key_Return:
                    case Qt.Key_Enter:
                        controller.openRow(selectionModel.currentIndex.row);
                        event.accepted = true;
                        break;
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
