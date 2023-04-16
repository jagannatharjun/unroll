
#include "viewcontroller.hpp"

#include "../core/directorysystemmodel.hpp"
#include "../core/hybriddirsystem.hpp"

#include <QtConcurrent/QtConcurrent>

namespace
{

QFuture<std::shared_ptr<Directory>> open_async(std::shared_ptr<DirectorySystem> system, const QUrl &url)
{
    return QtConcurrent::run([system, url]() -> std::shared_ptr<Directory>
    {
        return system->open(url);
    });
}

QFuture<std::shared_ptr<Directory>> open_async(std::shared_ptr<DirectorySystem> system, std::shared_ptr<Directory> dir, int child)
{
    return QtConcurrent::run([system, dir, child]() -> std::shared_ptr<Directory>
    {
        return system->open(dir.get(), child);
    });
}

QFuture<std::shared_ptr<Directory>> doubleClickAction_async(std::shared_ptr<DirectorySystem> system, std::shared_ptr<Directory> dir, int child)
{
    return QtConcurrent::run([system, dir, child]() -> std::shared_ptr<Directory>
    {
        std::shared_ptr<Directory> result = system->open(dir.get(), child);
        return result;
    });
}

}

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
    m_urlWatcher.setFuture(open_async(m_system, url));
}

void ViewController::clicked(int index)
{

}

void ViewController::doubleClicked(int index)
{
    if (auto parent = validParent(index))
    {
        m_urlWatcher.setFuture(doubleClickAction_async(m_system, parent, index));
    }
}

void ViewController::openIndex(const int index)
{
    if (auto parent = validParent(index))
    {
        m_urlWatcher.setFuture(open_async(m_system, parent, index));
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

