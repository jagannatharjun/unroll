#include "filesystem.hpp"

#include <QUrl>
#include <QDir>

namespace
{

const QString LEAN_URL_SCEHEME = u"lean_dir"_qs;

struct EntryInfo
{
    QString name;
    QString path;
    qint64 size;
    bool isdir;
};

class RegularDirectory : public Directory
{
public:
    QString directoryPath;
    QString directoryName;
    QVector<EntryInfo> entries;

    QString name() override { return directoryName; }
    QString path() override { return directoryPath; }

    int fileCount() override { return entries.size(); }
    QString fileName(int i) override { return entries[i].name; }
    QString filePath(int i) override { return entries[i].path; }
    QUrl fileUrl(int i) override { return QUrl::fromLocalFile(entries[i].path); }
    qint64 fileSize(int i) override { return entries[i].size; }
    bool isDir(int i) override { return entries[i].isdir; }


    QDateTime fileLastAccessTime(int i) override
    {
        return QFileInfo(entries[i].path).lastRead();
    }

    QDateTime fileCreationTime(int i) override
    {
        return QFileInfo(entries[i].path).birthTime();
    }

    QDateTime fileModifiedTime(int i) override
    {
        return QFileInfo(entries[i].path).lastModified();
    }
};

template<bool LeanMode>
class AdaptiveDirectory : public RegularDirectory
{
public:
    QUrl url() override
    {
        if constexpr (LeanMode)
        {
            auto u = QUrl::fromLocalFile(directoryPath);
            u.setScheme(LEAN_URL_SCEHEME);
            return u;
        }

        return QUrl::fromLocalFile(directoryPath);
    }
};

class RegularIOSource : public IOSource
{
public:
    QString filePath;

    RegularIOSource(const QString &filePath) : filePath {filePath} {}

    QString readPath() override
    {
        return filePath;
    }
};

void addFiles(const QString &path, bool flatMode, RegularDirectory *dir)
{
    QDir d(path);
    QFileInfoList list = d.entryInfoList();

    for (int i = 0; i < list.size(); ++i)
    {
        QFileInfo fileInfo = list.at(i);
        if ((fileInfo.fileName() == ".")
            || (fileInfo.fileName() == "..")) continue;

        if (flatMode && fileInfo.isDir())
        {
            addFiles(fileInfo.absoluteFilePath(), flatMode, dir);
            continue;
        }

        EntryInfo f
        {
           fileInfo.fileName()
            , fileInfo.absoluteFilePath()
            , fileInfo.size()
            , fileInfo.isDir()
        };

        dir->entries.push_back(std::move(f));
    }
}

std::unique_ptr<Directory> openDir(const QString &path, const bool flatMode)
{
    QDir d(path);
    if (!d.exists())
        return {};

    QFileInfoList list = d.entryInfoList();

    std::unique_ptr<RegularDirectory> r;
    if  (flatMode)
        r = std::make_unique<AdaptiveDirectory<true>>();
    else
        r = std::make_unique<AdaptiveDirectory<false>>();

    r->directoryPath = d.absolutePath();
    r->directoryName = d.dirName();

    addFiles(path, flatMode, r.get());

    return std::move(r);
}
}

std::unique_ptr<Directory> FileSystem::open(const QString &path)
{
    return openDir(path, m_leanMode);
}

std::unique_ptr<Directory> FileSystem::open(const QUrl &url)
{
    if (url.scheme() == LEAN_URL_SCEHEME)
    {
        auto u = url;
        u.setScheme("file");
        return openDir(u.toLocalFile(), true);
    }

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

bool FileSystem::leanMode() const
{
    return m_leanMode;
}

void FileSystem::setLeanMode(bool newLeanMode)
{
    m_leanMode = newLeanMode;
}
