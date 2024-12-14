import QtQuick
import QtMultimedia

Rectangle {
    id: root

    color: "white"

    property alias source: player.source

    property alias position: player.position

    readonly property real apr: {
        const res = player.metaData.value(MediaMetaData.Resolution)
        return res.width / res.height
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
    }

    VideoOutput {
        id: voutput

        anchors.fill: parent
        anchors.margins: 2
    }
}
