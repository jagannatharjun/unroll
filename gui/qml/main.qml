import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import filebrowser 0.1
import "widgets"
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

    property alias selectionModel: controller.selectionModel

    property alias history: controller.history

    Component.onCompleted: FileBrowser.window = root
    Component.onDestruction: FileBrowser.window = null

    onPreviewdataChanged: {
        if (previewdata.fileType() === PreviewData.Unknown)
        {
            const idx = selectionModel.currentIndex
            if (!idx.valid)
                return

            const isDir = controller.model.data(idx, DirectorySystemModel.IsDirRole)
            if (isDir)
                return

            const path = controller.model.data(idx, DirectorySystemModel.PathRole)
            if (FileBrowser.isContainer(path))
                return

            _previewCompleted = true
        }
    }

    on_PreviewCompletedChanged: {
        root.updateSeen(selectionModel.currentIndex)
    }

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
        return true
    }

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


    AutoPreviewHandler {
        id: autoPreviewHandler

        currentPreviewCompleted: root._previewCompleted

        nextIndex: root.nextIndex

        onActiveChanged: {
            mainView.showStatus("Auto Preview: %1".arg(active ? "ON" : "OFF"))
        }
    }

    ViewController {
        id: controller

        fileBrowser: FileBrowser

        onShowPreview: (data) => root.previewdata = data

        onUrlChanged: Preferences.lastSessionUrl = controller.url

        Component.onCompleted: controller.openUrl(Preferences.lastSessionUrl)
    }

    Connections {
        target: selectionModel

        function onCurrentChanged(current, previous) {
            if (root.previewRow === -1 || root.previewRow !== current.row) {
                root.updateProgress(previous)

                controller.setPreview(current.row)
                root.previewRow = current.row
            }

            root._previewCompleted = false
        }

        Component.onDestruction: {
            const idx = selectionModel.currentIndex
            root.updateProgress(idx)
        }
    }

    Connections {
        target: FileBrowser

        function onOpenFolder(folder) {
            controller.openPath(folder)
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

    Shortcut {
        sequences: ["A","\\"]
        onActivated: {
            autoPreviewHandler.active = !autoPreviewHandler.active
        }
    }

    menuBar: AppMenuBar {
        enableBack: history.canMoveBack

        enableLinearizeDirAction: controller.linearizeDirAvailable

        isLinearizeChecked: controller.isLinearDir

        onlyShowVideoFile: controller.model.onlyShowVideoFile

        onOnlyShowVideoFileChanged: controller.model.onlyShowVideoFile = onlyShowVideoFile

        onRandomSort: {
            const model = controller.model
            if (!model.randomSort)
                model.randomSort = true;
            else
                model.resetRandomSeed()
        }

        onBrowseFolder: folderDialog.open()
        onOpenUrl: url => controller.openUrl(url)
        onBack: root.back()
        onAppExit: root.close()
        onLinearizeDir: (checked) => {
            if (checked)
                controller.leanOpenPath(controller.path)
            else
                controller.openPath(controller.path)
        }
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
            modelSortOrder: controller.model.randomSort ? -1 : controller.model.sortOrder
            modelSortColumn: controller.model.sortColumn

            previewdata: root.previewdata

            selectionModel: root.selectionModel

            onActionAtIndex: (row) => controller.openRow(row)

            onPreviewCompleted: {
                root._previewCompleted = true

                const previewType = root.previewdata.fileType()
                const isPlaybackTypeMedia = (previewType === PreviewData.AudioFile)
                                          || (previewType === PreviewData.VideoFile)

                if (!autoPreviewHandler.active && isPlaybackTypeMedia)
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

                    case Qt.Key_BracketLeft:
                    case Qt.Key_BracketRight:
                        const rate = autoPreviewHandler.interval
                        const jump = (event.modifiers & Qt.ControlModifier) ? 200 : 100
                        const newRate = rate + ((event.key === Qt.Key_BracketLeft) ? - jump : jump)

                        autoPreviewHandler.interval = Math.max(200, newRate)
                        mainView.showStatus("Auto Preview Time: %1ms".arg(rate))

                        event.accepted = true
                        break;

                    case Qt.Key_Up:
                    case Qt.Key_Down:
                    case Qt.Key_PageDown:
                    case Qt.Key_N:
                    case Qt.Key_P:
                    case Qt.Key_PageUp:
                        let up = (event.key === Qt.Key_PageUp || event.key === Qt.Key_P || event.key === Qt.Key_Up);
                        if (root.nextIndex(up)) {
                            event.accepted = true;

                            // disable auto preview on navigation
                            autoPreviewHandler.active = false
                        }
                        break;

                    case Qt.Key_Escape:
                    {
                        autoPreviewHandler.active = false
                        break;
                    }

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
