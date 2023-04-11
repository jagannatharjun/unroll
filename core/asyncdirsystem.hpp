#ifndef ASYNCDIRSYSTEM_HPP
#define ASYNCDIRSYSTEM_HPP

#include "directorysystem.hpp"

#include <QFuture>

class AsyncDirSystem
{
public:
    AsyncDirSystem(std::unique_ptr<DirectorySystem> s);

    QFuture<std::shared_ptr<Directory>> open(const QUrl &url);
    QFuture<std::shared_ptr<Directory>> open(std::shared_ptr<Directory> dir, int child);

private:
    std::unique_ptr<DirectorySystem> m_system;
};

#endif // ASYNCDIRSYSTEM_HPP
