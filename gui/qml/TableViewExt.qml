import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    id: root

    padding: 0
    background: Rectangle {
        color: "#1E1E1E"
    }

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

    function _setCurrentIndex(row, col) {
        const idx = model.index(row, col)

        selectionModel.setCurrentIndex(idx, ItemSelectionModel.SelectCurrent)
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

            Item {
                id: headerDelegate

                y: view.contentY
                height: 32
                width: view.columnWidthProvider(index)

                property string headerText: view.model.headerData(index, Qt.Horizontal, Qt.DisplayRole)

                function resetWidth() {
                    width = Qt.binding(function () {
                        return view.columnWidthProvider(index)
                    })
                }

                function resetText() {
                    headerText = view.model.headerData(index, Qt.Horizontal, Qt.DisplayRole)
                }

                Rectangle {
                    anchors.fill: parent
                    color: "#2D2D2D"
                    border.color: "#3F3F3F"
                    border.width: 1
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        const order = (modelSortOrder === Qt.AscendingOrder)
                                    ? Qt.DescendingOrder
                                    : Qt.AscendingOrder
                        view.model.sort(index, order)
                    }
                }

                RowLayout {
                    anchors {
                        fill:parent
                        leftMargin: 8
                        rightMargin: 8
                    }
                    spacing: 4

                    Text {
                        text: headerDelegate.headerText
                        color: "#E8E8E8"
                        Layout.alignment: Qt.AlignVCenter
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    Text {
                        Layout.alignment: Qt.AlignVCenter
                        font.pixelSize: 12
                        color: "#E8E8E8"
                        visible: root.modelSortColumn === index
                        text: {
                            if (root.modelSortOrder === Qt.DescendingOrder) return "↓"
                            if (root.modelSortOrder === Qt.AscendingOrder)  return "↑"
                            return ""
                        }
                    }
                }

                HResizeHandle {
                    setResizeWidth: function (width) {
                        root._setColumnWidth(index, width)
                    }

                    Rectangle {
                        anchors {
                            right: parent.right
                            top: parent.top
                            bottom: parent.bottom
                        }
                        width: 1
                        color: "#3F3F3F"
                    }
                }

                Rectangle {
                    anchors {
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                    }
                    height: 1
                    color: "#3F3F3F"
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
            return 36
        }

        delegate: Item {
            id: delegate

            required property bool selected
            required property bool current

            width: view.columnWidthProvider(column)

            property bool isHighlighted: selected || current || row == view.selectionModel.currentIndex.row

            Rectangle {
                anchors.fill: parent
                color: {
                    if (delegate.isHighlighted) {
                        return "#0078D4"
                    } else if (row % 2 === 1) {
                        return "#252525"
                    }
                    return "#1E1E1E"
                }
                border.color: delegate.isHighlighted ? "#0078D4" : "transparent"
                border.width: delegate.isHighlighted ? 1 : 0
            }

            RowLayout {
                anchors {
                    fill: parent
                    leftMargin: 8
                }

                spacing: 6

                Image {
                    source: column === 0 ? model.iconId : ""
                    width: 24
                    height: 24
                    cache: true
                    Layout.alignment:Qt.AlignVCenter
                    visible: column === 0
                }

                Text {
                    Layout.fillWidth: true
                    text: model.display
                    color: delegate.isHighlighted ? "#FFFFFF" : "#E8E8E8"
                    elide: Text.ElideRight
                    Layout.alignment:Qt.AlignVCenter
                }
            }

            opacity: model.seen && !delegate.isHighlighted ? 0.7 : 1

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    delegate.forceActiveFocus(Qt.MouseFocusReason)
                    root._setCurrentIndex(row, column)
                }
                onDoubleClicked: {
                    delegate.forceActiveFocus(Qt.MouseFocusReason)
                    root._setCurrentIndex(row, column)
                    root.actionAtIndex(row, column)
                }
            }

            TapHandler {
                acceptedButtons: Qt.RightButton
                onTapped: eventPoint => root.rightClicked(eventPoint.globalPosition, model)
            }

            HResizeHandle {
                setResizeWidth: function (width) {
                    root._setColumnWidth(column, width)
                }

                Rectangle {
                    anchors {
                        right: parent.right
                        top: parent.top
                        bottom: parent.bottom
                    }
                    width: 1
                    color: "#3F3F3F"
                }
            }

            Rectangle {
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
                height: 1
                color: "#3F3F3F"
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

            switch (event.key)
            {
            case Qt.Key_PageUp:
            {
                if (view.currentRow - 1 >= 0) {
                    root._setCurrentIndex(view.currentRow - 1, view.currentColumn)

                    event.accepted = true
                }
                break;
            }

            case Qt.Key_PageDown:
            {
                if (view.currentRow + 1 < view.rows)
                {
                   root._setCurrentIndex(view.currentRow + 1, view.currentColumn)

                    event.accepted = true
                }
                break;
            }
            case Qt.Key_Return:
            {
                event.accepted = true
                actionAtIndex(currentRow, currentColumn)
                break;
            }
            }
        }

        ScrollBar.vertical: MyScrollBar {
            id: vscrollbar

            policy: ScrollBar.AsNeeded

            z: 11 // place it above header

            parent: view.parent
            anchors.top: view.top
            anchors.left: view.right
            anchors.bottom: hscrollbar.bottom
        }

        ScrollBar.horizontal: MyScrollBar {
            id: hscrollbar

            policy: ScrollBar.AsNeeded

            parent: view.parent
            anchors.left: view.left
            anchors.right: view.right
            anchors.top: view.bottom
        }
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

    component MyScrollBar: ScrollBar {
        id: control

        implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                                implicitContentWidth + leftPadding + rightPadding)
        implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                                 implicitContentHeight + topPadding + bottomPadding)

        padding: 2
        visible: control.policy !== ScrollBar.AlwaysOff
        minimumSize: orientation === Qt.Horizontal ? height / width : width / height

        background: Rectangle {
            color: "#1E1E1E"
        }

        contentItem: Rectangle {
            implicitWidth: control.interactive ? 8 : 4
            implicitHeight: control.interactive ? 8 : 4

            radius: width / 2
            color: {
                if (control.pressed) return "#6A6A6A"
                if (control.active) return "#555555"
                return "#404040"
            }
            opacity: {
                if (control.policy === ScrollBar.AlwaysOn) return 1.0
                if (control.active && control.size < 1.0) return 0.8
                return 0.0
            }

            Behavior on opacity {
                NumberAnimation { duration: 200 }
            }
        }
    }
}
