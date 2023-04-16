import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15

import filebrowser 0.1

Window {
    id: root

    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    property var history: []

    function back() {
        if (history.length > 0) {
            history.pop()

            push(history[history.length - 1])
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

        Component.onCompleted: {
            push("file:///e:/")
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.BackButton
        onClicked: root.back()
    }

    ListView {
        id: view

        anchors.fill: parent

        boundsBehavior: Flickable.StopAtBounds

        model: controller.model // FIXME: on qt 5.15.2, app crashes whenever content of model changes

        delegate: ItemDelegate {
            text: model.name

            width: view.width

            highlighted: view.currentIndex == index

            onClicked: {
                view.currentIndex = index

                controller.clicked(index)
            }

            onDoubleClicked: {
                controller.doubleClicked(index)
            }
        }
    }
}
