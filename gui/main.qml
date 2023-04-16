import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15

import filebrowser 0.1

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    ViewController {
        id: controller

        Component.onCompleted: {
            controller.openUrl("file:///e:/");
        }
    }

    ListView {
        anchors.fill: parent

        model: controller.model // FIXME: on qt 5.15.2, app crashes whenever content of model changes

        delegate: ItemDelegate {
            text: model.name

            onClicked: {
                controller.clicked(index)
            }

            onDoubleClicked: {
                controller.doubleClicked(index)
            }
        }
    }
}
