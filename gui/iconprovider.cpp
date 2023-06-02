#include "iconprovider.hpp"

#include "../core/directorysystem.hpp"
#include "qurlquery.h"

#include <QDir>
#include <QFileInfo>

#include <qt_windows.h>

QImage fileImage(const QString &path, const bool isdir)
{
    SHFILEINFOW fileInfo {};

    const auto attribute = FILE_ATTRIBUTE_NORMAL | (isdir ? FILE_ATTRIBUTE_DIRECTORY : 0);
    const auto nativeName = QString(path).replace("/", "\\").toStdWString();

    if (SHGetFileInfoW(nativeName.c_str(), attribute, &fileInfo
                 , sizeof fileInfo, SHGFI_ICON) == 0)
    {
        memset(&fileInfo, 0, sizeof fileInfo);

        if (SHGetFileInfoW(nativeName.c_str(), attribute, &fileInfo
                            , sizeof fileInfo, SHGFI_ICON | SHGFI_USEFILEATTRIBUTES) == 0)
        {
            qWarning("SHGetFileInfoW failed %d, '%s'", GetLastError(), qUtf8Printable(path));
            return {};
        }
    }

    const QImage image = QImage::fromHICON(fileInfo.hIcon);
    DestroyIcon(fileInfo.hIcon);

    return image;
}


IconProvider::IconProvider(const QString &id)
    // don't force asynchoronous image loading this breaks images for some nested archive
    : QQuickImageProvider(QQuickImageProvider::Image/*, QQuickImageProvider::ForceAsynchronousImageLoading*/)
    , m_id {id}
{
}

QString IconProvider::url(Directory *dir, int child)
{
    QUrl url;
    url.setScheme("image");
    url.setPath(m_id);

    QUrlQuery query;
    query.addQueryItem("path", dir->filePath(child));
    query.addQueryItem("isdir", dir->isDir(child) ? "1" : "0");
    url.setQuery(query);

    auto s = url.url();
    s.replace("image:", "image://");
    return s;
}

QImage IconProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    QUrlQuery query(id);

    const QString path = query.queryItemValue("path");
    const bool isdir = query.queryItemValue("isdir") == "1";

    auto image = fileImage(path, isdir);
    if (size)
        *size = image.size();

    if (requestedSize.isValid())
        image = image.scaled(requestedSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    return image;
}
