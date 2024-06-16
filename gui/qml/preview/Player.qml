import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

FocusScope {
    id: root

    required property var previewdata

    property alias muted: audioOutput.muted
    property alias volume: audioOutput.volume

    signal previewCompleted()

    focus: true

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

        audioOutput: AudioOutput {
            id: audioOutput
        }

        loops: MediaPlayer.Infinite

        onPlayingChanged: print("resolution", metaData.keys().map(
                                    (index) => [metaData.metaDataKeyToString(index), metaData.stringValue(index)]))

        onPositionChanged: {
            if (player.position === player.duration)
                root.previewCompleted()
        }
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
        focus: true

        RowLayout {
            anchors.fill: parent

            ToolButton {
                icon.source: player.playbackState === MediaPlayer.PlayingState ? "qrc:/resources/pause.svg" : "qrc:/resources/play.svg"
                icon.color: palette.buttonText
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
                stepSize: 5000 // 5 sec

                focus: true

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

                Keys.onPressed: function (event) {
                    let jump = 0
                    if (event.key === Qt.Key_Left) {
                        jump = slider.stepSize
                    } else if (event.key === Qt.Key_Right) {
                        jump = - slider.stepSize
                    }

                    if (jump !== 0 && (event.modifiers & Qt.ShiftModifier)) {
                        jump *= 2
                    }

                    value += jump
                }
            }

            Label {
                text: millisecondsToReadable(player.duration)
            }

            ToolButton {
                onPressed: audioOutput.muted = !audioOutput.muted
                icon.color: palette.buttonText
                icon.source: {
                    if (audioOutput.muted)
                        return "qrc:/resources/speaker_off.svg"
                    if (audioOutput.volume == 0)
                        return "qrc:/resources/speaker_0.svg"
                    if (audioOutput.volume < .5)
                        return "qrc:/resources/speaker_1.svg"
                    return "qrc:/resources/speaker_2.svg"
                }
            }

            Slider {
                id: volSlider
                from: 0
                to: 100
                stepSize: 1
                focus: true
                value: audioOutput.volume * to

                Layout.preferredWidth: 50

                onValueChanged: {
                    const v = value / to
                    audioOutput.volume = v
                }

                Connections {
                    target: audioOutput
                    function onVolumeChanged() {
                        const v = Math.trunc(audioOutput.volume * volSlider.to)
                        if (v != volSlider.value) {
                            volSlider.value = v
                        }
                    }
                }
            }
        }
    }

    Keys.onSpacePressed: {
        root.toogleState()
    }

    Component.onCompleted: player.play()

}
