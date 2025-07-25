#ifndef FILESYSTEM_HPP
#define FILESYSTEM_HPP

#include "directorysystem.hpp"
#include <memory>

class QUrl;

class FileSystem : public DirectorySystem
{
public:
    std::unique_ptr<Directory> leanOpen(const QString &path);

    std::unique_ptr<Directory> open(const QString &path) override;
    std::unique_ptr<Directory> open(const QUrl &url) override;
    std::unique_ptr<Directory> open(Directory *dir, int child) override;

    std::unique_ptr<Directory> dirParent(Directory *dir) override;

    std::unique_ptr<IOSource> iosource(Directory *dir, int child) override;
};

#endif // FILESYSTEM_HPP
