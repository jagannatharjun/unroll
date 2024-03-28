#ifndef ARCHIVESYSTEM_HPP
#define ARCHIVESYSTEM_HPP

#include "directorysystem.hpp"


class ArchiveSystem : public DirectorySystem
{
public:
    ArchiveSystem();

    // DirectorySystem interface
public:
    std::unique_ptr<Directory> open(const QUrl &url) override;
    std::unique_ptr<Directory> open(const QString &path) override;
    std::unique_ptr<Directory> open(Directory *dir, int child) override;

    std::unique_ptr<Directory> dirParent(Directory *dir) override;

    std::unique_ptr<IOSource> iosource(Directory *dir, int child) override;

private:

};




#endif // ARCHIVESYSTEM_HPP
