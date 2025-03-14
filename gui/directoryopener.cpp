#include "directoryopener.hpp"

#include <QtConcurrent/QtConcurrent>
#include <QThreadPool>
#include <filesystem>

#include "../core/directorysystem.hpp"
#include "../core/hybriddirsystem.hpp"

DirectoryOpener::DirectoryOpener(std::shared_ptr<QThreadPool> pool, std::shared_ptr<DirectorySystem> system, QObject *parent)
    : QObject{parent}
    , m_pool {pool}
    , m_system {system}
{}

std::shared_ptr<Directory> DirectoryOpener::dir() const
{
    return m_dir;
}

void DirectoryOpener::openUrl(const QUrl &url)
{
    const auto open = [](
                          std::shared_ptr<DirectorySystem> system,
                          const QUrl &url) -> std::shared_ptr<Directory>
    {
        return system->open(url);
    };

    nextDir(QtConcurrent::run(m_pool.get(), open, m_system, url));
}

void DirectoryOpener::openPath(const QString &path)
{
    const auto open = [](
                          std::shared_ptr<DirectorySystem> system,
                          const QString &path) -> std::shared_ptr<Directory>
    {
        return system->open(path);
    };

    nextDir(QtConcurrent::run(m_pool.get(), open, m_system, path));
}

void DirectoryOpener::leanOpenPath(const QString &path)
{
    auto system = std::dynamic_pointer_cast<HybridDirSystem>(m_system);

    const auto open = [](
                          std::shared_ptr<HybridDirSystem> system,
                          const QString &path) -> std::shared_ptr<Directory>
    {
        return system->leanOpenDir(path);
    };

    nextDir(QtConcurrent::run(m_pool.get(), open, system, path));
}

void DirectoryOpener::openChild(const int child)
{
    if (!m_dir) return;

    const auto open = [](
                          std::shared_ptr<DirectorySystem> system,
                          std::shared_ptr<Directory> dir,
                          int child) -> std::shared_ptr<Directory>
    {
        return system->open(dir.get(), child);
    };

    nextDir(QtConcurrent::run(m_pool.get(), open, m_system, m_dir, child));
}

void DirectoryOpener::openParentPath()
{
    if (!m_dir) return;

    // QDir::cdUp doesn't work for non existent files
    std::filesystem::path p = m_dir->path().toStdWString();
    p /= "..";
    p = std::filesystem::absolute(p);

    openPath(QString::fromStdString(p.generic_string()));
}

void DirectoryOpener::nextDir(QFuture<std::shared_ptr<Directory> > &&future)
{
    auto requestID = ++m_currentRequest;
    future.then(this, [this, requestID](std::shared_ptr<Directory> dir)
    {
        if (requestID != m_currentRequest)
            return;

        m_dir = dir;
        emit directoryChanged();
    });
}
