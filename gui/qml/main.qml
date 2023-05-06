import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs
import Qt.labs.platform

import filebrowser 0.1
import "preview" as Preview

Window {
    id: root

    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    property var history: []

    function back() {
        if (history.length > 1) {
            history.pop()

            var top = history[history.length - 1]
            push(top)
        }
    }

    function push(url) {
        // try to open url first, if it succeded
        // controller.url will updated, and only then
        // history will be updated
        controller.openUrl(url)
    }


    ViewController {
        id: controller

        onUrlChanged: {
            if (history.length == 0 || history[history.length - 1] !== controller.url) {
                history.push(controller.url)
            }
        }

        onShowPreview: function (data) {
            previewloader.setSource("qrc:/preview/ImagePreview.qml", {"previewdata": data})
        }

        Component.onCompleted: {
            push("file:///C:/Users/prince/Pictures/")
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.BackButton
        onClicked: root.back()
    }

    FolderDialog {
        id: folderDialog

        onAccepted: {
            push(folderDialog.folder)
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
                back()
            }
        }

        ListView {
            id: view

            SplitView.fillWidth: true
            SplitView.fillHeight: true

            boundsBehavior: Flickable.StopAtBounds

            focus: true
            keyNavigationEnabled: true

            model: controller.model // FIXME: on qt 5.15.2, app crashes whenever content of model changes

            delegate: ItemDelegate {
                text: model.name

                focus: true

                width: view.width

                highlighted: view.currentIndex == index

                onActiveFocusChanged: {
                    if (!activeFocus)
                        return

                    view._makeCurrentItem(index)
                }

                onClicked: {
                    view._makeCurrentItem(index)
                }

                onDoubleClicked: {
                    controller.doubleClicked(index)
                }

                Keys.onPressed: (event) =>   {
                    if (event.key === Qt.Key_Return)
                        controller.doubleClicked(index)
                }
            }

            function _makeCurrentItem(index) {
                view.currentIndex = index
                view.forceActiveFocus(Qt.MouseFocusReason)

                controller.clicked(index)
            }
        }

        Loader {
            id: previewloader

            SplitView.preferredWidth: root.width / 2
            SplitView.fillHeight: true

            asynchronous: true
        }
    }


}
