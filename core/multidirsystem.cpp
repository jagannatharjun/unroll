#include "multidirsystem.hpp"

#include "filesystem.hpp"
#include "archivesystem.hpp"

MultiDirSystem::MultiDirSystem()
    : m_systems { new FileSystem(), new ArchiveSystem() }
{
}

MultiDirSystem::~MultiDirSystem()
{
    qDeleteAll(m_systems);
}

std::unique_ptr<Directory> MultiDirSystem::open(const QUrl &url)
{
    for (auto system : qAsConst(m_systems))
    {
        if (auto r = system->open(url))
        {
            m_source[r.get()] = system;
            return r;
        }
    }

    return nullptr;
}

std::unique_ptr<Directory> MultiDirSystem::open(Directory *dir, int child)
{
    std::unique_ptr<Directory> result;

    if (auto system = m_source.value(dir, nullptr))
        result = system->open(dir, child);
// TODO handle multi dir system
//    if (!result)
//        result = open(dir->)

    qWarning("can't find source for dir in MultiDirSystem::open(Directory*)");
    return nullptr;
}

