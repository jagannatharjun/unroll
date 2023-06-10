import QtQuick
import QtQuick.Controls
import QtQuick.Layouts


Container {
    id: root

    required property string path

    signal requestPath(string path)

    readonly property var pathcomponents: {
        return path.split("/").filter((p) => { return p && p !== "" })
    }

    property bool _editMode: false

    padding: 2

    hoverEnabled: false

    implicitHeight: 30

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

    Component {
        id: visualComponent

        RowLayout {

            spacing: 1

            Repeater {
                model: pathcomponents

                Button {
                    text: modelData
                    onPressed: {
                        var p = pathcomponents.slice(0, index + 1).join("/")
                        root.requestPath(p)
                    }
                }
            }

            Item {
                Layout.fillHeight: true
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
            padding: 1

            Keys.onEscapePressed: {
                root._editMode = !root._editMode
            }
        }
    }

    function _setContentItem() {
        if (contentItem)
            delete contentItem

        if (!_editMode)
            contentItem = visualComponent.createObject()
        else
            contentItem = editComponent.createObject(null, {"text": path})
    }

    on_EditModeChanged: _setContentItem()

    Component.onCompleted: _setContentItem()
}
