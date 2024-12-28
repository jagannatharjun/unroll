import QtQuick
import QtQuick.Layouts
import "./widgets"

import filebrowser 0.1
import "preview" as Preview

Loader {
    id: root

    enum LoadedComponent
    {
        None,
        Image,
        Player
    }

    property PreviewData previewdata

    property var progress: item?.progress ?? null

    property int _loadedComp: PreviewView.LoadedComponent.None

    signal previewCompleted()

    signal previewed()

    asynchronous: true

    function showStatus(txt, statusType) {
        if (statusType === StatusLabel.LabelType.FileName)
            leftlabel.showStatus(txt)
        else
            rightLabel.showStatus(txt)
    }

    onPreviewdataChanged: {
        const fileType = previewdata.fileType();
        _setComponentFromPreviewType(fileType);
    }

    // Function to set the appropriate component based on preview type
    function _setComponentFromPreviewType(previewType) {
        root.active = (previewType !== PreviewData.Unknown);

        const componentMap = {
            [PreviewData.ImageFile]: { component: imageComponent, type: PreviewView.LoadedComponent.Image },
            [PreviewData.VideoFile]: { component: playerComponent, type: PreviewView.LoadedComponent.Player },
            [PreviewData.AudioFile]: { component: playerComponent, type: PreviewView.LoadedComponent.Player },
        };

        const selected = componentMap[previewType] || { component: undefined, type: PreviewView.LoadedComponent.None };

        if (_loadedComp !== selected.type) {
            sourceComponent = selected.component;
            _loadedComp = selected.type;
        }
    }

    Component {
        id: playerComponent

        Preview.Player {
            focus: true

            previewdata: root.previewdata

            // TODO: track preferences value
            volume: Preferences.volume

            muted: Preferences.volumeMuted

            videoRotation: Preferences.videoRotation

            onMutedChanged: {
                Preferences.volumeMuted = muted
            }

            onVolumeChanged: {
                Preferences.volume = volume
            }

            onVideoRotationChanged: {
                Preferences.videoRotation = videoRotation
            }

            onPreviewCompleted: root.previewCompleted()

            onPreviewed: root.previewed()

            onShowStatus: (txt, type) => root.showStatus(txt, type)
        }
    }

    Component {
        id: imageComponent

        Preview.ImagePreview {
            previewdata: root.previewdata

            onPreviewCompleted: {
                root.previewCompleted()

                root.previewed()
            }
        }
    }


    RowLayout {
        z: 10
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: 10
        }

        StatusLabel {
            id: leftlabel

            sourceWidth: root.width
            sourceHeight: root.height

            Layout.fillWidth: true
        }

        Item {
            Layout.fillWidth: true
            Layout.minimumWidth: 32
        }

        StatusLabel {
            id: rightLabel

            sourceWidth: root.width
            sourceHeight: root.height
        }
    }
}
