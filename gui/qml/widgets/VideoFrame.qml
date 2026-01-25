import QtQuick
import QtMultimedia

Rectangle {
    id: root

    color: "white"

    property alias player: player

    property alias source: player.source

    property int position: player.position

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
        else
            player.stop()
    }

    onPositionChanged: {
        if (enabled)
            player.pause()
        else
            player.stop()
    }

    visible: enabled && player.mediaStatus !== MediaPlayer.PauseState

    MediaPlayer {
        id: player

        videoOutput: voutput

        // resolution is 1sec, we don't need this too much accurate
        position: (root.position / 1000) * 1000
    }

    VideoOutput {
        id: voutput

        anchors.fill: parent
        anchors.margins: 2

        visible: player.mediaStatus === MediaPlayer.LoadedMedia
    }
}
