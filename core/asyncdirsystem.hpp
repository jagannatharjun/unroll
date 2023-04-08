#ifndef ASYNCDIRSYSTEM_HPP
#define ASYNCDIRSYSTEM_HPP

#include "directorysystem.hpp"

#include <QFuture>

class AsyncDirSystem
{
public:
    AsyncDirSystem(std::unique_ptr<DirectorySystem> s) : m_system {std::move(s)} {}

    QFuture<std::unique_ptr<Directory>> open(const QUrl &url);
    QFuture<std::unique_ptr<Directory>> open(Directory *dir, int child);

private:
    std::unique_ptr<DirectorySystem> m_system;
};

#endif // ASYNCDIRSYSTEM_HPP
