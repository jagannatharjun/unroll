import QtQuick

import filebrowser 0.1
import "preview" as Preview

Loader {
    id: root

    property PreviewData previewdata

    signal previewCompleted()

    asynchronous: true

    onItemChanged: {
        if (!!item)
            item.previewCompleted.connect(root.previewCompleted)
    }

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

            onMutedChanged: {
                Preferences.volumeMuted = muted
            }

            onVolumeChanged: {
                Preferences.volume = volume
            }
        }
    }

    Component {
        id: imageComponent

        Preview.ImagePreview {
            previewdata: root.previewdata
        }
    }
}
