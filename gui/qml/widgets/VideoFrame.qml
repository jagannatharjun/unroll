import QtQuick
import QtMultimedia

Rectangle {
    id: root

    color: "white"

    property alias previewPlayer: player

    property alias source: player.source

    property int position

    property alias videoRotation: voutput.orientation

    readonly property real apr: {
        const res = player.metaData.value(MediaMetaData.Resolution)
        return !res ? 1 : res.width / res.height
    }

    implicitHeight: 150
    implicitWidth: implicitHeight * apr

    onEnabledChanged: {
        if (enabled)
            player.pause()
    }

    Component.onCompleted: player.pause()

    visible: enabled && player.mediaStatus === MediaPlayer.LoadedMedia

    MediaPlayer {
        id: player

        videoOutput: voutput

        // resolution is 1sec, we don't need this too much accurate
        position: Math.floor(root.position / 1000) * 1000

        playbackOptions.playbackIntent: PlaybackOptions.LowLatencyStreaming
    }

    VideoOutput {
        id: voutput

        anchors.fill: parent
        anchors.margins: 2
    }
}
