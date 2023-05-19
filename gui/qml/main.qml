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

    ViewController {
        id: controller

        onShowPreview: function (data) {
            previewloader.active = data.fileType() !== PreviewData.Unknown
            if (data.fileType() === PreviewData.ImageFile)
                previewloader.setSource("qrc:/preview/ImagePreview.qml", {"previewdata": data})
            else if (data.fileType() === PreviewData.VideoFile || data.fileType() == PreviewData.AudioFile)
                previewloader.setSource("qrc:/preview/Player.qml", {"previewdata": data})
        }
    }

    HistoryController {
        id: history

        view: controller

        onResetFocus: (row, column) => {
            // FIXME: TableView.positionView* doesn't work inside this function, use a timer
            viewplacer.row = row
            viewplacer.col = column
            viewplacer.start()
        }

        Component.onCompleted: {
            history.pushUrl("file:///C:/Users/prince/Pictures/")
        }
    }

    Timer {
        id: viewplacer

        property var row
        property var col

        interval: 100
        onTriggered: {
            // positionViewAtRow doesn't take scrollbar into consideration so add offset to make sure row is visible
            let offset = row == 0 ? 0 : 30
            view.positionViewAtCell(Qt.point(col, row), TableView.Visible, Qt.point(offset, offset))
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.BackButton
        onClicked: history.pop()
    }

    FolderDialog {
        id: folderDialog

        onAccepted: {
            history.pushUrl(folderDialog.folder)
        }
    }

    SplitView {
        anchors.fill: parent

        focus: true

        Keys.priority: Keys.AfterItem
        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_O && (event.modifiers & Qt.ControlModifier)) {
                folderDialog.open()
            } else if (event.key === Qt.Key_Back || event.key === Qt.Key_Backspace) {
                history.pop()
            }
        }

        Pane {

            SplitView.fillWidth: true
            SplitView.fillHeight: true

            padding: 0

            Row {
                id: header

                z: 10 // otherwise outoff view cells will cover this
                x: - view.visibleArea.xPosition * view.contentWidth

                Repeater {
                    model: Math.max(view.columns, 0)

                    ItemDelegate {
                        text: view.model.headerData(index, Qt.Horizontal, Qt.DisplayRole)

                        // don't use layout change signal, since that will set width to 0, when cell goes out of view
                        width: view.columnWidthProvider(index)

                        onVisibleChanged: {
                            console.trace()
                            print("onvisible changed", visible, text, '\n')
                        }

                        Connections {
                            target: view.model

                            function onHeaderDataChanged() {
                                text = view.model.headerData(index, Qt.Horizontal, Qt.DisplayRole)
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

                model: controller.model // FIXME: on qt 5.15.2, app crashes whenever content of model changes
                selectionModel: controller.selectionModel
                reuseItems: true

                selectionBehavior: TableView.SelectRows


                columnWidthProvider: function (column) {
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
                        history.pushRow(row)
                    }

                    Keys.onPressed: (event) =>   {
                        if (event.key === Qt.Key_Return)
                            history.pushRow(row)
                    }

                    function select() {
                        view.selectionModel.setCurrentIndex(view.model.index(row, column), ItemSelectionModel.SelectCurrent)
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

                onCurrentRowChanged: {
                    controller.setPreview(currentRow)
                }
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
}
