import QtQuick
import QtQuick.Controls

Label {
    id: lbl

    enum LabelType {
        Other,
        FileName
    }

    required property int sourceWidth
    required property int sourceHeight

    font.pointSize: 18 * Math.max(sourceWidth, sourceHeight) / 1000

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
