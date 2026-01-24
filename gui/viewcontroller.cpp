
#include "viewcontroller.hpp"

#include "../core/directorysystemmodel.hpp"
#include "../core/directorysortmodel.hpp"
#include "../core/hybriddirsystem.hpp"
#include "../core/filehistorydb.hpp"
#include "directoryopener.hpp"

#include <QThreadPool>

#include <QtConcurrent/QtConcurrent>
#include <QMimeData>
#include <QDir>

static const QString ICON_PROVIDER_ID = "fileicon";


ViewController::ViewController(QObject *parent)
    : QObject {parent}
    , m_pool {std::make_unique<QThreadPool>()}
    , m_dirModel {std::make_unique<DirectorySystemModel>()}
    , m_sortModel { std::make_unique<DirectorySortModel>() }
    , m_system {std::make_unique<HybridDirSystem>()}
    , m_dirOpener {std::make_unique<DirectoryOpener>(m_pool, m_system)}
{
    m_selectionModel.setModel(model());

    m_dirModel->setIconProvider([this](Directory *dir, int child) -> QString
    {
        return iconID(dir, child);
    });

    m_sortModel->setSortRole(DirectorySystemModel::DataRole);
    m_sortModel->sort(DirectorySystemModel::NameColumn, Qt::AscendingOrder);
    m_sortModel->setSourceModel(m_dirModel.get());

    connect(m_dirOpener.get(), &DirectoryOpener::directoryChanged
            , this, &ViewController::updateModel);

    connect(&m_history, &HistoryStack::depthChanged
            , this, &ViewController::syncUrl);

    connect(this, &ViewController::loadingChanged
            , this, &ViewController::updateHistoryStack);

    connect(m_sortModel.get(), &DirectorySortModel::sortParametersChanged
            , this, &ViewController::updatePathHistory);

    connect(m_sortModel.get(), &DirectorySortModel::randomSortChanged
            , this, &ViewController::updatePathHistory);

    connect(m_sortModel.get(), &DirectorySortModel::onlyShowVideoFileChanged
            , this, &ViewController::updatePathHistory);


    connect(&m_selectionModel, &QItemSelectionModel::currentChanged
            , this, &ViewController::updatePathHistory);
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
    m_dirOpener->openUrl(url);
}

void ViewController::openPath(const QString &path)
{
    m_dirOpener->openPath(path);
}

void ViewController::leanOpenPath(const QString &path)
{
    m_dirOpener->leanOpenPath(path);
}

void ViewController::openRow(const int row)
{
    const auto directoryChild = sourceRow(row);
    if (directoryChild == -1)
    {
        qDebug("invalid openRow call with row - %d", row);
        return;
    }

    m_dirOpener->openChild(directoryChild);
}

void ViewController::openParentPath()
{
    // QDir::cdUp doesn't work for non existent files
    std::filesystem::path p = path().toStdWString();
    p /= "..";
    p = std::filesystem::absolute(p);

    openPath(QString::fromStdString(p.generic_string()));
}

void ViewController::setPreview(int row)
{
    const auto getPreviewData = [](
            std::shared_ptr<DirectorySystem> system,
            std::shared_ptr<Directory> dir,
            int child) -> PreviewData
    {
        if (dir->isDir(child))
            return {nullptr, nullptr, PreviewData::Unknown, 0};

        // some directory system may have custom urls, so you can't directly use fileUrl here
        const QString path = dir->filePath(child);
        const auto mime = QMimeDatabase().mimeTypeForUrl(QUrl::fromLocalFile(path)).name();

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

        std::unique_ptr<IODevice> device;
        std::unique_ptr<IOSource> source;

        if (filetype == PreviewData::VideoFile)
            device = system->iodevice(dir.get(), child);

        if (!device)
            source = system->iosource(dir.get(), child);

        if (!source && !device) {
            qDebug("failed to get iosource '%s'", qUtf8Printable(dir->fileUrl(child).toString()));
            return {nullptr, nullptr, PreviewData::Unknown, 1};
        }

        return PreviewData(std::move(source), std::move(device), filetype, 0);
    };


    const auto directoryRow = sourceRow(row);
    if (directoryRow == -1)
    {
        qDebug("invalid openRow call with row - %d", row);
        return;
    }

    // skip previous request
    const size_t requestID = ++m_previewRequest;

    auto root = m_dirModel->directory();
    if (!root)
        emit showPreview({}, row);

    using FutureVariant = std::variant<QFuture<PreviewData>, QFuture<FileHistoryDB::Data>>;

    QFuture<PreviewData> preview = QtConcurrent::run(
                m_pool.get(), getPreviewData, m_system, root, directoryRow);

    QFuture<FileHistoryDB::Data> data = m_historyDB->read(root->filePath(directoryRow));

    QtFuture::whenAll(preview, data)
            .then(this
                  , [this, requestID, row](const QList<FutureVariant> &results)
    {
        if (requestID != m_previewRequest)
            return;

        PreviewData preview = std::get<0>(results[0]).result();
        FileHistoryDB::Data data = std::get<1>(results[1]).result();

        preview.m_progress = data.progress.value_or(0);
        emit showPreview(preview, row);
    });
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
    auto dir = m_dirOpener->dir();
    if (!dir) return; // openening failed???

    setLoading(true);
    m_url = dir->url();

    auto history = m_pathHistoryDB->value(m_url.toString());
    m_dirModel->setDirectory(dir);

    if (history.onlyShowVideoFiles.has_value())
        m_sortModel->setOnlyShowVideoFile(history.onlyShowVideoFiles.value());

    if (history.randomsort.value_or(false))
    {
        m_sortModel->setRandomSortEx(true, history.randomseed.value_or(0));

        auto current = m_sortModel->index(history.random_row.value_or(0)
                                          , history.random_col.value_or(0));

        m_selectionModel.setCurrentIndex(current, QItemSelectionModel::ClearAndSelect);
    }
    else
    {

        if (history.sortcolumn.has_value())
        {
            const auto sortorder = (Qt::SortOrder)history.sortorder.value_or(Qt::AscendingOrder);
            m_sortModel->sort(history.sortcolumn.value()
                              , sortorder);
        }
        else
        {
            m_sortModel->setRandomSort(false);
        }

        auto current = m_sortModel->index(history.row.value_or(0)
                                          , history.col.value_or(0));

        m_selectionModel.setCurrentIndex(current, QItemSelectionModel::ClearAndSelect);
    }

    if (m_historyDB)
        m_historyDB->setPreviewed(m_dirModel->directory()->path(), true);

    setLoading(false);
    emit urlChanged();
}

void ViewController::updatePathHistory()
{
    if (loading()) return;

    auto current = m_selectionModel.currentIndex();
    if (!current.isValid())
        current = m_sortModel->index(0, 0);

    auto url = m_url.toString();
    auto history = m_pathHistoryDB->value(url);
    if (m_sortModel->randomSort())
    {
        history.randomsort = m_sortModel->randomSort();
        history.randomseed = m_sortModel->randomSeed();

        history.random_row = current.row();
        history.random_col = current.column();
    }
    else
    {
        history.randomsort = false;
        history.row = current.row();
        history.col = current.column();
        history.sortcolumn = m_sortModel->sortColumn();
        history.sortorder = m_sortModel->sortOrder();
    }

    history.onlyShowVideoFiles = m_sortModel->onlyShowVideoFile();

    qDebug() << "new history" << url << history.row << history.col;
    m_pathHistoryDB->insert(url, history);
}

void ViewController::syncUrl()
{
    if (loading() || (m_history.size() > 0 && m_url == m_history.currentUrl()))
        return;

    openUrl(m_history.currentUrl());
}

void ViewController::updateHistoryStack()
{
    if (loading() || (m_history.size() > 0 && m_url == m_history.currentUrl()))
        return;

    m_history.pushUrl(m_url);
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

    m_pathHistoryDB.reset( new PathHistoryDB(m_fileBrowser->pathHistoryDBPath()) );
}

bool ViewController::linearizeDirAvailable() const
{
    if (auto dir = m_dirModel->directory(); dir)
        return m_system->canLinearizeDir(dir->path());

    return false;
}

bool ViewController::isLinearDir() const
{
    if (auto dir = m_dirModel->directory(); dir)
        return dir->isLinearDir();

    return false;
}


bool ViewController::loading() const
{
    return m_loading;
}

void ViewController::setLoading(bool newLoading)
{
    if (m_loading == newLoading)
        return;
    m_loading = newLoading;
    emit loadingChanged();
}

HistoryStack *ViewController::history()
{
    return &m_history;
}

QItemSelectionModel *ViewController::selectionModel()
{
    return &m_selectionModel;
}
