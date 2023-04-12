#include "archivesystem.hpp"

#include <QDir>
#include <QVector>
#include <QUrl>
#include <QUrlQuery>

#include <archive.h>
#include <archive_entry.h>

namespace
{

class ArchiveDir;

static const QString CHILD_KEY = "child";
static const QString URL_SCHEME = "archivesystem";

// TODO handle archive urls view URL's query parameter, not pound
// extend tests
struct ArchiveUrl
{

    static ArchiveUrl makeurl(const QString &path, const QString &child)
    {
        QUrl url;
        url.setScheme(URL_SCHEME);
        url.setPath(path);
        url.setQuery(QUrlQuery({{CHILD_KEY, child}}));
        return ArchiveUrl {url};
    }

    QUrl url;

    QString filepath() const
    {
        return url.path(QUrl::PrettyDecoded) + child();
    }

    QString child() const
    {
        return QUrlQuery(url).queryItemValue(CHILD_KEY);
    }

    QString archivepath() const
    {
        return url.path(QUrl::PrettyDecoded);
    }
};

class ArchiveNode : public Directory
{
public:
    ArchiveDir *parent_;
    QString name_;
    ArchiveUrl url_;
    qint64 size_;

    ArchiveNode(ArchiveDir *parent, QString name, ArchiveUrl url, qint64 size)
        : parent_ {parent}
        , name_ {name}
        , url_ {url}
        , size_ {size}
    {
    }

    ArchiveDir *parent() { return parent_; }

    QString name() { return name_; }
    QString path() { return url_.filepath(); }
    QUrl url() { return url_.url; }
    qint64 size() { return size_; }
};

class ArchiveFile : public ArchiveNode
{
public:
    using ArchiveNode::ArchiveNode;

    bool isDir(int i) override { return false; }

    int fileCount() override { return 0; }
    QString fileName(int i) override { return {}; }
    QString filePath(int i) override { return {}; }
    qint64 fileSize(int i) override { return 0; }
    QUrl fileUrl(int i) override { return {}; }
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

    bool isDir(int i) override { return dynamic_cast<ArchiveDir *>(children[i]) != nullptr; }

    int fileCount() override { return children.size(); }
    QString fileName(int i) override { return children[i]->name(); }
    QString filePath(int i) override { return children[i]->path(); }
    QUrl fileUrl(int i) override { return children[i]->url(); }
    qint64 fileSize(int i) override { return children[i]->size(); }
};


QString pathName(const QString &filePath)
{
    QDir d(filePath);
    return d.dirName();
}

struct BuildTreeResult
{
    std::unique_ptr<ArchiveDir> root;
    ArchiveNode *child {}; // owned by root
};

BuildTreeResult buildTree(const QString &filePath, const QString &childpath)
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
    ArchiveDir *root = new ArchiveDir(nullptr, pathName(filePath), ArchiveUrl::makeurl(filePath, {}), 0);
    ArchiveNode *child {};

    QHash<ArchiveDir *, QHash<QString, ArchiveDir *>> dirMap;

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
        const QString path = archive_entry_pathname(entry);
        const auto size = archive_entry_size(entry);

        auto dirparts = path.split('/');
        const auto name = dirparts.takeLast();

        ArchiveDir *current = root;
        current->size_ += size;

        QString nodepath; // current nodepath per traversal

        for (const auto &dirpart : dirparts)
        {
            nodepath += QString("/") + dirpart;

            ArchiveDir *& next = dirMap[current][dirpart];
            if (!next)
            {
                next = new ArchiveDir(current, dirpart, ArchiveUrl::makeurl(filePath, nodepath), 0);
                current->children.push_back(next);
            }

            next->size_ += size; // update directory sizes
            current = next;
        }

        if (!child && (nodepath == childpath))
        {
            child = current;
        }

        if (!name.isEmpty())
        {
            nodepath += QString("/") + name;

            const auto size = archive_entry_size(entry);
            auto file = new ArchiveFile(current, name, ArchiveUrl::makeurl(filePath, nodepath), size);

            current->children.push_back(file);
            if (!child && (nodepath == childpath))
            {
                child = file;
            }
        }

        archive_read_data_skip(a);
    }

    return BuildTreeResult {std::unique_ptr<ArchiveDir>(root), child};
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
    SHAREDDIRECTORY_WRAP(QUrl, url)
    SHAREDDIRECTORY_WRAP(qint64, size)
    SHAREDDIRECTORY_WRAP(int, fileCount)

#undef SHAREDDIRECTORY_WRAP
#define SHAREDDIRECTORY_WRAP(type, name) type name (int i) override { return d->name (i); }

    SHAREDDIRECTORY_WRAP(QString, fileName)
    SHAREDDIRECTORY_WRAP(QString, filePath)
    SHAREDDIRECTORY_WRAP(QUrl, fileUrl)
    SHAREDDIRECTORY_WRAP(qint64, fileSize)
    SHAREDDIRECTORY_WRAP(bool, isDir)

#undef SHAREDDIRECTORY_WRAP
};

} // namespace



ArchiveSystem::ArchiveSystem()
{

}

std::unique_ptr<Directory> ArchiveSystem::open(const QUrl &url)
{
    std::unique_ptr<Directory> root;
    Directory *result {};

    if (url.isLocalFile())
    {
        root = buildTree(url.toLocalFile(), {}).root;
        result = root.get();
    }
    else if (url.scheme() == URL_SCHEME)
    {
        const ArchiveUrl archiveurl {url};
        auto tree = buildTree(archiveurl.archivepath(), archiveurl.child());
        root = std::move(tree.root);
        result = tree.child;
    }

    if (!result) return nullptr;

    assert(root);
    // returned a ref-counted instance so that we can reuse this in open(Directory *) call
    return std::make_unique<SharedDirectory>(std::move(root), result);
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
