
#include "viewcontroller.hpp"

#include "../core/directorysystemmodel.hpp"
#include "../core/hybriddirsystem.hpp"

#include <QtConcurrent/QtConcurrent>


ViewController::ViewController(QObject *parent)
    : QObject {parent}
    , m_system { std::make_unique<HybridDirSystem>()}
    , m_model { std::make_unique<DirectorySystemModel>()}
{
    connect(&m_urlWatcher, &QFutureWatcherBase::finished, this, &ViewController::updateModel);
}

ViewController::~ViewController()
{

}

QAbstractItemModel *ViewController::model()
{
    return m_model.get();
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

void ViewController::openIndex(const int index)
{
    const auto open = [](
            std::shared_ptr<DirectorySystem> system,
            std::shared_ptr<Directory> dir,
            int child) -> std::shared_ptr<Directory>
    {
        return system->open(dir.get(), child);
    };

    if (auto parent = validParent(index))
    {
        m_urlWatcher.setFuture(QtConcurrent::run(&m_pool, open, m_system, parent, index));
    }
}

void ViewController::clicked(int index)
{

}

void ViewController::doubleClicked(int index)
{
    const auto action = [](
              std::shared_ptr<DirectorySystem> system
            , std::shared_ptr<Directory> dir
            , int child) -> std::shared_ptr<Directory>
    {
        return system->open(dir.get(), child);
    };

    if (auto parent = validParent(index))
    {
        m_urlWatcher.setFuture(QtConcurrent::run(&m_pool, action, m_system, parent, index));
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
        m_model->setDirectory(s->result());
        emit urlChanged();
    }
}

