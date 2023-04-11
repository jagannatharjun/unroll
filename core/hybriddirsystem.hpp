#ifndef MULTIDIRSYSTEM_HPP
#define MULTIDIRSYSTEM_HPP

#include "directorysystem.hpp"

#include <QFuture>

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

private:
    std::unique_ptr<FileSystem> m_filesystem;
    std::unique_ptr<ArchiveSystem> m_archivesystem;
    QHash<Directory *, DirectorySystem *> m_sources;
};

#endif // MULTIDIRSYSTEM_HPP
