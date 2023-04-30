#include "hybriddirsystem.hpp"

#include "filesystem.hpp"
#include "archivesystem.hpp"

#include <QFileInfo>

HybridDirSystem::HybridDirSystem()
    : m_filesystem (new FileSystem), m_archivesystem (new ArchiveSystem) , m_systems { m_filesystem.get(), m_archivesystem.get() }
{
}

HybridDirSystem::~HybridDirSystem()
{

}

bool HybridDirSystem::canopen(const QUrl &url)
{
    return m_filesystem->canopen(url) || m_archivesystem->canopen(url);
}

bool HybridDirSystem::canopen(Directory *dir, int child)
{
    DirectorySystem *sourcesystem = source(dir);
    if (sourcesystem)
    {
        // only return is source can open this, otherwise test with other available systems
        if (sourcesystem->canopen(dir, child)) return true;
    }

    for (auto system : qAsConst(m_systems))
    {
        if (system == sourcesystem) continue; // don't double check since this operation can be expensive
        if (system->canopen(dir->fileUrl(child))) return true;
    }

    return false;
}

std::unique_ptr<Directory> HybridDirSystem::open(const QUrl &url)
{
    const auto trysystem = [this, url](DirectorySystem *system)
    {
        auto r = system->open(url);
        if (r) updatesource(r.get(), system);
        return r;
    };

    if (auto r = trysystem(m_filesystem.get()))
        return r;

    return trysystem(m_archivesystem.get());
}

std::unique_ptr<Directory> HybridDirSystem::open(Directory *dir, int child)
{
    std::unique_ptr<Directory> result;

    if (auto system = source(dir))
    {
        result = system->open(dir, child);
        if (result)
            updatesource(result.get(), system);
    }

    if (!result) // retry with child url
    {
        qWarning("failed to get value from source trying open('%s')", qPrintable(dir->fileUrl(child).toString()));
        result = open(dir->fileUrl(child));
    }

    return result;
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

