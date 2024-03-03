import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Templates as T
import filebrowser

Container {
    id: root

    required property string path

    signal requestPath(string path)

    readonly property var pathcomponents: {
        return path.split("/").filter((p) => { return p && p !== "" })
    }

    property var _displayedPathComponents: []

    property bool _editMode: false

    property int _pathButtonPadding: 5

    property int _pathButtonSpacing: 3

    function _pathButtonObject(arr) {
        return arr.map((path, index) => ({"pathText": path, "pathIndex": index}))
    }

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
        font: pathButtonFont.font
        padding: root._pathButtonPadding
        hoverEnabled: true
        background: Rectangle {
            color: palette.active.button
            visible: (visualFocus || hovered)
        }
    }

    FontMetrics {
        id: pathButtonFont

    }

    Component {
        id: pathButtonComponent


        RowLayout {
            id: row

            spacing: root._pathButtonSpacing

            PathButton {
                id: menuButton

                text: "<<"
                visible: root._displayedPathComponents.length !== root.pathcomponents.length

                onPressed: {
                    var o = root.pathcomponents, c = root._displayedPathComponents
                    menuRepeater.model = root._pathButtonObject(o.slice(0, o.length - c.length)).reverse()
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
                    Layout.minimumWidth: Math.min(implicitWidth, row.width - menuButton.width - row.spacing * 2)
                    Layout.maximumWidth: implicitWidth
                    Layout.fillWidth: true // required for auto resize

                    text: {
                        const suffix = "   >"
                        // format volume name
                        if (modelData.pathIndex === 0) {
                            const volume = FileBrowser.volumeName(modelData.pathText)
                            return "%1 (%2)".arg(volume).arg(modelData.pathText) + suffix
                        }

                        return modelData.pathText + suffix
                    }

                    onPressed: root._requestPath(modelData.pathIndex)
                }
            }

            Item {
                Layout.fillWidth: true
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

        _displayedPathComponents = _calcDisplayedPathButtons()
        contentItem = pathButtonComponent.createObject(null)
    }

    function _requestPath(pathIndex) {
        var p = pathcomponents.slice(0, pathIndex + 1).join("/")
        root.requestPath(p)
    }

    function _calcDisplayedPathButtons() {
        let aw = root.availableWidth - pathButtonFont.advanceWidth("<<") - _pathButtonPadding * 2

        // reset default state
        var r = []
        var i
        for (i = pathcomponents.length - 1; i >= 0; --i) {
            const s = pathButtonFont.advanceWidth(pathcomponents[i])
                    + _pathButtonPadding * 2
                    + root._pathButtonSpacing

            if (aw < s) {
                r = _pathButtonObject(pathcomponents.slice(i + 1))
                break
            } else {
                aw -= s
            }
        }

        if (i === -1)
            r = _pathButtonObject(pathcomponents) // all fits
        else // nothing fits
            r = _pathButtonObject(pathcomponents.slice(pathcomponents.length - 1))

        return r
    }

    onAvailableWidthChanged: {
        if (_editMode)
            return

        var p = _calcDisplayedPathButtons()
        if (p !== _displayedPathComponents)
            _displayedPathComponents = p
    }

    on_EditModeChanged: Qt.callLater(_setContentItem)
    onPathcomponentsChanged: Qt.callLater(_setContentItem)

    Component.onCompleted: _setContentItem()
}
