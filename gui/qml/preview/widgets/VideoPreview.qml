import QtQuick
import QtMultimedia
import filebrowser
Rectangle {
    id: root

    color: "white"

    property alias source: rendrer.videoPath

    property int position

    function msToTimestamp(milliseconds) {
         const hours = Math.floor(milliseconds / (1000 * 60 * 60));
         const minutes = Math.floor((milliseconds % (1000 * 60 * 60)) / (1000 * 60));
         const seconds = Math.floor((milliseconds % (1000 * 60)) / 1000);
         const ms = milliseconds % 1000;

         return `${hours.toString().padStart(2, "0")}:` +
                `${minutes.toString().padStart(2, "0")}:` +
                `${seconds.toString().padStart(2, "0")}.` +
                `${ms.toString().padStart(3, "0")}`;
     }

    // readonly property real apr: {
    //     const res = player.metaData.value(MediaMetaData.Resolution)
    //     return res.width / res.height
    // }

    // implicitHeight: 150
    // implicitWidth: implicitHeight * apr

    // onEnabledChanged: {
    //     if (enabled)
    //         player.pause()
    //     else
    //         player.stop()
    // }

    // onPositionChanged: {

    //     if (enabled)
    //         player.pause()
    //     else
    //         player.stop()
    // }

    // visible: enabled && player.mediaStatus !== MediaPlayer.PauseState

    FrameRenderer {

        id: rendrer
        width: 100
        height: 100

        timestamp: msToTimestamp(position)
    }

    // MediaPlayer {
    //     id: player

    //     videoOutput: voutput
    // }

    // VideoOutput {
    //     id: voutput

    //     anchors.fill: parent
    //     anchors.margins: 2
    // }
}
