import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Templates as T
import filebrowser

Container {
    id: root

    required property string path

    signal requestPath(string path)

    readonly property var _pathcomponents: {
        return _pathButtonObject(path.split("/").filter((p) => { return p && p !== "" }))
    }

    property var _displayedPathComponents: _calcDisplayedPathButtons()

    property bool _editMode: false

    property int _pathButtonPadding: 5

    property int _pathButtonSpacing: 3

    property string _pathButtonSuffix:  "   >"

    function _pathText(path, index) {
        if (index === 0 && path.endsWith(":")) {
            const volume = FileBrowser.volumeName(path)
            return "%1 (%2)".arg(volume).arg(path)
        }

        return path
    }

    function _pathButtonObject(arr) {
        return arr.map((path, index) => ({"pathText": _pathText(path, index)
                                             , "pathIndex": index
                                             , "node": path}))
    }

    padding: 4

    hoverEnabled: false

    background: Rectangle {
        color: palette.dark
        border.color: root.visualFocus || contentItem.activeFocus ? palette.highlight : palette.light
        border.width: 1

        Behavior on border.color {
            ColorAnimation {
                duration: 80
            }
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onClicked: {
                root._editMode = !root._editMode
                contentItem.forceActiveFocus(Qt.MouseFocusReason)
            }
        }
    }

    contentItem: Loader {
        id: contentLoader

        sourceComponent: root._editMode ? editComponent : pathButtonComponent
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
                visible: root._displayedPathComponents.length !== root._pathcomponents.length

                onPressed: {
                    const o = root._pathcomponents
                    const c = root._displayedPathComponents

                    menuRepeater.model = o.slice(0, o.length - c.length).reverse()
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
                    Layout.minimumWidth: Math.min(implicitWidth, 32)
                    Layout.maximumWidth: implicitWidth + 2 // in some cases, Layout seems to elide
                    Layout.fillWidth: true // required for auto resize

                    text: modelData.pathText + root._pathButtonSuffix

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

            text: path
            color: palette.text
            focus: true
            verticalAlignment: TextInput.AlignVCenter
            padding: 5

            Keys.onEscapePressed: {
                root._editMode = !root._editMode
            }

            onAccepted: root.requestPath(text)
        }
    }

    function _requestPath(pathIndex) {
        var p = _pathcomponents.slice(0, pathIndex + 1).map(obj => obj.node).join("/")
        root.requestPath(p)
    }

    function _calcDisplayedPathButtons() {
        let aw = root.availableWidth - pathButtonFont.advanceWidth("<<") - _pathButtonPadding * 2

        // reset default state
        var r = []
        var i
        for (i = _pathcomponents.length - 1; i >= 0; --i) {
            const s = pathButtonFont.advanceWidth(_pathcomponents[i].pathText + root._pathButtonSuffix)
                    + _pathButtonPadding * 2
                    + root._pathButtonSpacing

            if (aw < s) {
                r = _pathcomponents.slice(i + 1)
                break
            } else {
                aw -= s
            }
        }

        if (i === -1)
            r = _pathcomponents // all fits
        else // nothing fits
            r = _pathcomponents.slice(_pathcomponents.length - 1)

        return r
    }
}
