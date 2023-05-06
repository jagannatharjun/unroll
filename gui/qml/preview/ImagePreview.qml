import QtQuick

Image {
    required property var previewdata

    source: previewdata.readUrl()
    fillMode: Image.PreserveAspectFit
    asynchronous: true
}
