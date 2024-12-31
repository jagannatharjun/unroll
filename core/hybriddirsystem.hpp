#ifndef MULTIDIRSYSTEM_HPP
#define MULTIDIRSYSTEM_HPP

#include "directorysystem.hpp"

#include <QReadWriteLock>
#include <QHash>

class FileSystem;
class ArchiveSystem;

class HybridDirSystem : public DirectorySystem
{
    Q_DISABLE_COPY_MOVE(HybridDirSystem)

public:
    HybridDirSystem();
    ~HybridDirSystem();

    // DirectorySystem interface
public:
    std::unique_ptr<Directory> open(const QString &path) override;
    std::unique_ptr<Directory> open(const QUrl &url) override;
    std::unique_ptr<Directory> open(Directory *dir, int child) override;

    std::unique_ptr<Directory> dirParent(Directory *dir) override;

    std::unique_ptr<IOSource> iosource(Directory *dir, int child) override;

    void setLeanModeForFileSystem(bool leanMode);

    bool leanModeForFileSystem() const;

private:
    std::unique_ptr<Directory> call(std::function<std::unique_ptr<Directory>(DirectorySystem *)> functor)
    {
        for (auto source : std::as_const(m_systems))
        {
            auto r = functor(source);
            if (!r) continue;

            updatesource(r.get(), source);
            return r;
        }

        return nullptr;
    }

    std::unique_ptr<Directory> call(Directory *dir, std::function<std::unique_ptr<Directory>(DirectorySystem *)> functor)
    {
        if (auto system = source(dir))
        {
            auto r = functor(system);
            if (!r) return nullptr;

            updatesource(r.get(), system);
            return r;
        }

        return nullptr;
    }

    void updatesource(Directory * dir, DirectorySystem *source);
    DirectorySystem *source(Directory *dir);

    std::unique_ptr<FileSystem> m_filesystem;
    std::unique_ptr<ArchiveSystem> m_archivesystem;
    std::array<DirectorySystem *, 2> m_systems;

    QReadWriteLock m_lock;
    QHash<Directory *, DirectorySystem *> m_sources;
};

#endif // MULTIDIRSYSTEM_HPP
