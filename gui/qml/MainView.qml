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

    property var progress

    signal actionAtIndex(int row, int column)

    signal previewCompleted

    Component.onCompleted: {
        splitView.restoreState(Preferences.mainSplitviewState())
    }

    Component.onDestruction: Preferences.setMainSplitViewState(splitView.saveState())

    handle: Rectangle {
        implicitWidth: 4
        implicitHeight: 4

        color: SplitHandle.pressed ? palette.highlight
               : (SplitHandle.hovered ? palette.button : palette.base)
    }

    TableViewExt {
        id: tableView

        SplitView.minimumWidth: 200

        focus: true

        onActionAtIndex: function (row, col) {
            splitView.actionAtIndex(row, col)
        }
    }

    Pane {
        // tableview content can overflow, so wrap preview inside Pane,
        // so the extra content is not visible

        SplitView.fillWidth: true

        PreviewView {
            id: previewView

            anchors.fill: parent

            onPreviewCompleted: splitView.previewCompleted()
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
        if (event.key === Qt.Key_Z || event.key === Qt.Key_N)
            tableView.forceActiveFocus(Qt.TabFocusReason)
        else if (event.key === Qt.Key_X || event.key === Qt.Key_M)
            previewView.forceActiveFocus(Qt.TabFocusReason)
    }
}

