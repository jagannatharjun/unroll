import QtQuick
import QtQuick.Controls

import filebrowser 0.1

MenuBar {
    id: root

    property alias enableBack: back.enabled

    property alias enableLinearizeDirAction: linearizeDir.enabled

    property alias isLinearizeChecked: linearizeDir.checked

    signal linearizeDir(bool checked)
    signal browseFolder()
    signal openUrl(string url)
    signal back()
    signal appExit()

    Menu {
        id: fileMenu
        title: qsTr("File")

        Action {
            text: qsTr("&Open Folder")
            shortcut: StandardKey.Open
            onTriggered: root.browseFolder()
        }

        Menu {
            id: recentFilesMenu
            title: qsTr("&Recent")

            visible: Preferences.recentUrls.length > 0

            Instantiator {
                model: Preferences.recentUrls

                delegate: MenuItem {
                    text: modelData
                    onTriggered: root.openUrl(modelData)
                }
                onObjectAdded: (index, object) => recentFilesMenu.insertItem(index, object)
                onObjectRemoved: (index, object) => recentFilesMenu.removeItem(object)

            }
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

    Menu {
        id: viewMenu

        title: qsTr("View")

        Action {
            id: linearizeDir

            text: qsTr("Linearize Directory")
            onCheckedChanged: root.linearizeDir(this.checked)

            checkable: true
        }
    }
}
