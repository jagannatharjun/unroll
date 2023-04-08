#include "multidirsystem.hpp"

#include "filesystem.hpp"
#include "archivesystem.hpp"

MultiDirSystem::MultiDirSystem()
    : m_systems { std::make_unique<FileSystem>(), std::make_unique<ArchiveSystem>()}
{
}

std::unique_ptr<Directory> MultiDirSystem::open(const QUrl &url)
{
    for (auto &system : m_systems)
        if (auto r = system->open(url))
            return r;

    return nullptr;
}

std::unique_ptr<Directory> MultiDirSystem::open(Directory *dir, int child)
{
    for (auto &system : m_systems)
        if (auto r = system->open(dir, child))
            return r;

    return nullptr;
}

