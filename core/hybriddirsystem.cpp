#include "hybriddirsystem.hpp"

#include "filesystem.hpp"
#include "archivesystem.hpp"

#include <QFileInfo>
#include <QDir>

HybridDirSystem::HybridDirSystem()
    : m_filesystem (new FileSystem), m_archivesystem (new ArchiveSystem) , m_systems { m_filesystem.get(), m_archivesystem.get() }
{
}

HybridDirSystem::~HybridDirSystem()
{

}

std::unique_ptr<Directory> HybridDirSystem::open(const QString &path)
{
    return call([path](DirectorySystem *system)
    {
        return system->open(path);
    });
}

std::unique_ptr<Directory> HybridDirSystem::open(const QUrl &url)
{
    return call([url](DirectorySystem *system)
    {
        return system->open(url);
    });
}

std::unique_ptr<Directory> HybridDirSystem::open(Directory *dir, int child)
{
    if (auto system = source(dir))
    {
        auto r = system->open(dir, child);

        if (r)
        {
            updatesource(r.get(), system);
            return r;
        }
    }

    return open(dir->fileUrl(child));
}

std::unique_ptr<Directory> HybridDirSystem::dirParent(Directory *dir)
{
    if (auto system = source(dir))
    {
        auto r = system->dirParent(dir);

        if (r)
        {
            updatesource(r.get(), system);
            return r;
        }
        else
        {
            QDir d(dir->path());
            if (d.cdUp())
                return open(d.path());
        }
    }

    return nullptr;
}

std::unique_ptr<IOSource> HybridDirSystem::iosource(Directory *dir, int child)
{
    if (auto system = source(dir))
    {
        return system->iosource(dir, child);
    }

    return nullptr;
}

void HybridDirSystem::updatesource(Directory *dir, DirectorySystem *source)
{
    assert(dir);

    QWriteLocker l(&m_lock);
    m_sources[dir] = source;
}

DirectorySystem *HybridDirSystem::source(Directory *dir)
{
    assert(dir);

    QReadLocker l(&m_lock);
    return m_sources.value(dir);
}

