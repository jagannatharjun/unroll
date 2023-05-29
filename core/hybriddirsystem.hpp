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
    std::unique_ptr<Directory> open(const QUrl &url) override;
    std::unique_ptr<Directory> open(Directory *dir, int child) override;

    std::unique_ptr<IOSource> iosource(Directory *dir, int child) override;

private:
    void updatesource(Directory * dir, DirectorySystem *source);
    DirectorySystem *source(Directory *dir);

    std::unique_ptr<FileSystem> m_filesystem;
    std::unique_ptr<ArchiveSystem> m_archivesystem;
    QVector<DirectorySystem *> m_systems;

    QReadWriteLock m_lock;
    QHash<Directory *, DirectorySystem *> m_sources;
};

#endif // MULTIDIRSYSTEM_HPP
