import QtQuick
import filebrowser

MouseArea {
    id: root

    // function (width) -> called to set width on resizing
    required property var setResizeWidth

    property bool _capturedMouse: false

    function captureMouse() {
        if (_capturedMouse)
            return

        _capturedMouse = true
        FileBrowser.setCursor(Qt.SplitHCursor)
    }

    function checkAndReleaseMouse() {
        if (!_capturedMouse) return
        if (pressed || containsMouse) return

        _capturedMouse = false
        FileBrowser.setCursor(Qt.ArrowCursor)
    }

    z: 2
    anchors.right: parent.right
    width: 6
    height: parent.height
    preventStealing: true


    property int startX: -1
    property int startWidth: -1

    hoverEnabled: true

    // FIXME: cursorShape doesn't work (tested in TableViewExt delegates)
    onEntered: root.captureMouse()
    onExited: checkAndReleaseMouse()

    onPressedChanged: {
        if (pressed) {
            startX = mapToGlobal(mouseX, 0).x
            startWidth = parent.width
        } else {
            startX = - 1
            startWidth = 0

            // containsMouse and pressed can be outof sync
            if (!containsMouse)
                root.captureMouse()
        }
    }

    onPositionChanged: {
        if (startX != -1 && pressed) {
            let globalX =  mapToGlobal(mouseX, 0).x
            let diff = (startX - globalX)

            root.setResizeWidth(startWidth - diff)
        }
    }
}
