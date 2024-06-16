import QtQuick

Item {
    id: root

    required property var previewdata

    signal previewCompleted()

    clip: true

    Timer {
        id: previewTimer

        interval: 500

        onTriggered: {
            if (image.currentFrame === image.frameCount || image.frameCount === 1)
                root.previewCompleted()
        }
    }

    onPreviewdataChanged: previewTimer.restart()

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
            if (currentFrame == frameCount && !previewTimer.running)
                root.previewCompleted()
        }

        onStatusChanged: {
            if (status == Image.Ready)
                root.previewCompleted()
        }
    }
}
