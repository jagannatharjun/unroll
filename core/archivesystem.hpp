#ifndef ARCHIVESYSTEM_HPP
#define ARCHIVESYSTEM_HPP

#include "directorysystem.hpp"

class ArchiveSystem : public DirectorySystem
{
public:
    ArchiveSystem();

    // DirectorySystem interface
public:
    bool canopen(const QUrl &url) override;
    bool canopen(Directory *dir, int child) override;

    std::unique_ptr<Directory> open(const QUrl &url) override;
    std::unique_ptr<Directory> open(Directory *dir, int child) override;

    std::unique_ptr<IOSource> iosource(Directory *dir, int child) override;
};




#endif // ARCHIVESYSTEM_HPP
