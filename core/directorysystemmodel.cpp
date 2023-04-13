#include "directorysystemmodel.hpp"
#include "hybriddirsystem.hpp"

DirectorySystemModel::DirectorySystemModel(QObject *parent)
    : QAbstractListModel{parent}
    , m_loader { std::make_unique<HybridDirSystem>() }
{
    using Watcher = decltype(m_watcher);
    connect(&m_watcher, &Watcher::finished
            , this, &DirectorySystemModel::handleLoadingFinished);
}

bool DirectorySystemModel::isLoading() const
{
    return m_watcher.isRunning();
}

void DirectorySystemModel::open(const QUrl &url)
{
    beginResetModel();
    m_dir.reset();

    m_watcher.setFuture(m_loader.open(url));
    emit isLoadingChanged();
}

void DirectorySystemModel::openindex(int index)
{
    beginResetModel();
    assert(m_dir);
    auto parent = m_dir;
    m_dir.reset();

    m_watcher.setFuture(m_loader.open(parent, index));
    emit isLoadingChanged();
}

void DirectorySystemModel::handleLoadingFinished()
{
    m_dir = m_watcher.result();
    endResetModel();

    emit isLoadingChanged();
}

int DirectorySystemModel::rowCount(const QModelIndex &) const
{
    return m_dir ? m_dir->fileCount() : 0;
}

QVariant DirectorySystemModel::data(const QModelIndex &index, int role) const
{
    if (!m_dir || !checkIndex(index))
        return {};

    const int r = index.row();
    switch (role)
    {
    case NameRole:
        return m_dir->fileName(r);
    case PathRole:
        return m_dir->filePath(r);
    case SizeRole:
        return m_dir->fileSize(r);
    case IsDirRole:
        return m_dir->isDir(r);
    }

    return {};
}

QHash<int, QByteArray> DirectorySystemModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {PathRole, "path"},
        {SizeRole, "size"},
        {IsDirRole, "isdir"},
    };
}
