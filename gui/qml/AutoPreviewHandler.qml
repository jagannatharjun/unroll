import QtQuick

Timer {
    id: root

    property bool active: false

    required property bool currentPreviewCompleted

    // function() -> bool, jump to next index, return true if successful
    required property var nextIndex

    property bool _nextOnPreviewCompleted: false

    function _next() {
        if (!root.nextIndex())
            active = false
        else
            start()
    }

    interval: 900

    running: false

    repeat: false

    onActiveChanged:  {
        _nextOnPreviewCompleted = false // reset state

        if (active)
            restart()
    }

    onTriggered: {
        if (!active)
            return

        if (currentPreviewCompleted) {
            _next()
        } else {
            _nextOnPreviewCompleted = true
        }
    }

    onCurrentPreviewCompletedChanged: {
        if (!active)
            return

        if (_nextOnPreviewCompleted) {
            Qt.callLater(_next) // otherwise causes binding loop warning

            _nextOnPreviewCompleted = false
        }
    }
}
