#include "filetype.hpp"

#include <string_view>
#include <unordered_set>
#include <vector>

#include <QFileInfo>
#include <QString>

const std::unordered_set<std::string_view> videoExtensions =
{
    ".3g2",
    ".3gp",
    ".amv",
    ".asf",
    ".avi",
    ".drc",
    ".flv",
    ".f4v",
    ".f4p",
    ".f4a",
    ".f4b",
    ".gif",
    ".gifv",
    ".mng",
    ".mkv",
    ".m2v",
    ".m4v",
    ".mp4",
    ".m4p",
    ".mpg",
    ".mpeg",
    ".mpe",
    ".mpv",
    ".mxf",
    ".nsv",
    ".ogv",
    ".ogg",
    ".rm",
    ".rmvb",
    ".roq",
    ".svi",
    ".vob",
    ".webm",
    ".wmv",
    ".yuv"
};


const std::unordered_set<std::string_view> imageExtensions =
{
    ".bmp",
    ".dib",
    ".exif",
    ".gif",
    ".ico",
    ".icon",
    ".jfif",
    ".jpeg",
    ".jpg",
    ".png",
    ".svg",
    ".svgz",
    ".tiff",
    ".tif",
    ".webp"
};

FileType::Type FileType::findType(const QString &path)
{
    const QByteArray ext = "." + QFileInfo(path).suffix().toLower().toUtf8();
    const std::string_view ext_view(ext.data(), ext.size());
    if (videoExtensions.count(ext_view))
       return VideoFile;
    else if (imageExtensions.count(ext_view))
       return ImageFile;
    else
        return OtherFile;
}
