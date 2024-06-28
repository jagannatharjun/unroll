import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

FocusScope {
    id: root

    required property var previewdata

    property alias muted: audioOutput.muted

    // volume value 1-100
    property int volume

    property alias videoRotation: videooutput.orientation

    property bool _progressRestored: false

    property int _positionRounded: Math.floor(player.position / 1000) * 1000

    signal previewCompleted()

    signal previewed()

    function progress() {
        return player.position / player.duration
    }

    function _restoreProgress() {
        if (_progressRestored)
            return;

        let position = (previewdata?.progress() ?? 0)
        if (position > 0 && position < 1)
            position = player.duration * position
        if (position !== player.duration)
            player.position = position

        _progressRestored = true
    }

    function _changeTrack(tracks, activeTrackIndex, type) {
        const next = activeTrackIndex + 1
        if (next === tracks.length) {
            return {
                newIndex: -1,
                status: `${type} track: Disable`
            }
        } else {
            const track = tracks[next]
            let text = track.stringValue(6) // Language
            if (!text) text = track.stringValue(0) // Title
            if (!text) text = `Track ${next + 1}`

            return {
                newIndex: next,
                status: `${type} track: ${text}`
            }
        }
    }

    function _changeSubtitleTrack() {
        const result = _changeTrack(player.subtitleTracks, player.activeSubtitleTrack, "Subtitle")
        player.activeSubtitleTrack = result.newIndex
        statusLabel.showStatus(result.status)
    }

    function _changeAudioTrack() {
        const result = _changeTrack(player.audioTracks, player.activeAudioTrack, "Audio")
        player.activeAudioTrack = result.newIndex
        statusLabel.showStatus(result.status)
    }

    onPreviewdataChanged: {
        _progressRestored = false

        player.source = previewdata.readUrl()

        previewTimer.restart()
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

    Timer {
        id: previewTimer

        interval: 5000

        onTriggered: root.previewed()
    }

    MediaDevices {
        id: devices
    }

    MediaPlayer {
        id: player

        source: previewdata.readUrl()

        videoOutput: videooutput

        audioOutput: AudioOutput {
            id: audioOutput

            device: devices.defaultAudioOutput

            volume:  - Math.log(1 - (root.volume / 100.)) / 4.60517018599 /*LOG 100*/
        }

        onMediaStatusChanged: {
            switch (mediaStatus) {
            case MediaPlayer.EndOfMedia:
                root.previewCompleted()
                break;

            case MediaPlayer.LoadedMedia:
                root._restoreProgress()
                break;
            }
        }

        onMetaDataChanged: {
            const TitleIdx = 0
            const title = metaData.stringValue(TitleIdx)
            if (!!title)
                fileLabel.showStatus(title)
        }

        onSubtitleTracksChanged: {
            if (activeSubtitleTrack !== -1)
                return

            const subtitles = player.subtitleTracks
            for (var i in subtitles) {
                const track = subtitles[i]
                if (track.value(6) === Locale.English) {
                    activeSubtitleTrack = i
                    statusLabel.showStatus("Subtitle track: %1"
                                           .arg(track.stringValue(6)))
                }
            }
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

        onWheel: function (wheel)  {
            if (wheel.angleDelta.y === 0)
                return

            const y = wheel.angleDelta.y
            const steps = Math.abs(y) / 120
            const delta = steps * volSlider.stepSize * (y > 0 ? 1 : -1) * (wheel.inverted ? - 1 : 1)

            volSlider.value += delta
        }
    }

    ColumnLayout {
        anchors.fill: parent

        spacing: 0

        Rectangle {

            Layout.fillWidth: true
            Layout.fillHeight: true

            color: "black"

            VideoOutput {
                id: videooutput

                anchors.fill: parent
            }

            RowLayout {
                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                    margins: 10
                }

                StatusLabel {
                    id: fileLabel

                    Layout.fillWidth: true
                }

                Item {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 32
                }

                StatusLabel {
                    id: statusLabel
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
                    text: millisecondsToReadable(root._positionRounded)
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

                            const p = root.millisecondsToReadable(player.position)
                            const d = root.millisecondsToReadable(player.duration)
                            statusLabel.showStatus("%1 / %2".arg(p).arg(d))
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

                    property bool _showStatus: true
                    property bool _updateSource: true

                    from: 0
                    to: 100
                    stepSize: 5
                    focus: true
                    value: root.volume
                    snapMode: Slider.SnapAlways

                    Layout.preferredWidth: 70

                    onValueChanged: {
                        if (_updateSource)
                            root.volume = value

                        if (_showStatus)
                            statusLabel.showStatus("Volume: %1%".arg(root.volume))
                    }

                    Connections {
                        target: root

                        function onVolumeChanged() {
                            if (root.volume === volSlider.value)
                                 return

                            volSlider._updateSource = false
                            volSlider._showStatus = false

                            volSlider.value = v

                            volSlider._showStatus = true
                            volSlider._updateSource = true
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
        switch (event.key)
        {
        case Qt.Key_R:
            event.accepted = true

            videoRotation = (videoRotation + 90) % 360
            statusLabel.showStatus("Rotation %1°".arg(videoRotation))

            break;

        case Qt.Key_BracketLeft:
        case Qt.Key_BracketRight:
            var rate = player.playbackRate
            const jump = (event.modifiers & Qt.ControlModifier) ? .1 : .25

            rate += (event.key === Qt.Key_BracketLeft) ? - jump : jump
            if (rate <= 0 || rate > 4)
                break;

            player.playbackRate = rate
            statusLabel.showStatus("Playback Speed: %1x".arg(player.playbackRate))

            event.accepted = true
            break;

        case Qt.Key_M:
            root.muted = !root.muted
            if (root.muted)
                statusLabel.showStatus("Volume: Muted")
            else
                statusLabel.showStatus("Volume: %1%".arg(root.volume * 100))

            event.accepted = true
            break;

        case Qt.Key_Up:
        case Qt.Key_Down:
            if (event.key === Qt.Key_Up)
                volSlider.increase()
            else
                volSlider.decrease()

            event.accepted = true
            break;

        case Qt.Key_V:
            root._changeSubtitleTrack()

            event.accepted = true
            break;

        case Qt.Key_B:
            root._changeAudioTrack()

            event.accepted = true
            break;
        }
    }

    Component.onCompleted: player.play()

    component StatusLabel : Label {
        id: lbl

        font.pointSize: 18 * Math.max(root.width, root.height) / 1000

        visible: opacity > 0

        style: Text.Outline

        color: "white"
        styleColor: "black"

        elide: Text.ElideRight

        Label {
            anchors {
                top: parent.top
                left: parent.left
                topMargin: 3
                leftMargin: 3
            }

            z: -1
            width: parent.width
            height: parent.width
            color: "black"
            opacity: .45
            text: lbl.text
            font: lbl.font
            elide: lbl.elide
        }

        function showStatus(txt) {
            lbl.text = txt

            lbl.opacity = 1
            visibleTimer.restart()
        }

        OpacityAnimator {
            id: oAnim

            target: lbl
            duration: 400
        }

        Timer {
            id: visibleTimer

            interval: 1600

            onTriggered: {
                oAnim.to = 0
                oAnim.start()
            }
        }
    }
}
