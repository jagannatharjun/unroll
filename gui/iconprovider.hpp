#ifndef ICONPROVIDER_HPP
#define ICONPROVIDER_HPP

#include <QQuickImageProvider>

class Directory;

class IconProvider : public QQuickImageProvider
{
public:
    IconProvider(const QString &id);

    QString url(Directory *dir, int child);

    // QQuickImageProvider interface
public:
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);

private:
    const QString m_id;
    QHash<QString, QImage> m_images;
};

#endif // ICONPROVIDER_HPP
