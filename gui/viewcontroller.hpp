
#ifndef VIEWCONTROLLER_HPP
#define VIEWCONTROLLER_HPP

#include <memory>

#include <QObject>
#include <QFutureWatcher>
#include <QAbstractItemModel>
#include <QItemSelectionModel>

#include "../core/hybriddirsystem.hpp"
#include "iconprovider.hpp"
#include "filebrowser.hpp"
#include "historystack.hpp"
#include "pathhistorydb.hpp"

class QAbstractItemModel;
class HybridDirSystem;
class DirectorySystemModel;
class DirectorySortModel;
class FileHistoryDB;
class PathHistoryDB;
class DirectoryOpener;
class QThreadPool;

class PreviewData
{
    Q_GADGET
public:
    enum FileType
    {
        Unknown,
        ImageFile,
        AudioFile,
        VideoFile
    };

    Q_ENUM(FileType)

    PreviewData() = default; // construct invalid PreviewData
    PreviewData(std::shared_ptr<IOSource> source, FileType type, double progress)
        : source {source}
        , m_progress {progress}
        , m_type {type} {}

    Q_INVOKABLE QString readPath() const { return source ? source->readPath() : QString {}; }
    Q_INVOKABLE QUrl readUrl() const { return QUrl::fromLocalFile(readPath()); }

    Q_INVOKABLE FileType fileType() const { return m_type; }

    Q_INVOKABLE double progress() const { return m_progress; }

    bool valid() const { return !!source; }

private:
    std::shared_ptr<IOSource> source;
    double m_progress = 0;
    FileType m_type;

    friend class ViewController;
};


class ViewController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* model READ model CONSTANT)
    Q_PROPERTY(QItemSelectionModel *selectionModel READ selectionModel CONSTANT)

    Q_PROPERTY(HistoryStack* history READ history CONSTANT)

    Q_PROPERTY(QString url READ url NOTIFY urlChanged)
    Q_PROPERTY(QString path READ path NOTIFY urlChanged)
    Q_PROPERTY(bool linearizeDirAvailable READ linearizeDirAvailable NOTIFY urlChanged)
    Q_PROPERTY(bool isLinearDir READ isLinearDir NOTIFY urlChanged FINAL)

    Q_PROPERTY(FileBrowser *fileBrowser READ fileBrowser WRITE setFileBrowser NOTIFY fileBrowserChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged FINAL)

public:
    ViewController(QObject *parent = nullptr);
    ~ViewController();

    QAbstractItemModel *model();

    QString url() const;
    QString path() const;

    Q_INVOKABLE void refresh();

    Q_INVOKABLE PreviewData invalidPreviewData();

    FileBrowser *fileBrowser() const;
    void setFileBrowser(FileBrowser *newFileBrowser);

    bool linearizeDirAvailable() const;

    bool isLinearDir() const;

    bool loading() const;

    HistoryStack *history();

    QItemSelectionModel *selectionModel();

public slots:
    void openUrl(const QUrl &url);
    void openPath(const QString &path);
    void leanOpenPath(const QString &path);
    void openRow(const int row);
    void openParentPath();

    void setPreview(int row);

signals:
    void urlChanged();
    void showPreview(PreviewData data);

    void fileBrowserChanged();

    void isLinearDirChanged();

    void loadingChanged();

private slots:
    void updateModel();

    void updatePathHistory();

    void syncUrl();

    void updateHistoryStack();

private:
    int sourceRow(const int row);

    void setLoading(bool newLoading);

    QString iconID(Directory *dir, int child);

    // place this at start so it get destroyed last, this
    // make sure any pending operation don't use invalid resource
    std::shared_ptr<QThreadPool> m_pool;

    // owned by qqmlengine
    IconProvider *m_iconProvider {};

    // store current url of m_model, store it seperately so
    // that url() and model updates are always in sync
    QUrl m_url;

    std::unique_ptr<DirectorySystemModel> m_dirModel;
    std::unique_ptr<DirectorySortModel> m_sortModel;

    std::shared_ptr<HybridDirSystem> m_system;
    std::unique_ptr<DirectoryOpener> m_dirOpener;

    size_t m_previewRequest = 0;

    FileBrowser *m_fileBrowser = nullptr;
    bool m_loading;
    HistoryStack m_history;

    std::shared_ptr<FileHistoryDB> m_historyDB;
    std::shared_ptr<PathHistoryDB> m_pathHistoryDB;

    QItemSelectionModel m_selectionModel;
};

#endif // VIEWCONTROLLER_HPP
