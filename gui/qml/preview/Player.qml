import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

FocusScope {
    id: root

    required property var previewdata

    property alias muted: audioOutput.muted
    property alias volume: audioOutput.volume
    property alias videoRotation: videooutput.orientation

    property bool _progressRestored: false

    signal previewCompleted()

    function progress() {
        return player.position
    }

    function _restoreProgress() {
        if (_progressRestored)
            return;

        const previewProgress = (previewdata?.progress() ?? 0)
        player.position = previewProgress

        _progressRestored = true
    }

    onPreviewdataChanged: {
        _progressRestored = false

        player.source = previewdata.readUrl()
    }

    focus: true

    function toogleState() {
        var playbackState = player.playbackState

        switch (playbackState) {
        case MediaPlayer.PlayingState:
            player.pause()
            break;
        case MediaPlayer.PausedState:
        case MediaPlayer.StoppedState:
            player.play();
            break;
        }
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

        onPositionChanged: {
            if (player.position === player.duration)
                root.previewCompleted()
        }

        onDurationChanged: {
            // player seems to need some time to correctly handle position change here
            Qt.callLater(root._restoreProgress)
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            root.toogleState()
            if (player.playbackState === MediaPlayer.PlayingState)
                statusLabel.showStatus("Playing")
            else if (player.playbackState === MediaPlayer.PausedState)
                statusLabel.showStatus("Paused")

            root.forceActiveFocus(Qt.MouseFocusReason)
        }
    }

    ColumnLayout {
        anchors.fill: parent

        spacing: 0

        VideoOutput {
            id: videooutput

            Layout.fillWidth: true
            Layout.fillHeight: true

            StatusLabel {
                id: statusLabel

                anchors {
                    top: parent.top
                    right: parent.right
                    margins: 10
                }
            }
        }

        Pane {
            Layout.fillWidth: true
            Layout.preferredHeight: 40

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
                        if (event.key === Qt.Key_Right) {
                            jump = slider.stepSize
                        } else if (event.key === Qt.Key_Left) {
                            jump = - slider.stepSize
                        }

                        if (jump !== 0 && (event.modifiers & Qt.ShiftModifier)) {
                            jump *= 3
                        }

                        if (jump !== 0) {
                            player.position += jump
                            event.accepted = true
                        }

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
                        if (audioOutput.volume === 0)
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
                            if (v !== volSlider.value) {
                                volSlider.value = v
                            }
                        }
                    }
                }
            }
        }
    }

    Keys.onSpacePressed: {
        root.toogleState()
    }

    Keys.onPressed: function (event) {
        if (event.key === Qt.Key_R) {
            videoRotation = (videoRotation + 90) % 360
            statusLabel.showStatus("Rotation %1°".arg(videoRotation))
        }
    }

    Component.onCompleted: player.play()


    component StatusLabel : Label {
        id: lbl

        font.pixelSize: 32

        function showStatus(txt) {
            lbl.text = txt

            lbl.opacity = 1
            visibleTimer.restart()
        }

        Behavior on opacity {
            OpacityAnimator {
                duration: 400

                onStarted: {
                    if (to === 1)
                        lbl.visible = true
                }

                onFinished: {
                    if (lbl.opacity === 0)
                        lbl.visible = false
                }
            }
        }

        Timer {
            id: visibleTimer

            interval: 1600

            onTriggered: lbl.opacity = 0
        }
    }

}
