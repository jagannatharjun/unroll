import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import QtQml.StateMachine as DSM

import filebrowser 0.1
import "../widgets"

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

    signal showStatus(string txt, int type)

    focus: true

    function _showStatus(txt, statusType) {
        root.showStatus(txt, statusType || StatusLabel.LabelType.Other)
    }

    function progress() {
        return player.position / player.duration
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
        root._showStatus(result.status)
    }

    function _changeAudioTrack() {
        const result = _changeTrack(player.audioTracks, player.activeAudioTrack, "Audio")
        player.activeAudioTrack = result.newIndex
        root._showStatus(result.status)
    }


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

    onPreviewdataChanged: {
        playbackStateMachine.startLoading()

        previewTimer.restart()
    }

    Component.onDestruction: {
        console.info("destorying player")
        FileBrowser.unsetMediaSource(player)
    }

    DSM.StateMachine {
        id: playbackStateMachine

        signal loadingCompleted()

        signal startLoading()

        running: true

        initialState: loadingState

        onLoadingCompleted: print("loading completed")

        DSM.State {
            id: loadingState

            function _calcInitialPosition() {
                if (player.duration === 0) return undefined

                let progress = (previewdata?.progress() ?? 0)
                if (progress > 0 && progress < 1)
                    return player.duration * progress
                return 0
            }

            function _init() {
                player.stop()

                if (!previewdata)
                    return

                if (!FileBrowser.setMediaSource(player, previewdata))
                    console.warn("failed to set source")

                player.play()
            }

            onEntered: {
                // state machine can only handle state change after onEntered signal handling
                Qt.callLater(_init)
            }

            Connections {
                target: player

                enabled: loadingState.active

                function onMediaStatusChanged() {
                    const initialPosition = loadingState._calcInitialPosition()
                    if (initialPosition === undefined) return
                    if (player.position < initialPosition)
                        player.position = initialPosition
                }

                function onPositionChanged() {
                    const initialPosition = loadingState._calcInitialPosition()
                    if (initialPosition === undefined) return
                    if (player.position > initialPosition) {
                        playbackStateMachine.loadingCompleted()
                    }
                }
            }

            DSM.SignalTransition {
                targetState: runningState
                signal: playbackStateMachine.loadingCompleted
            }

            DSM.SignalTransition {
                targetState: loadingState
                signal: playbackStateMachine.startLoading
            }
        }

        DSM.State {
            id: runningState

            onEntered: previewTimer.restart()

            DSM.SignalTransition {
                targetState: loadingState
                signal: playbackStateMachine.startLoading
            }
        }
    }

    Timer {
        id: previewTimer

        interval: 5000

        onTriggered: if (runningState.active) root.previewed()
    }

    MediaDevices {
        id: devices
    }

    MediaPlayer {
        id: player

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

            }
        }

        onMetaDataChanged: {
            const TitleIdx = 0
            const title = metaData.stringValue(TitleIdx)
            if (!!title)
                root._showStatus(title, StatusLabel.LabelType.FileName)
        }

        onSubtitleTracksChanged: {
            if (activeSubtitleTrack !== -1)
                return

            const subtitles = player.subtitleTracks
            for (var i in subtitles) {
                const track = subtitles[i]
                if (track.value(6) === Locale.English) {
                    activeSubtitleTrack = i
                    root._showStatus("Subtitle track: %1"
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
                root._showStatus("Playing")
            else if (player.playbackState === MediaPlayer.PausedState)
                root._showStatus("Paused")

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
        }

        Pane {
            Layout.fillWidth: true
            Layout.preferredHeight: 40

            focus: true

            RowLayout {
                anchors.fill: parent

                enabled: runningState.active

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

                    HoverHandler {
                        id: durationHoverHandler

                        target: slider
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
                            root._showStatus("%1 / %2".arg(p).arg(d))
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

                    property bool __showStatus: true
                    property bool _updateSource: true

                    from: 0
                    to: 100
                    stepSize: 5
                    focus: true
                    snapMode: Slider.SnapAlways

                    Layout.preferredWidth: 70

                    onValueChanged: {
                        if (_updateSource)
                            root.volume = value

                        if (__showStatus)
                            root._showStatus("Volume: %1%".arg(root.volume))
                    }

                    Connections {
                        target: root

                        function onVolumeChanged() {
                            if (root.volume === volSlider.value)
                                 return

                            volSlider._updateSource = false
                            volSlider.__showStatus = false

                            volSlider.value = v

                            volSlider.__showStatus = true
                            volSlider._updateSource = true
                        }
                    }

                    Component.onCompleted: {
                        volSlider.__showStatus = false

                        volSlider.value = Qt.binding(() => root.volume)

                        volSlider.__showStatus = true
                    }
                }
            }
        }
    }

    VideoFrame {
        id: videoPreview

        source: player.source

        property bool sliderLongPressed: false

        readonly property point pos: {
            const p = durationHoverHandler.point.position
            const mp = durationHoverHandler.target.mapToItem(parent, p)
            const mid = mp.x - width / 2
            const x = Math.min(Math.max(0, mid), parent.width - width)
            const y = durationHoverHandler.target.mapToItem(parent, Qt.point(0, 0)).y - height - 10
            return Qt.point(x, y)
        }


        x: pos.x

        y: pos.y

        width: implicitWidth
        height: implicitHeight

        enabled: durationHoverHandler.hovered && !sliderLongPressed

        position: {
            if (!enabled) return 0
            const pos = durationHoverHandler.point.position.x / slider.background.width
            return pos * player.duration
        }

        videoRotation: root.videoRotation

        Timer {
            id: pressedTimeout

            running: slider.pressed

            interval: 200

            onRunningChanged: if (running) videoPreview.sliderLongPressed = false

            onTriggered: videoPreview.sliderLongPressed = Qt.binding(() => slider.pressed)
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
            root._showStatus("Rotation %1°".arg(videoRotation))

            break;

        case Qt.Key_BracketLeft:
        case Qt.Key_BracketRight:
            var rate = player.playbackRate
            const jump = (event.modifiers & Qt.ControlModifier) ? .1 : .25

            rate += (event.key === Qt.Key_BracketLeft) ? - jump : jump
            if (rate <= 0 || rate > 4)
                break;

            player.playbackRate = rate
            root._showStatus("Playback Speed: %1x".arg(player.playbackRate))

            event.accepted = true
            break;

        case Qt.Key_M:
            root.muted = !root.muted
            if (root.muted)
                root._showStatus("Volume: Muted")
            else
                root._showStatus("Volume: %1%".arg(root.volume * 100))

            event.accepted = true
            break;

        case Qt.Key_Up:
        case Qt.Key_Down:
            if (!(event.modifiers & Qt.ShiftModifier))
                break

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

        case Qt.Key_Home:
            player.position = 0;

            event.accepted = true
            break;
        case Qt.Key_End:
            player.position = player.duration;

            event.accepted = true
            break;
        }

    }
}
