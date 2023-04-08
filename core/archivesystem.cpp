#include "archivesystem.hpp"

#include <QDir>
#include <QVector>
#include <QUrl>

#include <archive.h>
#include <archive_entry.h>

namespace
{

class ArchiveDir;

class ArchiveNode : public Directory
{
public:
    ArchiveDir *parent_;
    QString name_;
    QString path_;
    qint64 size_;

    ArchiveNode(ArchiveDir *parent, QString name, QString path, qint64 size)
        : parent_ {parent}
        , name_ {name}
        , path_ {path}
        , size_ {size}
    {
    }

    virtual bool isDir() = 0;

    ArchiveDir *parent() { return parent_; }

    QString name() { return name_; }
    QString path() { return path_; }
    qint64 size() { return size_; }
};

class ArchiveFile : public ArchiveNode
{
public:
    using ArchiveNode::ArchiveNode;

    bool isDir() override { return false; }

    int fileCount() override { return 0; }
    QString fileName(int i) override { return {}; }
    QString filePath(int i) override { return {}; }
    qint64 fileSize(int i) override { return 0; }
};


class ArchiveDir : public ArchiveNode
{
public:
    QVector<ArchiveNode *> children;

    using ArchiveNode::ArchiveNode;

    ~ArchiveDir()
    {
        qDeleteAll(children);
    }

    bool isDir() override { return true; }

    int fileCount() override { return children.size(); }
    QString fileName(int i) override { return children[i]->name(); }
    QString filePath(int i) override { return children[i]->path(); }
    qint64 fileSize(int i) override { return children[i]->size(); }
};


QString pathName(const QString &filePath)
{
    QDir d(filePath);
    return d.dirName();
}

std::unique_ptr<Directory> buildTree(const QString &filePath)
{
    archive *a = archive_read_new();
    if (!a)
    {
        qWarning("failed archive_read_new");
        return {};
    }

    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);

    const int r = archive_read_open_filename_w(a, filePath.toStdWString().c_str(), 10240);
    if (r != ARCHIVE_OK)
    {
        qWarning("failed to archive read open filename %d", r);
        return {};
    }

    archive_entry *entry {};
    ArchiveDir *root = new ArchiveDir(nullptr, pathName(filePath), filePath, 0);
    QString rootpath = filePath + "#";

    QHash<ArchiveDir *, QHash<QString, ArchiveDir *>> dirMap;

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
        const QString path = archive_entry_pathname(entry);
        const auto size = archive_entry_size(entry);

        auto dirparts = path.split('/');
        const auto name = dirparts.takeLast();

        ArchiveDir *current = root;
        current->size_ += size;

        QString nodepath = rootpath;

        for (const auto &dirpart : dirparts)
        {
            nodepath += QString("/") + dirpart;

            ArchiveDir *& next = dirMap[current][dirpart];
            if (!next)
            {
                next = new ArchiveDir(current, dirpart, nodepath, 0);
                current->children.push_back(next);
            }

            next->size_ += size; // update directory sizes
            current = next;
        }

        if (!name.isEmpty())
        {
            const auto size = archive_entry_size(entry);
            auto file = new ArchiveFile(current, name, nodepath, size);

            current->children.push_back(file);
        }

        archive_read_data_skip(a);
    }


    return std::unique_ptr<Directory>(root);
}


class SharedDirectory : public Directory
{
public:
    std::shared_ptr<Directory> r;
    Directory *d;

    SharedDirectory(std::shared_ptr<Directory> r, Directory *d) : r {r}, d {d} {}

#define SHAREDDIRECTORY_WRAP(type, name) type name () override { return d->name (); }

    SHAREDDIRECTORY_WRAP(QString, path)
    SHAREDDIRECTORY_WRAP(QString, name)
    SHAREDDIRECTORY_WRAP(qint64, size)
    SHAREDDIRECTORY_WRAP(int, fileCount)

#undef SHAREDDIRECTORY_WRAP
#define SHAREDDIRECTORY_WRAP(type, name) type name (int i) override { return d->name (i); }

    SHAREDDIRECTORY_WRAP(QString, fileName)
    SHAREDDIRECTORY_WRAP(QString, filePath)
    SHAREDDIRECTORY_WRAP(qint64, fileSize)

#undef SHAREDDIRECTORY_WRAP
};

} // namespace



ArchiveSystem::ArchiveSystem()
{

}

std::unique_ptr<Directory> ArchiveSystem::open(const QUrl &url)
{
    auto r = buildTree(url.toLocalFile());
    auto *d = r.get();

    // returned a ref-counted instance so that we can reuse this in open(Directory *) call
    return std::make_unique<SharedDirectory>(std::move(r), d);
}

std::unique_ptr<Directory> ArchiveSystem::open(Directory *dir, int child)
{
    auto rootwrapper = dynamic_cast<SharedDirectory *> (dir);
    if (!rootwrapper) return nullptr;

    auto root = dynamic_cast<ArchiveDir *>( rootwrapper->d );
    if (!root) return nullptr;

    auto next = dynamic_cast<ArchiveDir *>( root->children.at(child) );
    if (!next) return nullptr;

    return std::make_unique<SharedDirectory>(rootwrapper->r, next);
}
