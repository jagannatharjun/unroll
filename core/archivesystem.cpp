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


/**
 * @brief The ArchiveUrl class
 *
 * the schema for urls for ArchiveSystem is as follows
 * archivesystem://{archivepath}&child0=path&child1=path...
 *
 * individual nth child key represents path inside that nth archive level
 * if that path points to file inside the archive, the ArchiveSystem may try
 * to open that with and return a corresponding Directory
 *
 *
 * TODO extend tests for ArchiveUrl
 */
class ArchiveUrl
{

    // TODO extend tests for ArchiveUrl
public:

    static bool isarchiveurl(const QUrl &url)
    {
        return url.scheme() == URL_SCHEME;
    }

    static QString childKey(const int index)
    {
        return QString("%1%2").arg(CHILD_KEY, QString::number(index));
    }

    static int countChildren(const QUrl &url)
    {
        int count = 0;
        QUrlQuery query(url);

        while (query.hasQueryItem(childKey(count)))
            count++;

        return count;
    }


    ArchiveUrl(const QString &filePath)
    {
        m_url.setScheme(URL_SCHEME);
        m_url.setPath(filePath);
        m_childCount = 0;
    }

    ArchiveUrl(const QUrl &url)
        : ArchiveUrl {url, countChildren(url)}
    {
    }

    int childrenCount() const
    {
        assert(children().size() == m_childCount);
        return m_childCount;
    }

    QString filepath() const
    {
        QString childPath;
        QUrlQuery query(m_url);

        for (int i = 0; i < m_childCount; ++i)
        {
            auto p = query.queryItemValue(childKey(i));
            if (childPath.endsWith("/"))
                childPath = childPath.sliced(0, childPath.size() - 1);
            if (!p.startsWith("/"))
                p = "/" + p;

            childPath += p;
        }

        return m_url.path(QUrl::PrettyDecoded) + childPath;
    }

    QString firstChild() const
    {
        return QUrlQuery(m_url).queryItemValue(childKey(0));
    }

    QString lastChild() const
    {
        return QUrlQuery(m_url).queryItemValue(childKey(m_childCount - 1));
    }

    QString archivePath() const
    {
        return m_url.path(QUrl::PrettyDecoded);
    }

    QUrl url() const { return m_url; }

    ArchiveUrl withChild(const QString &child) const
    {
        QUrlQuery query(m_url);
        query.addQueryItem(childKey(m_childCount), child);

        QUrl n = m_url;
        n.setQuery(query);
        return ArchiveUrl {n, m_childCount + 1};
    }

    QVector<QString> children() const
    {
        QVector<QString>  r;

        QUrlQuery q(m_url);
        for (int i = 0; i < m_childCount; ++i)
            r.push_back(q.queryItemValue(childKey(i)));

        return r;
    }

private:
    ArchiveUrl(QUrl url, int childCount) : m_url {url}, m_childCount {childCount}
    {
        assert(isarchiveurl(url));
    }

    QUrl m_url;
    int m_childCount = 0;
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
    QUrl url() { return url_.url(); }
    qint64 size() { return size_; }
};


class ArchiveFile : public ArchiveNode
{
public:
    using ArchiveNode::ArchiveNode;

    QDateTime lastAccessTime;
    QDateTime creationTime;
    QDateTime modifiedTime;

    bool isDir(int i) override { return false; }

    int fileCount() override { return 0; }
    QString fileName(int i) override { return {}; }
    QString filePath(int i) override { return {}; }
    qint64 fileSize(int i) override { return 0; }
    QUrl fileUrl(int i) override { return {}; }

    QDateTime fileLastAccessTime(int i) override { return {}; }
    QDateTime fileCreationTime(int i) override { return {}; }
    QDateTime fileModifiedTime(int i) override { return {}; }
};


class ArchiveDir : public ArchiveNode
{
public:
    QVector<ArchiveNode *> children;
    std::shared_ptr<QTemporaryFile> source;

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

    QDateTime fileLastAccessTime(int i) override
    {
        if (auto f = dynamic_cast<ArchiveFile *>(children[i]))
            return f->lastAccessTime;

        return {};
    }

    QDateTime fileCreationTime(int i) override {
        if (auto f = dynamic_cast<ArchiveFile *>(children[i]))
            return f->creationTime;

        return {};
    }

    QDateTime fileModifiedTime(int i) override
    {
        if (auto f = dynamic_cast<ArchiveFile *>(children[i]))
            return f->modifiedTime;

        return {};
    }
};

/*
 * Used to keep a reference true reference to root when returing child directory
 * the root owns all children (including nth-grand children)
*/
class SharedDirectory : public Directory
{
public:
    // reference to root
    std::shared_ptr<ArchiveDir> r;

    // root's subdirectory we wrap
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

    SHAREDDIRECTORY_WRAP(QDateTime, fileLastAccessTime)
    SHAREDDIRECTORY_WRAP(QDateTime, fileCreationTime)
    SHAREDDIRECTORY_WRAP(QDateTime, fileModifiedTime)

#undef SHAREDDIRECTORY_WRAP
};


class ArchiveTempIOSource : public IOSource
{
public:
    std::shared_ptr<QTemporaryFile> file;

    QString readPath() override
    {
        return file->fileName();
    }

};


QString pathName(const QString &filePath)
{
    QDir d(filePath);
    return d.dirName();
}


bool iterateArchiveEntries(const QString &archivepath, std::function<bool(archive * archive, archive_entry *entry)> functor)
{
    std::unique_ptr<archive, decltype(&archive_read_free)> a (archive_read_new(), &archive_read_free);
    if (!a)
    {
        qWarning("failed archive_read_new, '%s'", qUtf8Printable(archivepath));
        return false;
    }

    archive_read_support_filter_all(a.get());
    archive_read_support_format_all(a.get());

    const int r = archive_read_open_filename_w(a.get(), archivepath.toStdWString().c_str(), 10240);
    if (r != ARCHIVE_OK)
    {
        qWarning("failed to archive read open filename %d, '%s'", r, qUtf8Printable(archivepath));
        return false;
    }

    archive_entry *entry {};
    while (archive_read_next_header(a.get(), &entry) == ARCHIVE_OK)
    {
        if (functor(a.get(), entry))
            return true;

        archive_read_data_skip(a.get());
    }

    return true;
}



struct BuildTreeResult
{
    std::unique_ptr<ArchiveDir> root;
    ArchiveNode *child {}; // owned by root
};


BuildTreeResult buildTree(const QString &filePath, const QString &childpath, const ArchiveUrl &baseUrl)
{
    const auto rootName = baseUrl.childrenCount() > 0
            ? pathName(baseUrl.children().back())
            : pathName(baseUrl.archivePath());

    std::unique_ptr<ArchiveDir> root {new ArchiveDir(nullptr, rootName, baseUrl, 0)};
    ArchiveNode *child {};

    QHash<ArchiveDir *, QHash<QString, ArchiveDir *>> dirMap;

    const auto insertFileNode = [&](archive *, archive_entry *entry)
    {
        const QString path = archive_entry_pathname(entry);
        const auto size = archive_entry_size(entry);

        auto dirparts = path.split('/');
        const auto name = dirparts.takeLast();

        ArchiveDir *current = root.get();
        current->size_ += size;

        QString nodepath; // current nodepath per traversal

        for (const auto &dirpart : dirparts)
        {
            nodepath += QString("/") + dirpart;

            ArchiveDir *& next = dirMap[current][dirpart];
            if (!next)
            {
                next = new ArchiveDir(current, dirpart, baseUrl.withChild(nodepath), 0);
                current->children.push_back(next);

                if (!child && (nodepath == childpath))
                {
                    child = next;
                }
            }

            next->size_ += size; // update directory sizes
            current = next;
        }

        if (!name.isEmpty()) // this is a file
        {
            nodepath += QString("/") + name;

            const auto size = archive_entry_size(entry);
            auto file = new ArchiveFile(current, name, baseUrl.withChild(nodepath), size);

            if (archive_entry_birthtime_is_set(entry))
                file->creationTime = QDateTime::fromMSecsSinceEpoch(archive_entry_birthtime(entry));

            if (archive_entry_ctime_is_set(entry))
                file->modifiedTime = QDateTime::fromMSecsSinceEpoch(archive_entry_ctime(entry));

            if (archive_entry_atime_is_set(entry))
                file->lastAccessTime = QDateTime::fromMSecsSinceEpoch(archive_entry_atime(entry));

            current->children.push_back(file);
            if (!child && (nodepath == childpath))
            {
                child = file;
            }
        }

        return false; // continue
    };

    if (!iterateArchiveEntries(filePath, insertFileNode))
        return {};

    return BuildTreeResult {std::move(root), child};
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

    iterateArchiveEntries(filePath, extract);
}



std::shared_ptr<QTemporaryFile> extractFile(const QString &filePath, const QString &childPath)
{
    const QDir tempdir(QDir::tempPath());
    const QString templateName = "XXXXXXXXXX." + QFileInfo(childPath).completeSuffix();

    auto r = std::make_shared<QTemporaryFile>();
    r->setFileTemplate(tempdir.absoluteFilePath(templateName));
    if (!r->open())
    {
        // TODO: error handling
        qWarning("failed to open temporary file");
        return nullptr;
    }

    extractFile(filePath, childPath, r.get());

    // this is necessary to flush the content, otherwise reader may get invalid content
    r->close();

    return r;
}


SharedDirectory *unwrap(Directory *dir)
{
    return dynamic_cast<SharedDirectory *> (dir);
}


std::unique_ptr<SharedDirectory> wrap(std::shared_ptr<ArchiveDir> root, ArchiveDir *child)
{
    return std::make_unique<SharedDirectory>(std::move(root), child);
}


QString sourcePath(ArchiveDir *d, const ArchiveUrl &url)
{
    if (!d->source && url.childrenCount() > 1)
    {
        qWarning("recursive node doesn't have source");
        return {};
    }

    if (d->source)
        return d->source->fileName();

    return url.archivePath();
}


std::unique_ptr<SharedDirectory> openFile(const QString &fileName, const QString &childName, const ArchiveUrl &baseUrl)
{
    auto tree = buildTree(fileName, childName, baseUrl);
    if (!tree.root || (!tree.child != childName.isEmpty()))
        return nullptr;

    if (!tree.child)
    {
        auto d = tree.root.get();
        return wrap(std::move(tree.root), d);
    }

    auto childDir = dynamic_cast<ArchiveDir *>(tree.child);
    if (childDir)
    {
        return wrap(std::move(tree.root), childDir);
    }

    auto tmp = extractFile(fileName, childName);
    if (!tmp)
        return {};

    const auto newUrl = baseUrl.withChild(childName);
    auto newroot = buildTree(tmp->fileName(), {}, newUrl);
    if (!newroot.root) return nullptr;
    assert(!newroot.child);

    newroot.root->source = tmp;
    auto d = newroot.root.get();

    return wrap(std::move(newroot.root), d);
}


} // namespace



ArchiveSystem::ArchiveSystem()
{
}

std::unique_ptr<Directory> ArchiveSystem::open(const QUrl &url)
{
    if (url.isLocalFile())
    {
        return openFile(url.toLocalFile(), {}, ArchiveUrl(url.toLocalFile()));
    }
    else if (ArchiveUrl::isarchiveurl(url))
    {
        const ArchiveUrl fullUrl(url);
        if (fullUrl.childrenCount() == 0)
            return openFile(fullUrl.archivePath(), {}, ArchiveUrl(fullUrl.archivePath()));


        auto currentPath = fullUrl.archivePath();
        ArchiveUrl currentUrl(fullUrl.archivePath());
        std::shared_ptr<QTemporaryFile> temp;

        std::unique_ptr<SharedDirectory> current;
        for (const auto &child : fullUrl.children())
        {
            assert(!currentPath.isEmpty());

            current = openFile(currentPath, child, currentUrl);
            if (!current)
                return nullptr;

            // if the openFile() openned a recursive archive then returrned value
            // will already have a source otherwise if a new directory is openned
            // we need to transfer the source, so that it can be retained
            if (!current->r->source)
                current->r->source = temp;

            temp = current->r->source;
            currentPath = temp ? temp->fileName() : QString {};
            currentUrl = currentUrl.withChild(child);
        }

        return std::move(current);
    }

    return {};
}


// true returned value is SharedDirectory, it is useful to keep a reference to child directory
std::unique_ptr<Directory> ArchiveSystem::open(Directory *dir, int child)
{
    if (!dir || child < 0 || child >= dir->fileCount())
        return nullptr;

    auto wrapper = unwrap(dir);
    if (!wrapper)
        return nullptr;

    auto dirnode = dynamic_cast<ArchiveDir * >(wrapper->d->children[child]);
    if (dirnode)
        return wrap(wrapper->r, dirnode);

    // child is a file, check for recursive archive
    const ArchiveUrl url {dir->fileUrl(child)};
    const auto p = sourcePath(wrapper->r.get(), url);
    if (p.isEmpty())
        return {};

    return openFile(p, url.lastChild(), url);
}

std::unique_ptr<IOSource> ArchiveSystem::iosource(Directory *dir, int child)
{
    auto result = std::make_unique<ArchiveTempIOSource>();

    auto wrapper = unwrap(dir);
    if (!wrapper)
        return {}; // invalid input

    const ArchiveUrl url{dir->fileUrl(child)};
    const auto p = sourcePath(wrapper->r.get(), url);
    if (p.isEmpty())
        return {};

    result->file = extractFile(p, url.lastChild());
    if (!result->file)
        return {};

    return result;
}
