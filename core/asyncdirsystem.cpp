#include "asyncdirsystem.hpp"
#include <QtConcurrent>


AsyncDirSystem::AsyncDirSystem(std::unique_ptr<DirectorySystem> s) : m_system {std::move(s)} {}

QFuture<std::shared_ptr<Directory>> AsyncDirSystem::open(const QUrl &url)
{
    return QtConcurrent::run([this, url]()
    {
        return std::shared_ptr<Directory>(m_system->open(url));
    });
}

QFuture<std::shared_ptr<Directory> > AsyncDirSystem::open(std::shared_ptr<Directory> dir, int child)
{
    return QtConcurrent::run([this, dir, child]()
    {
        return std::shared_ptr<Directory>(m_system->open(dir.get(), child));
    });
}
