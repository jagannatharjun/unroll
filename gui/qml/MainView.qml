import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import filebrowser 0.1

SplitView {
    id: splitView

    property alias model: tableView.model
    property alias modelSortOrder: tableView.modelSortOrder
    property alias modelSortColumn: tableView.modelSortColumn
    property alias selectionModel: tableView.selectionModel

    property alias previewdata: previewView.previewdata

    property alias previewProgress: previewView.progress

    property bool _showTableView: Preferences.showMainFileView
    on_ShowTableViewChanged: Preferences.showMainFileView = _showTableView

    property var progress

    property var showStatus: previewView.showStatus

    signal actionAtIndex(int row, int column)

    signal previewCompleted

    signal previewed

    focus: true

    Component.onCompleted: {
        splitView.restoreState(Preferences.mainSplitviewState())

        previewView.forceActiveFocus()
    }

    Component.onDestruction: Preferences.setMainSplitViewState(splitView.saveState())

    handle: Rectangle {
        implicitWidth: 4
        implicitHeight: 4

        color: {
            if (SplitHandle.pressed) return "#0078D4"
            if (SplitHandle.hovered) return "#404040"
            return "#2D2D2D"
        }

        HoverHandler {
            onHoveredChanged: {
                if (hovered) FileBrowser.setCursor(Qt.SplitHCursor)
                else FileBrowser.setCursor(Qt.ArrowCursor)
            }
        }
    }

    TableViewExt {
        id: tableView

        SplitView.minimumWidth: 200

        focus: true

        visible: splitView._showTableView || (previewdata?.fileType() ?? PreviewData.Unknown) === PreviewData.Unknown

        onActionAtIndex: function (row, col) {
            splitView.actionAtIndex(row, col)
        }

        onRightClicked: function (pos, model) {
            FileBrowser.showFileContextMenu(pos, model.path)
        }
    }

    Pane {
        // tableview content can overflow, so wrap preview inside Pane,
        // so the extra content is not visible

        SplitView.fillWidth: true
        background: Rectangle {
            color: "#1E1E1E"
        }

        PreviewView {
            id: previewView

            anchors.fill: parent

            onPreviewCompleted: splitView.previewCompleted()

            onPreviewed: splitView.previewed()
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

    Connections {
        target: selectionModel

        function onCurrentChanged(current, previous) {
            // FIXME: TableView.positionView* doesn't work inside this function, use a timer
            viewplacer.row = current.row
            viewplacer.col = current.column
            viewplacer.start()
        }
    }

    Keys.onPressed: function (event) {
        switch (event.key) {
            case Qt.Key_Z:
            case Qt.Key_Comma:
                tableView.forceActiveFocus(Qt.TabFocusReason)
                event.accepted = true
                break

            case Qt.Key_X:
            case Qt.Key_Period:
                previewView.forceActiveFocus(Qt.TabFocusReason)
                event.accepted = true
                break

            case Qt.Key_F1:
                splitView._showTableView = !splitView._showTableView
                event.accepted = true
                break;
        }
    }
}

