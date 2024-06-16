
#include "viewcontroller.hpp"

#include "../core/directorysystemmodel.hpp"
#include "../core/directorysortmodel.hpp"
#include "../core/hybriddirsystem.hpp"
#include "../core/filehistorydb.hpp"

#include <QtConcurrent/QtConcurrent>
#include <QMimeData>

static const QString ICON_PROVIDER_ID = "fileicon";

ViewController::ViewController(QObject *parent)
    : QObject {parent}
    , m_dirModel {std::make_unique<DirectorySystemModel>()}
    , m_sortModel { std::make_unique<DirectorySortModel>() }
    , m_system {std::make_unique<HybridDirSystem>()}
{
    m_dirModel->setIconProvider([this](Directory *dir, int child) -> QString
    {
        return iconID(dir, child);
    });

    m_sortModel->setSortRole(DirectorySystemModel::DataRole);
    m_sortModel->sort(DirectorySystemModel::NameColumn, Qt::AscendingOrder);
    m_sortModel->setSourceModel(m_dirModel.get());

    connect(&m_urlWatcher, &QFutureWatcherBase::finished, this, &ViewController::updateModel);
    connect(&m_previewWatcher, &QFutureWatcherBase::finished, this, &ViewController::updatePreview);
}

ViewController::~ViewController()
{
}

QAbstractItemModel *ViewController::model()
{
    return m_sortModel.get();
}

QString ViewController::url() const
{
    return m_dirModel->directory() ? m_dirModel->directory()->url().toString() : QString {};
}

QString ViewController::path() const
{
    return m_dirModel->directory() ? m_dirModel->directory()->path() : QString {};
}

void ViewController::refresh()
{
    if (m_dirModel->directory())
        openUrl(m_dirModel->directory()->url());
}

PreviewData ViewController::invalidPreviewData()
{
    return PreviewData();
}

void ViewController::openUrl(const QUrl &url)
{
    const auto open = [](
            std::shared_ptr<DirectorySystem> system,
            const QUrl &url) -> std::shared_ptr<Directory>
    {
        return system->open(url);
    };

    m_urlWatcher.setFuture(QtConcurrent::run(&m_pool, open, m_system, url));
}

void ViewController::openPath(const QString &path)
{
    const auto open = [](
            std::shared_ptr<DirectorySystem> system,
            const QString &path) -> std::shared_ptr<Directory>
    {
        return system->open(path);
    };

    m_urlWatcher.setFuture(QtConcurrent::run(&m_pool, open, m_system, path));

}

void ViewController::openRow(const int row)
{
    const auto directoryRow = sourceRow(row);
    if (directoryRow == -1)
    {
        qDebug("invalid openRow call with row - %d", row);
        return;
    }

    const auto open = [](
            std::shared_ptr<DirectorySystem> system,
            std::shared_ptr<Directory> dir,
            int child) -> std::shared_ptr<Directory>
    {
        return system->open(dir.get(), child);
    };

    if (auto parent = m_dirModel->directory())
    {
        m_urlWatcher.setFuture(QtConcurrent::run(&m_pool, open, m_system, parent, directoryRow));
    }
}

void ViewController::setPreview(int row)
{
    const auto getPreviewData = [db = m_historyDB](
            std::shared_ptr<DirectorySystem> system,
            std::shared_ptr<Directory> dir,
            int child) -> PreviewData
    {
        if (dir->isDir(child))
            return {nullptr, PreviewData::Unknown, 0};

        // some directory system may have custom urls, so you can't directly use fileUrl here
        const QString path = dir->filePath(child);
        const auto mime = QMimeDatabase().mimeTypeForUrl(QUrl::fromLocalFile(path)).name();
        auto progressFuture = db->progress(path);

        PreviewData::FileType filetype = PreviewData::Unknown;
        const auto types =
        {
            std::pair {PreviewData::ImageFile, "image"},
            {PreviewData::AudioFile, "audio"},
            {PreviewData::VideoFile, "video"}
        };

        for (const auto type : types)
        {
            if (mime.startsWith(type.second))
            {
                filetype = type.first;
                break;
            }
        }

        if (filetype == PreviewData::Unknown)
        {
            if (path.endsWith(".ts"))
                filetype = PreviewData::VideoFile;
        }

        auto io = system->iosource(dir.get(), child);
        if (!io)
        {
            qDebug("failed to get iosource '%s'", qUtf8Printable(dir->fileUrl(child).toString()));
            return {nullptr, PreviewData::Unknown, 1};
        }

        progressFuture.waitForFinished();
        const double progress = progressFuture.isValid() ? progressFuture.result() : 0;

        return PreviewData(std::move(io), filetype, progress);
    };


    const auto directoryRow = sourceRow(row);
    if (directoryRow == -1)
    {
        qDebug("invalid openRow call with row - %d", row);
        return;
    }

    if (auto parent = m_dirModel->directory())
    {
        m_previewWatcher.setFuture(QtConcurrent::run(&m_pool, getPreviewData, m_system, parent, directoryRow));
    }
    else
    {
        emit showPreview({});
    }
}

int ViewController::sourceRow(const int row)
{
    const auto index = m_sortModel->mapToSource(m_sortModel->index(row, 0));
    if (!index.isValid())
    {
        return -1;
    }

    return index.row();
}

QString ViewController::iconID(Directory *dir, int child)
{
    if (!m_iconProvider)
    {
        auto engine = qmlEngine(this);
        if (!engine)
            return {};

        for (int i = 0; i < 10 && !m_iconProvider; ++i) {
            const auto id = ICON_PROVIDER_ID + QString::number(i);
            if (engine->imageProvider(id))
                continue;

            m_iconProvider = new IconProvider(id);
            engine->addImageProvider(id, m_iconProvider);
        }
    }

    return m_iconProvider ? m_iconProvider->url(dir, child) : QString {};
}

void ViewController::updateModel()
{
    auto s = dynamic_cast<decltype (m_urlWatcher) *>(sender());
    if (s && s->result())
    {
        m_url = s->result()->url();
        m_dirModel->setDirectory(s->result());

        emit urlChanged();
    }
}

void ViewController::updatePreview()
{
    auto s = dynamic_cast<decltype (m_previewWatcher) *>(sender());
    if (s)
    {
        emit showPreview(s->result());
    }
}


FileBrowser *ViewController::fileBrowser() const
{
    return m_fileBrowser;
}

void ViewController::setFileBrowser(FileBrowser *newFileBrowser)
{
    if (m_fileBrowser == newFileBrowser)
        return;

    m_fileBrowser = newFileBrowser;
    emit fileBrowserChanged();

    m_historyDB.reset( new FileHistoryDB(m_fileBrowser->fileHistoryDBPath()) );
    m_dirModel->setFileHistoryDB(m_historyDB);
}
