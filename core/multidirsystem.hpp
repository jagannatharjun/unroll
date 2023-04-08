#ifndef MULTIDIRSYSTEM_HPP
#define MULTIDIRSYSTEM_HPP

#include "directorysystem.hpp"

#include <QFuture>

class MultiDirSystem : public DirectorySystem
{
public:
    MultiDirSystem();


    // DirectorySystem interface
public:
    std::unique_ptr<Directory> open(const QUrl &url) override;
    std::unique_ptr<Directory> open(Directory *dir, int child) override;

private:
    QVector<std::unique_ptr<DirectorySystem>> m_systems;
};

#endif // MULTIDIRSYSTEM_HPP
