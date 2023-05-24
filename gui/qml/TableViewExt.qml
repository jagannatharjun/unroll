import QtQuick
import QtQuick.Controls

Pane {
    id: root

    padding: 0

    property alias model: view.model
    property alias selectionModel: view.selectionModel

    property alias currentRow: view.currentRow
    property alias currentColumn: view.currentColumn

    property alias view: view

    // default QAbstractItemModel doesn't support these properties
    // the user of this class must override these to support sorting
    property int modelSortOrder
    property int modelSortColumn

    signal actionAtIndex(int row, int column)

    signal _columnWidthChanged()

    function _setColumnWidth(column, width) {
        view.setColumnWidth(column, Math.max(200, width))
        root._columnWidthChanged()
    }

    Row {
        id: header

        z: 10 // otherwise outoff view cells will cover this
        x: - view.visibleArea.xPosition * view.contentWidth
        spacing: view.rowSpacing

        Repeater {
            model: Math.max(view.columns, 0)

            ItemDelegate {
                text: view.model.headerData(index, Qt.Horizontal, Qt.DisplayRole)

                // don't use layout change signal, since that will set width to 0, when cell goes out of view
                width: view.columnWidthProvider(index)

                Connections {
                    target: root

                    function on_ColumnWidthChanged() {
                        width = Qt.binding(function() { return view.columnWidthProvider(index) })
                    }
                }

                onPressed: {
                    var order = modelSortOrder === Qt.AscendingOrder ? Qt.DescendingOrder : Qt.AscendingOrder
                    view.model.sort(index, order)
                }

                Connections {
                    target: view.model

                    function onHeaderDataChanged() {
                        text = view.model.headerData(index, Qt.Horizontal, Qt.DisplayRole)
                    }
                }

                HResizeHandle {
                    setResizeWidth: function (width) {
                        root._setColumnWidth(index, width)
                    }
                }
            }
        }
    }

    // when navigating current item of view gets hidden under scrollbar
    // wrap the view inside Pane, and set the scrollbar on Pane to fix that
    TableView {
        id: view

        anchors {
            left: parent.left
            right: parent.right
            top: header.bottom
            bottom: parent.bottom
            rightMargin: vscrollbar.width
            bottomMargin: hscrollbar.height
        }

        boundsBehavior: Flickable.StopAtBounds

        focus: true
        keyNavigationEnabled: true
        pointerNavigationEnabled: true

        reuseItems: true

        selectionBehavior: TableView.SelectRows
        // resizableColumns: true // this doesn't work correctly if delegate is ItemDelegate

        columnWidthProvider: function (column) {
            let w = explicitColumnWidth(column)
            if (w >= 0) return w
            if (column === 0) return 300
            return  200
        }

        delegate: ItemDelegate {
            required property bool selected
            required property bool current

            text: model.display

            focus: true

            highlighted: selected || current

            onClicked: {
                forceActiveFocus(Qt.MouseFocusReason)
                select()
            }

            onDoubleClicked: {
                forceActiveFocus(Qt.MouseFocusReason)
                select()

                actionAtIndex(row, column)
            }

            // TableView.KeyNavigationEnabled doesn't transfer focus, manually transfer focus so we can handle keypresse
            onCurrentChanged: forceActiveFocus()

            Keys.onPressed: function (event) {
                if (event.accepted)
                    return

                if (event.key === Qt.Key_Return) {
                    event.accepted = true
                    actionAtIndex(row, column)
                }
            }

            function select() {
                view.selectionModel.setCurrentIndex(view.model.index(row, column), ItemSelectionModel.SelectCurrent)
            }

            HResizeHandle {
                id: resizeHandle

                setResizeWidth: function (width) {
                    root._setColumnWidth(column, width)
                }
            }
        }

        ScrollBar.vertical: ScrollBar {
            id: vscrollbar

            policy: ScrollBar.AsNeeded

            z: 11 // place it above header

            parent: view.parent
            anchors.top: view.top
            anchors.left: view.right
            anchors.bottom: hscrollbar.bottom
        }

        ScrollBar.horizontal: ScrollBar {
            id: hscrollbar

            policy: ScrollBar.AsNeeded

            parent: view.parent
            anchors.left: view.left
            anchors.right: view.right
            anchors.top: view.bottom
        }
    }
}
