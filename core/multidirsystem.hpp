#ifndef MULTIDIRSYSTEM_HPP
#define MULTIDIRSYSTEM_HPP

#include "directorysystem.hpp"

#include <QFuture>

class MultiDirSystem : public DirectorySystem
{
    Q_DISABLE_COPY_MOVE(MultiDirSystem)

public:
    MultiDirSystem();
    ~MultiDirSystem();

    // DirectorySystem interface
public:
    std::unique_ptr<Directory> open(const QUrl &url) override;
    std::unique_ptr<Directory> open(Directory *dir, int child) override;

private:
    QVector<DirectorySystem *> m_systems;
    QHash<Directory*, DirectorySystem *> m_source;
};

#endif // MULTIDIRSYSTEM_HPP
