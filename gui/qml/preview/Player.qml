import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

Item {
    id: root

    required property var previewdata

    function toogleState() {
        var playbackState = player.playbackState

        if (playbackState == MediaPlayer.PlayingState)
            player.pause()
        else if (playbackState === MediaPlayer.PausedState)
            player.play()
        else if (playbackState === MediaPlayer.StoppedState)
            player.play()
    }

    function millisecondsToReadable(ms) {
        // Calculate the number of hours, minutes, and seconds
        const seconds = Math.floor(ms / 1000) % 60
        const minutes = Math.floor(ms / (1000 * 60)) % 60
        const hours = Math.floor(ms / (1000 * 60 * 60))

        // Convert the values into two-digit format
        const formattedSeconds = String(seconds).padStart(2, "0")
        const formattedMinutes = String(minutes).padStart(2, "0")
        let formattedTime = `${formattedMinutes}:${formattedSeconds}`

        // Add hours if it's greater than zero
        if (hours > 0) {
            const formattedHours = String(hours).padStart(2, "0")
            formattedTime = `${formattedHours}:${formattedTime}`
        }

        return formattedTime
    }

    MediaPlayer {
        id: player

        source: previewdata.readUrl()

        videoOutput: videooutput
        loops: MediaPlayer.Infinite
    }

    VideoOutput {
        id: videooutput

        anchors.fill: root
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.toogleState()
    }

    Pane {
        anchors {
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }

        height: 40

        RowLayout {
            anchors.fill: parent

            ToolButton {
                icon.source: player.playbackState === MediaPlayer.PlayingState ? "qrc:/resources/pause.qml" : "qrc:/resources/play.qml"
                icon.color: "white" // TODO: manage theme
                onClicked: root.toogleState()
            }

            Label {
                text: millisecondsToReadable(player.position)
            }

            Slider {
                id: slider

                Layout.fillHeight: true
                Layout.fillWidth: true

                from: 0
                to: player.duration
                stepSize: 1000

                Binding {
                    target: slider
                    property: "value"
                    when: !slider.pressed
                    value: player.position
                }

                onValueChanged: {
                    if (player.position !== slider.value)
                        player.position = slider.value
                }
            }

            Label {
                text: millisecondsToReadable(player.duration)
            }
        }
    }

    Component.onCompleted: player.play()
}
