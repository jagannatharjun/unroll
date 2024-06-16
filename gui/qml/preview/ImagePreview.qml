import QtQuick

Item {
    id: root

    required property var previewdata

    signal previewCompleted()

    clip: true

    AnimatedImage {
        id: image

        // this item is supposed to move, following is only initial position
        x: 0
        y: 0
        width: root.width
        height: root.height

        source: root.previewdata.readUrl()
        fillMode: Image.PreserveAspectFit
        asynchronous: true

        DragHandler {}

        WheelHandler {
            property: "scale"
            acceptedModifiers: Qt.ControlModifier
        }

        Behavior on scale {
            SmoothedAnimation {
                velocity: 4
            }
        }

        onCurrentFrameChanged: {
            if (currentFrame == frameCount)
                root.previewCompleted()
        }

        onStatusChanged: {
            if (status == Image.Ready && frameCount <= 1)
                root.previewCompleted()
        }
    }
}
