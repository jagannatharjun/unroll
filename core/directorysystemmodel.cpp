#include "directorysystemmodel.hpp"

#include "directorysystem.hpp"

#include <QTextStream>

namespace
{

QString formatSize(qint64 size) {
    static const char * units[] = {"Bytes", "KB", "MB", "GB", "TB", "PB", "EB"};
    int idx = 0;

    double bytes = size;
    while(bytes >= 1024 && idx < sizeof (units) - 1) {
        bytes /= 1024;
        idx++;
    }

    return QString("%1 %2").arg(bytes, 0, 'f', 2).arg(units[idx]);
}

}

DirectorySystemModel::DirectorySystemModel(QObject *parent)
    : QAbstractTableModel{parent}
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

int DirectorySystemModel::columnCount(const QModelIndex &parent) const
{
    return ColumnCount;
}

QVariant DirectorySystemModel::data(const QModelIndex &index, int role) const
{
    if (!m_dir || !checkIndex(index))
        return {};


    const int r = index.row();
    const int c = index.column();

    const auto displayRole = [&]() -> QString
    {
        switch (c)
        {
        case NameColumn:
            return m_dir->fileName(r);
        case PathColumn:
            return m_dir->filePath(r);
        case SizeColumn:
            if (m_dir->isDir(r) && m_dir->fileSize(r) == 0)
                return QString {};

            return formatSize(m_dir->fileSize(r));
        }

        return "";
    };

    const auto sortRole = [&]() -> QVariant
    {
        switch (c)
        {
        case NameColumn:
            return m_dir->fileName(r);
        case PathColumn:
            return m_dir->filePath(r);
        case SizeColumn:
            return m_dir->fileSize(r);
        }

        return {};
    };

    switch (role)
    {
    case Qt::DisplayRole:
        return displayRole();
    case DataRole:
        return sortRole();
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

QVariant DirectorySystemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return {};

    switch (section)
    {
    case NameColumn:
        return "Name";
    case PathColumn:
        return "Path";
    case SizeColumn:
        return "Size";
    }

    return {};
}

QHash<int, QByteArray> DirectorySystemModel::roleNames() const
{
    return {
        {Qt::DisplayRole, "display"},
        {NameRole, "name"},
        {PathRole, "path"},
        {SizeRole, "size"},
        {IsDirRole, "isdir"},
    };
}
