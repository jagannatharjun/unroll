#pragma once

#include "directorysystem.hpp"

#include <memory>

#include <QFuture>

class QThreadPool;

class AsyncDirectorySystem
{
public:
    AsyncDirectorySystem(QThreadPool *pool, std::shared_ptr<DirectorySystem> system);

    QFuture<std::shared_ptr<Directory>> open(const QUrl &url);

    QFuture<std::shared_ptr<Directory>> open(const QString &path);

    QFuture<std::shared_ptr<Directory>> open(std::shared_ptr<Directory> dir
                                             , int child);

private:
    QThreadPool *m_pool;
    std::shared_ptr<DirectorySystem> m_system;
};
