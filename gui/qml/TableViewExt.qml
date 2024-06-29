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

    signal rightClicked(var pos, var model)

    signal _columnWidthChanged

    function _setColumnWidth(column, width) {
        view.setColumnWidth(column, Math.max(200, width))
        root._columnWidthChanged()
        view.forceLayout()
    }

    focus: true

    Item {
        id: header

        height: children.length > 0 ? children[0].height : 0

        z: 10 // otherwise outoff view cells will cover this

        parent: view.contentItem
        y: -height // place this out of view otherwise the currentItem doesn't get in view correctly on currentChanged

        Repeater {
            id: repeater

            model: Math.max(view.columns, 0)

            ItemDelegate {
                id: headerDelegate

                y: view.contentY

                height: 36

                text: view.model.headerData(index, Qt.Horizontal,
                                            Qt.DisplayRole)

                function resetWidth() {
                    // don't use layout change signal, since that will set width to 0, when cell goes out of view
                    width = Qt.binding(function () {
                        return view.columnWidthProvider(index)
                    })
                }

                function resetText() {
                    text = Qt.binding(function () {
                        return view.model.headerData(index, Qt.Horizontal,
                                                     Qt.DisplayRole)
                    })
                }

                onPressed: {
                    const order = (modelSortOrder === Qt.AscendingOrder)
                                ? Qt.DescendingOrder
                                : Qt.AscendingOrder

                    view.model.sort(index, order)
                }

                Row {
                    anchors {
                        top: parent.top
                        bottom: parent.bottom
                        right: parent.right
                    }

                    Label {
                        anchors.verticalCenter: parent.verticalCenter

                        font: headerDelegate.font
                        visible: root.modelSortColumn === index
                        text: root.modelSortOrder === Qt.DescendingOrder ? "↓" : "↑"
                        scale: 1.1
                        rightPadding: headerDelegate.rightPadding + 10
                    }
                }

                HResizeHandle {
                    setResizeWidth: function (width) {
                        root._setColumnWidth(index, width)
                    }

                    VerticalBorder {
                        palette: headerDelegate.palette
                    }
                }

                HorizontalBorder {
                    palette: headerDelegate.palette
                }

                Connections {
                    target: root

                    function on_ColumnWidthChanged() {
                        headerDelegate.resetWidth()
                    }
                }

                Connections {
                    target: view.model

                    function onHeaderDataChanged() {
                        headerDelegate.resetText()
                    }
                }

                Connections {
                    target: view

                    function onLayoutChanged() {

                        // position header delegates
                        const item = view.itemAtCell(index, view.topRow)

                        if (item !== null) {
                            headerDelegate.visible = true
                            headerDelegate.x = item.x
                        } else {
                            headerDelegate.visible = true
                            headerDelegate.x = Qt.binding(function () {
                                const prev = index > 0
                                           ? repeater.itemAt(index - 1)
                                           : null

                                return !!prev ? prev.x + prev.width : 0
                            })
                        }
                    }
                }

                Component.onCompleted: {
                    resetText()
                    resetWidth()
                }
            }
        }
    }

    // when navigating current item of view gets hidden under scrollbar
    // wrap the view inside Pane, and set the scrollbar on Pane to fix that
    TableView {
        id: view

        anchors {
            fill: parent
            topMargin: header.height
            rightMargin: vscrollbar.width
            bottomMargin: hscrollbar.height
        }

        boundsBehavior: Flickable.StopAtBounds

        focus: rows > 0
        keyNavigationEnabled: true
        pointerNavigationEnabled: true

        reuseItems: true

        selectionBehavior: TableView.SelectCells

        selectionMode: TableView.SingleSelection

        // resizableColumns: true // this doesn't work correctly if delegate is ItemDelegate
        columnWidthProvider: function (column) {
            let w = explicitColumnWidth(column)
            if (w >= 0)
                return w
            if (column === 0)
                return 300
            return 200
        }

        rowHeightProvider: function (row) {
            return 40
        }

        delegate: ItemDelegate {
            id: delegate

            required property bool selected
            required property bool current

            text: model.display

            icon.cache: true
            icon.source: model.iconId
            icon.height: 32
            icon.width: 32

            height: 36

            focus: true

            highlighted: selected || current || row == view.selectionModel.currentIndex.row

            opacity: model.seen && !highlighted ? .6 : 1

            onClicked: {
                forceActiveFocus(Qt.MouseFocusReason)
                select()
            }

            onDoubleClicked: {
                forceActiveFocus(Qt.MouseFocusReason)
                select()

                actionAtIndex(row, column)
            }

            function select() {
                view.selectionModel.setCurrentIndex(
                            view.model.index(row, column),
                            ItemSelectionModel.SelectCurrent)
            }

            TapHandler {
                acceptedButtons: Qt.RightButton
                onTapped: eventPoint => root.rightClicked(eventPoint.globalPosition, model)
            }

            HResizeHandle {
                id: resizeHandle

                setResizeWidth: function (width) {
                    root._setColumnWidth(column, width)
                }

                VerticalBorder {
                    palette: delegate.palette
                }
            }

            HorizontalBorder {
                palette: delegate.palette
            }

            Triangle {
                width: 8
                height: 8

                color: "#FF610A"
                visible: (column == 0) && (model.showNewIndicator ?? false)
            }
        }

        Keys.onPressed: function (event) {
            if (event.accepted)
                return

            if (event.key === Qt.Key_Return) {
                event.accepted = true
                actionAtIndex(currentRow, currentColumn)
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

    component VerticalBorder: Rectangle {
        anchors {
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }

        width: 1
        color: palette.midlight
    }

    component HorizontalBorder: Rectangle {
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        height: 1
        color: palette.midlight
    }

    component Triangle : Item {
        id : tri

        clip : true

        // The index of corner for the triangle to be attached
        property int corner : 0;
        property alias color : rect.color

        Rectangle {
            id : rect

            x : tri.width * ((tri.corner+1) % 4 < 2 ? 0 : 1) - width / 2
            y : tri.height * (tri.corner    % 4 < 2 ? 0 : 1) - height / 2

            color : "red"
            antialiasing: true
            width : Math.min(tri.width, tri.height)
            height : width
            transformOrigin: Item.Center
            rotation : 45
            scale : 1.414

            radius: 3
        }
    }
}
