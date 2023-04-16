#include "directorysystemmodel.hpp"

#include "directorysystem.hpp"

DirectorySystemModel::DirectorySystemModel(QObject *parent)
    : QAbstractListModel{parent}
{
}

DirectorySystemModel::~DirectorySystemModel()
{

}

void DirectorySystemModel::setDirectory(std::shared_ptr<Directory> dir)
{
    beginResetModel();
    m_dir = dir;
    endResetModel();
}

std::shared_ptr<Directory> DirectorySystemModel::directory()
{
    return m_dir;
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
