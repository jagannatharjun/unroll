
#ifndef VIEWCONTROLLER_HPP
#define VIEWCONTROLLER_HPP

#include <memory>

#include <QObject>
#include <QFutureWatcher>
#include <QAbstractItemModel>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrent>

#include "../core/directorysystemmodel.hpp"
#include "../core/directorysortmodel.hpp"
#include "../core/hybriddirsystem.hpp"
#include "iconprovider.hpp"
#include "filebrowser.hpp"

class QAbstractItemModel;
class HybridDirSystem;
class FileHistoryDB;

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

    Q_PROPERTY(QString url READ url NOTIFY urlChanged)
    Q_PROPERTY(QString path READ path NOTIFY urlChanged)
    Q_PROPERTY(bool linearizeDirAvailable READ linearizeDirAvailable NOTIFY urlChanged)
    Q_PROPERTY(bool isLinearDir READ isLinearDir NOTIFY urlChanged FINAL)

    Q_PROPERTY(FileBrowser *fileBrowser READ fileBrowser WRITE setFileBrowser NOTIFY fileBrowserChanged)

public:
    ViewController(QObject *parent = nullptr);
    ~ViewController();

    QAbstractItemModel *model();
    DirectorySortModel *sortModel();

    QUrl urlEx() const;
    QString url() const;
    QString path() const;

    Q_INVOKABLE void refresh();

    Q_INVOKABLE PreviewData invalidPreviewData();

    FileBrowser *fileBrowser() const;
    void setFileBrowser(FileBrowser *newFileBrowser);

    bool linearizeDirAvailable() const;

    bool isLinearDir() const;

public slots:
    void openUrl(const QUrl &url);
    void openUrlWithRandomSort(const QUrl &url, int randomSeed);
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

private:
    template <typename Func, typename... Args>
    void requestUrlUpdate(Func&& func, Args&&... args)
    {
        requestUrlEx(false, -1, std::forward<Func>(func), std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args>
    void requestUrlEx(bool randomSort, int randomSeed, Func&& func, Args&&... args)
    {
        auto request = ++m_urlRequest;
        qDebug() << "requestUrlEx" << randomSort << randomSeed;
        const auto handleUpdate = [this, request, randomSort, randomSeed](std::shared_ptr<Directory> dir)
        {
            if (request != m_urlRequest || !dir)
                return;

            m_sortModel->setRandomSortEx(randomSort, randomSeed);

            m_dirModel->setDirectory(dir);

            emit urlChanged();
        };

        auto f = QtConcurrent::run(&m_pool, std::forward<Func>(func), std::forward<Args>(args)...);
        f.then(this, handleUpdate);
    }

    int sourceRow(const int row);

    QString iconID(Directory *dir, int child);

    // place this at start so it get destroyed last, this
    // make sure any pending operation don't use invalid resource
    QThreadPool m_pool;

    IconProvider *m_iconProvider {};

    std::unique_ptr<DirectorySystemModel> m_dirModel;
    std::unique_ptr<DirectorySortModel> m_sortModel;

    std::shared_ptr<HybridDirSystem> m_system;
    intmax_t m_urlRequest = 0;

    size_t m_previewRequest = 0;

    std::shared_ptr<FileHistoryDB> m_historyDB;
    FileBrowser *m_fileBrowser = nullptr;
};

#endif // VIEWCONTROLLER_HPP
