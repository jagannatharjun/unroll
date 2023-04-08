#include "asyncdirsystem.hpp"
#include <QtConcurrent>

AsyncDirSystem::AsyncDirSystem()
{

}

QFuture<std::unique_ptr<Directory> > AsyncDirSystem::open(const QUrl &url)
{
    return QtConcurrent::run([this, url]() { return m_system->open(url); });
}

QFuture<std::unique_ptr<Directory> > AsyncDirSystem::open(Directory *dir, int child)
{
    return QtConcurrent::run([this, dir, child]() { return m_system->open(dir, child); });
}
