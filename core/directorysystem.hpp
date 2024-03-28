#ifndef DIRECTORYSYSTEM_HPP
#define DIRECTORYSYSTEM_HPP

#include <QString>
#include <QDateTime>
#include <memory>
#include <QUrl>


// ALL functions must be thread-safe
// ALL functions of interface can be called from any number of threads

// Directory is only observer, no operation can be applied
class Directory
{
public:
    virtual ~Directory() = default;

    virtual QString path() = 0;
    virtual QString name() = 0;
    virtual QUrl url() = 0;

    // this is optional
    virtual qint64 size() { return -1; }

    // children property
    virtual int fileCount() = 0;
    virtual QString fileName(int i) = 0;
    virtual QString filePath(int i) = 0;
    virtual QUrl fileUrl(int i) = 0;
    virtual qint64 fileSize(int i) = 0;
    virtual bool isDir(int i) = 0;

    virtual QDateTime fileLastAccessTime(int i) = 0;
    virtual QDateTime fileCreationTime(int i) = 0;
    virtual QDateTime fileModifiedTime(int i) = 0;
};



class IOSource
{
public:
    virtual ~IOSource() = default;

    // All return values are only valid till the assosicated IOSource is alive
    virtual QString readPath() = 0;
};



class DirectorySystem
{
public:
    virtual ~DirectorySystem() = default;

    virtual std::unique_ptr<Directory> open(const QUrl &url) = 0;
    virtual std::unique_ptr<Directory> open(const QString &path) = 0;
    virtual std::unique_ptr<Directory> open(Directory *dir, int child) = 0;

    virtual std::unique_ptr<Directory> dirParent(Directory *dir) = 0;

    virtual std::unique_ptr<IOSource> iosource(Directory *dir, int child) = 0;
};


#endif // DIRECTORYSYSTEM_HPP
