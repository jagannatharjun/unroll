import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15

import filebrowser 0.1

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    ListView {
        anchors.fill: parent

        model: DirectorySystemModel {
            id: directoryModel
        }

        delegate: ItemDelegate {
            text: model.name
            onClicked: {
                // how to handle archive?
//                if (model.isdir)
                    directoryModel.openindex(index)
            }
        }

        Component.onCompleted: {
            directoryModel.open("file:///e:/")
        }
    }
}
