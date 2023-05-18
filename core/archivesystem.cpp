#include "archivesystem.hpp"

#include <QDir>
#include <QVector>
#include <QUrl>
#include <QUrlQuery>
#include <QTemporaryFile>

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

    static bool isarchiveurl(const QUrl &url)
    {
        return url.scheme() == URL_SCHEME;
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

/*
 * Used to keep a reference true reference to root when returing child as parse result
*/
class SharedDirectory : public Directory
{
public:
    std::shared_ptr<ArchiveDir> r;
    ArchiveDir *d;

    SharedDirectory(std::shared_ptr<ArchiveDir> r, ArchiveDir *d) : r {r}, d {d} {}

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


class ArchiveTempIOSource : public IOSource
{
public:
    QTemporaryFile file;

    QString readPath() override
    {
        return file.fileName();
    }

};


QString pathName(const QString &filePath)
{
    QDir d(filePath);
    return d.dirName();
}


void iterate_archivefiles(const QString &archivepath, std::function<bool(archive * archive, archive_entry *entry)> functor)
{
    std::unique_ptr<archive, decltype(&archive_read_free)> a (archive_read_new(), &archive_read_free);
    if (!a)
    {
        qWarning("failed archive_read_new");
        return ;
    }

    archive_read_support_filter_all(a.get());
    archive_read_support_format_all(a.get());

    const int r = archive_read_open_filename_w(a.get(), archivepath.toStdWString().c_str(), 10240);
    if (r != ARCHIVE_OK)
    {
        qWarning("failed to archive read open filename %d", r);
        return ;
    }

    archive_entry *entry {};
    while (archive_read_next_header(a.get(), &entry) == ARCHIVE_OK)
    {
        if (functor(a.get(), entry))
            break;

        archive_read_data_skip(a.get());
    }
}



struct BuildTreeResult
{
    std::unique_ptr<ArchiveDir> root;
    ArchiveNode *child {}; // owned by root
};


BuildTreeResult buildTree(const QString &filePath, const QString &childpath)
{
    ArchiveDir *root = new ArchiveDir(nullptr, pathName(filePath), ArchiveUrl::makeurl(filePath, {}), 0);
    ArchiveNode *child {};

    QHash<ArchiveDir *, QHash<QString, ArchiveDir *>> dirMap;

    const auto insertFileNode = [&](archive *, archive_entry *entry)
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

        return false; // continue
    };

    iterate_archivefiles(filePath, insertFileNode);
    return BuildTreeResult {std::unique_ptr<ArchiveDir>(root), child};
}


void extractFile(const QString &filePath, const QString &childpath, QIODevice *output)
{
    const auto extract = [&](archive *a, archive_entry *entry)
    {
        // child always starts with '/'
        const QString path = QString('/') + archive_entry_pathname(entry);
        if (path != childpath)
            return false;

        const size_t bufsize = 1024;
        std::unique_ptr<char[]> buf(new char[bufsize]);
        la_ssize_t readsize = 0;
        while ((readsize = archive_read_data(a, buf.get(), bufsize)) > 0)
        {
            output->write(buf.get(), readsize);
        }

        return true; // break traversal
    };

    iterate_archivefiles(filePath, extract);
}


bool canopenarchive(const QString &filepath)
{
    std::unique_ptr<archive, decltype(&archive_read_free)> a (archive_read_new(), &archive_read_free);
    archive_read_support_format_all(a.get());

    int r = archive_read_open_filename(a.get(), filepath.toStdString().c_str(), 10240);

    return (r == ARCHIVE_OK);
}

struct GetChildDirResult
{
    SharedDirectory *dirwrapper {};
    ArchiveDir *child {};
};

GetChildDirResult getdirchild(Directory *dir, int child)
{
    auto dirwrapper = dynamic_cast<SharedDirectory *> (dir);
    if (!dirwrapper) return {};

    auto thisroot = dynamic_cast<ArchiveDir *>( dirwrapper->d );
    if (!thisroot) return {};

    auto next = dynamic_cast<ArchiveDir *>( thisroot->children.at(child) );
    return {dirwrapper, next};
}

} // namespace



ArchiveSystem::ArchiveSystem()
{

}

bool ArchiveSystem::canopen(const QUrl &url)
{
    if (url.isLocalFile())
    {
        return canopenarchive(url.toLocalFile());
    }

    if (ArchiveUrl::isarchiveurl(url))
    {
        // TODO handle child path? what if archive is password protected
        return canopenarchive(ArchiveUrl {url}.archivepath());
    }

    return false;
}

bool ArchiveSystem::canopen(Directory *dir, int child)
{
    return getdirchild(dir, child).child != nullptr;
}

std::unique_ptr<Directory> ArchiveSystem::open(const QUrl &url)
{
    std::unique_ptr<ArchiveDir> root;
    ArchiveDir *result {};

    if (url.isLocalFile())
    {
        root = buildTree(url.toLocalFile(), {}).root;
        result = root.get();
    }
    else if (ArchiveUrl::isarchiveurl(url))
    {
        const ArchiveUrl archiveurl {url};
        auto tree = buildTree(archiveurl.archivepath(), archiveurl.child());
        root = std::move(tree.root);

        result = dynamic_cast<ArchiveDir *>(tree.child);
    }

    if (!result) return nullptr;

    assert(root);
    // returned a ref-counted instance so that we can reuse this in open(Directory *) call
    return std::make_unique<SharedDirectory>(std::move(root), result);
}

std::unique_ptr<Directory> ArchiveSystem::open(Directory *dir, int child)
{
    auto r = getdirchild(dir, child);
    if (!r.child) return nullptr;

    return std::make_unique<SharedDirectory>(r.dirwrapper->r, r.child);
}

std::unique_ptr<IOSource> ArchiveSystem::iosource(Directory *dir, int child)
{
    auto result = std::make_unique<ArchiveTempIOSource>();

    const ArchiveUrl url{dir->fileUrl(child)};
    const QDir tempdir(QDir::tempPath());
    const QString templateName = "XXXXXXXXXXXXXXXXXXXX." + QFileInfo(url.child()).completeSuffix();

    result->file.setFileTemplate(tempdir.absoluteFilePath(templateName));
    if (!result->file.open())
    {
        // TODO: error handling
        qWarning("failed to open temporary file");
        return nullptr;
    }

    extractFile(url.archivepath(), url.child(), &result->file);

    // this is necessary to flush the content, otherwise reader may get invalid content
    result->file.close();

    return result;
}
