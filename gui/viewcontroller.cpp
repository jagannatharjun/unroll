
#include "viewcontroller.hpp"

#include "../core/directorysystemmodel.hpp"
#include "../core/hybriddirsystem.hpp"
#include "../core/filetype.hpp"

#include <QtConcurrent/QtConcurrent>
#include <QMimeData>


ViewController::ViewController(QObject *parent)
    : QObject {parent}
    , m_model {std::make_unique<DirectorySystemModel>()}
    , m_selectionModel {std::make_unique<QItemSelectionModel>()}
    , m_system {std::make_unique<HybridDirSystem>()}
{
    connect(&m_urlWatcher, &QFutureWatcherBase::finished, this, &ViewController::updateModel);
    connect(&m_previewWatcher, &QFutureWatcherBase::finished, this, &ViewController::updatePreview);
}

ViewController::~ViewController()
{

}

QAbstractItemModel *ViewController::model()
{
    return m_model.get();
}

QItemSelectionModel *ViewController::selectionModel()
{
    return m_selectionModel.get();
}

QString ViewController::url() const
{
    return m_model->directory() ? m_model->directory()->url().toString() : QString {};
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

void ViewController::openRow(const int row)
{
    const auto open = [](
            std::shared_ptr<DirectorySystem> system,
            std::shared_ptr<Directory> dir,
            int child) -> std::shared_ptr<Directory>
    {
        return system->open(dir.get(), child);
    };

    if (auto parent = validParent(row))
    {
        m_urlWatcher.setFuture(QtConcurrent::run(&m_pool, open, m_system, parent, row));
    }
}

void ViewController::setPreview(int row)
{
    const auto getiosource = [](
            std::shared_ptr<DirectorySystem> system,
            std::shared_ptr<Directory> dir,
            int child) -> PreviewData
    {
        if (dir->isDir(child))
            return {nullptr, PreviewData::Unknown};

        // some directory system has custom urls, so you can directly use fileUrl here
        const auto mime = QMimeDatabase().mimeTypeForUrl(QUrl::fromLocalFile(dir->filePath(child))).name();
        PreviewData::FileType filetype = PreviewData::Unknown;
        const auto types =
        {
            std::pair {PreviewData::ImageFile, "image"},
            {PreviewData::AudioFile, "audio"},
            {PreviewData::VideoFile, "video"}
        };

        for (const auto type : types) {
            if (mime.startsWith(type.second)) {
                filetype = type.first;
                break;
            }
        }

        return PreviewData(system->iosource(dir.get(), child), filetype);
    };

    if (auto parent = validParent(row))
    {
        m_previewWatcher.setFuture(QtConcurrent::run(&m_pool, getiosource, m_system, parent, row));
    }
    else
    {
        emit showPreview({nullptr, PreviewData::Unknown});
    }
}

std::shared_ptr<Directory> ViewController::validParent(const int index)
{
    if (auto parent =  m_model->directory())
    {
        if (index >= 0 && index < parent->fileCount())
            return parent;
    }

    return nullptr;
}

void ViewController::updateModel()
{
    auto s = dynamic_cast<decltype (m_urlWatcher) *>(sender());
    if (s && s->result())
    {
        // first trigger url change on UI side, so that view can redo itself based on new URL
        m_url = s->result()->url();
        m_model->setDirectory(s->result());

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

