import QtQuick

import filebrowser 0.1
import "preview" as Preview

Loader {
    id: root

    property PreviewData previewdata

    property var progress: item?.progress ?? null

    signal previewCompleted()

    asynchronous: true

    onPreviewdataChanged:  {
        const fileType = previewdata.fileType()
        root.active = (fileType !== PreviewData.Unknown)

        // reset state otherwise Player will crash with invalid inputs
        sourceComponent = undefined

        if (!active)
            return

        if (fileType === PreviewData.ImageFile) {
            sourceComponent = imageComponent
        } else if ((fileType === PreviewData.VideoFile)
                   || (fileType === PreviewData.AudioFile)) {

            sourceComponent = playerComponent
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
        }
    }

    Component {
        id: imageComponent

        Preview.ImagePreview {
            previewdata: root.previewdata

            onPreviewCompleted: root.previewCompleted()
        }
    }
}
