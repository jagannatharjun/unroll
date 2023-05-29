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

