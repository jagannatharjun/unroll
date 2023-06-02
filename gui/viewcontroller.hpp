
#ifndef VIEWCONTROLLER_HPP
#define VIEWCONTROLLER_HPP

#include <memory>
#include <stack>

#include <QObject>
#include <QFutureWatcher>
#include <QAbstractItemModel>
#include <QThreadPool>

#include "../core/directorysystem.hpp"
#include "iconprovider.hpp"

class QAbstractItemModel;
class DirectorySystemModel;
class DirectorySortModel;

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
    PreviewData(std::shared_ptr<IOSource> source, FileType type) : source {source}, m_type {type} {}

    Q_INVOKABLE QString readPath() const { return source->readPath(); }
    Q_INVOKABLE QUrl readUrl() const { return QUrl::fromLocalFile(readPath()); }

    Q_INVOKABLE FileType fileType() const { return m_type; }

    bool valid() const { return !!source; }

private:
    std::shared_ptr<IOSource> source;
    FileType m_type;
};


class ViewController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* model READ model CONSTANT)

    Q_PROPERTY(QString url READ url NOTIFY urlChanged)

public:
    ViewController(QObject *parent = nullptr);
    ~ViewController();

    QAbstractItemModel *model();

    QString url() const;


public slots:
    void openUrl(const QUrl &url);
    void openRow(const int row);
    void setPreview(int row);

signals:
    void urlChanged();
    void showPreview(PreviewData data);

private slots:
    void updateModel();
    void updatePreview();

private:
    int sourceRow(const int row);

    QString iconID(Directory *dir, int child);

    IconProvider *m_iconProvider {};

    // store current url of m_model, store it seperately so
    // that url() and model updates are always in sync
    QUrl m_url;

    std::unique_ptr<DirectorySystemModel> m_dirModel;
    std::unique_ptr<DirectorySortModel> m_sortModel;

    std::shared_ptr<DirectorySystem> m_system;
    QFutureWatcher<std::shared_ptr<Directory>> m_urlWatcher;

    QFutureWatcher<PreviewData> m_previewWatcher;

    // place this at end so it get destroyed first, this
    // make sure any pending operation don't use invalid resource
    QThreadPool m_pool;
};


#endif // VIEWCONTROLLER_HPP
