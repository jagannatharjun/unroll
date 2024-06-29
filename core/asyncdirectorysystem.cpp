#include "asyncdirectorysystem.hpp"

#include <QtConcurrent/QtConcurrent>
#include <QThreadPool>

AsyncDirectorySystem::AsyncDirectorySystem(QThreadPool *pool, std::shared_ptr<DirectorySystem> system)
    : m_pool {pool}, m_system {system}
{
}

QFuture<std::shared_ptr<Directory> > AsyncDirectorySystem::open(const QUrl &url)
{
    QPromise<std::shared_ptr<Directory>> p;
    return QtConcurrent::run(m_pool, [system = m_system, url]() -> std::shared_ptr<Directory>
    {
        return system->open(url);
    });
}

QFuture<std::shared_ptr<Directory>>
AsyncDirectorySystem::open(const QString &path)
{
    QPromise<std::shared_ptr<Directory>> p;
    return QtConcurrent::run(
                m_pool, [system = m_system, path]() -> std::shared_ptr<Directory>
    {
        return system->open(path);
    });
}


QFuture<std::shared_ptr<Directory>>
AsyncDirectorySystem::open(std::shared_ptr<Directory> dir, int child)
{
    QPromise<std::shared_ptr<Directory>> p;
    return QtConcurrent::run(
                m_pool, [system = m_system, dir, child]() -> std::shared_ptr<Directory>
    {
        return system->open(dir.get(), child);
    });
}
