#include "filesystem.hpp"

#include <QUrl>
#include <QDir>

namespace
{

struct File
{
    QString name;
    QString path;
    qint64 size;
    bool isdir;
};

class RegularDirectory : public Directory
{
public:
    QString path_;
    QString name_;
    QVector<File> files;

    QString name() override { return name_; }
    QString path() override { return path_; }
    QUrl url() override { return QUrl::fromLocalFile(path_); }

    int fileCount() override { return files.size(); }
    QString fileName(int i) override { return files[i].name; }
    QString filePath(int i) override { return files[i].path; }
    QUrl fileUrl(int i) override { return QUrl::fromLocalFile(files[i].path); }
    qint64 fileSize(int i) override { return files[i].size; }
    bool isDir(int i) override { return files[i].isdir; }


    QDateTime fileLastAccessTime(int i) override
    {
        return QFileInfo(files[i].path).lastRead();
    }

    QDateTime fileCreationTime(int i) override
    {
        return QFileInfo(files[i].path).birthTime();

    }

    QDateTime fileModifiedTime(int i) override
    {
        return QFileInfo(files[i].path).lastModified();
    }
};

class RegularIOSource : public IOSource
{
public:
    QString filePath_;

    RegularIOSource(const QString &filePath) : filePath_ {filePath} {}

    QString readPath() override
    {
        return filePath_;
    }
};

}

std::unique_ptr<Directory> FileSystem::open(const QString &path)
{
    QDir d(path);
    if (!d.exists())
        return {};

    QFileInfoList list = d.entryInfoList();

    auto r = std::make_unique<RegularDirectory>();
    r->path_ = d.absolutePath();
    r->name_ = d.dirName();

    for (int i = 0; i < list.size(); ++i)
    {
        QFileInfo fileInfo = list.at(i);
        if (fileInfo.fileName() == "." || fileInfo.fileName() == "..") continue;

        r->files.push_back(File {fileInfo.fileName(), fileInfo.absoluteFilePath(), fileInfo.size(), fileInfo.isDir()});
    }

    return std::move(r);
}

std::unique_ptr<Directory> FileSystem::open(const QUrl &url)
{
    // check before otherwise toLocalFile returns empty path
    // and we search current directory
    if (!url.isLocalFile())
        return {};

    return open(url.toLocalFile());
}

std::unique_ptr<Directory> FileSystem::open(Directory *dir, int child)
{
    return open(dir->filePath(child));
}

std::unique_ptr<Directory> FileSystem::dirParent(Directory *dir)
{
    QDir d(dir->path());
    if (d.cdUp())
        return open(d.path());

    return nullptr;
}

std::unique_ptr<IOSource> FileSystem::iosource(Directory *dir, int child)
{
    return std::make_unique<RegularIOSource>(dir->filePath(child));
}

