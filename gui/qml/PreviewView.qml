import QtQuick

import filebrowser 0.1
import "preview" as Preview

Loader {
    id: root

    property PreviewData previewdata

    onPreviewdataChanged:  {
        const fileType = previewdata.fileType()
        active = (fileType !== PreviewData.Unknown)

        // reset state otherwise Player will crash with invalid inputs
        sourceComponent = undefined

        if (!active)
            return

        if (fileType === PreviewData.ImageFile) {

            setSource("qrc:/preview/ImagePreview.qml"
                      , {
                          "previewdata": previewdata
                      })

        } else if ((fileType === PreviewData.VideoFile)
                   || (fileType === PreviewData.AudioFile)) {

            sourceComponent = playerComponent
        }
    }

    asynchronous: true

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
}
