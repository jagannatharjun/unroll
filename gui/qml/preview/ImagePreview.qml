import QtQuick

Item {
    id: root

    required property var previewdata
    property real imageScale: 1.
    clip: true

    AnimatedImage {

        // this item is supposed to move, following is only initial position
        x: 0
        y: 0
        width: root.width
        height: root.height

        source: root.previewdata.readUrl()
        fillMode: Image.PreserveAspectFit
        asynchronous: true

        scale: root.imageScale

        DragHandler {}
    }

    MouseArea {
        anchors.fill: parent
        onClicked: function (mouse) {
            // scale.origin.x = mouse.x
            // scale.origin.y = mouse.y
            root.imageScale += .1
        }
    }
}
