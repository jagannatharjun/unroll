import QtQuick
import QtQuick.Controls


MenuBar {
    id: root

    property alias enableBack: back.enabled

    signal openFolder()
    signal back()
    signal appExit()

    Menu {
        id: fileMenu
        title: qsTr("File")

        Action {
            text: qsTr("&Open Folder")
            shortcut: StandardKey.Open
            onTriggered: root.openFolder()
        }

        Action {
            id: back

            text: qsTr("Back")
            shortcut: StandardKey.Back
            onTriggered: root.back()
        }

        Action {
            text: qsTr("&Exit")
            shortcut: StandardKey.Quit
            onTriggered: root.appExit()
        }
    }
}
