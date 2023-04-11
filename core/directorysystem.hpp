#ifndef DIRECTORYSYSTEM_HPP
#define DIRECTORYSYSTEM_HPP

#include <QString>
#include <memory>
#include <QUrl>


// TODO: - better back system
// and fileUrl
class Directory
{
public:
    virtual ~Directory() = default;

    virtual QString path() = 0;
    virtual QString name() = 0;
    virtual QUrl url() = 0;

    virtual qint64 size() { return -1; } // it's optional

    virtual int fileCount() = 0;
    virtual QString fileName(int i) = 0;
    virtual QString filePath(int i) = 0;
    virtual QUrl fileUrl(int i) = 0;
    virtual qint64 fileSize(int i) = 0;
    virtual bool isDir(int i) = 0;
};


class DirectorySystem
{
public:
    virtual ~DirectorySystem() = default;

    virtual std::unique_ptr<Directory> open(const QUrl &url) = 0;
    virtual std::unique_ptr<Directory> open(Directory *dir, int child) = 0;
//    virtual std::unique_ptr<Directory> parent(Directory *dir) = 0;
};


#endif // DIRECTORYSYSTEM_HPP
