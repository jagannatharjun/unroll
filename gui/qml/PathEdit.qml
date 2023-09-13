import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Templates as T

Container {
    id: root

    required property string path

    signal requestPath(string path)

    readonly property var pathcomponents: {
        return path.split("/").filter((p) => { return p && p !== "" })
    }

    property var _displayedPathComponents: []

    property bool _editMode: false

    padding: 4

    hoverEnabled: false

    background: Rectangle {
        color: palette.dark
        border.color: root.visualFocus || contentItem.activeFocus ? palette.highlight : palette.light
        border.width: 1

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onClicked: {
                root._editMode = !root._editMode
                contentItem.forceActiveFocus(Qt.MouseFocusReason)
            }
        }
    }

    component PathButton : Button {
        padding: 5
        background: Rectangle {
            color: visualFocus ? palette.active.button : palette.inactive.button
        }
    }

    Component {
        id: pathButtonComponent

        // use wrapper Item, so RowLayout don't spread children in availableWidth
        Item {
            implicitWidth: row.implicitWidth
            implicitHeight: row.implicitHeight

            RowLayout {
                id: row

                spacing: 3

                PathButton {
                    text: "<<"
                    visible: root._displayedPathComponents.length !== root.pathcomponents.length

                    onPressed: {
                        var o = root.pathcomponents, c = root._displayedPathComponents
                        menuRepeater.model = o.slice(0, o.length - c.length)
                                                .map((path, index) => ({"pathText": path, "pathIndex": index}))

                        rootMenu.open()
                    }

                    Menu {
                        id: rootMenu

                        y: parent.height

                        Repeater {
                            id: menuRepeater

                            delegate: MenuItem {
                                text: modelData.pathText
                                onPressed: root._requestPath(modelData.pathIndex)
                            }
                        }
                    }
                }

                Repeater {
                    model: root._displayedPathComponents

                    delegate: PathButton {
                        text: modelData.pathText
                        onPressed: root._requestPath(modelData.pathIndex)
                    }
                }


                onImplicitWidthChanged: {
                    if (implicitWidth > root.availableWidth) {
                        // saves binding loop warning
                        Qt.callLater(root._shiftDisplayedPath)
                    }
                }
            }
        }
    }

    Component {
        id: editComponent

        TextInput {
            id: textInput

            color: palette.text
            focus: true
            verticalAlignment: TextInput.AlignVCenter
            padding: 5

            Keys.onEscapePressed: {
                root._editMode = !root._editMode
            }
        }
    }

    function _setContentItem() {
        if (contentItem)
            delete contentItem

        if (_editMode) {
            contentItem = editComponent.createObject(null, {"text": path})
            return
        }

        _displayedPathComponents = pathcomponents.map((path, index) => ({"pathText": path, "pathIndex": index }))
        contentItem = pathButtonComponent.createObject(null)
    }

    function _requestPath(pathIndex) {
        var p = pathcomponents.slice(0, pathIndex + 1).join("/")
        root.requestPath(p)
    }

    function _shiftDisplayedPath() {
        if (_displayedPathComponents.length <= 1)
            return

        var p = root._displayedPathComponents
        p.shift()
        root._displayedPathComponents = p
    }


    on_EditModeChanged: Qt.callLater(_setContentItem)
    onWidthChanged: Qt.callLater(_setContentItem)
    onPathcomponentsChanged: Qt.callLater(_setContentItem)

    Component.onCompleted: _setContentItem()
}
