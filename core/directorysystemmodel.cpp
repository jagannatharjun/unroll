#include "directorysystemmodel.hpp"

#include "directorysystem.hpp"

#include <QTextStream>
#include <QDir>

namespace
{

QString formatSize(qint64 size) {
    const auto l = QLocale::system();
    return l.formattedDataSize(size);
}


QString formatDateTime(const QDateTime &t)
{
    if (!t.isValid() || t.toMSecsSinceEpoch() == 0) return {};

    const auto l = QLocale::system();
    return l.toString(t);
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
            if (role == Qt::DisplayRole)
                return QDir::toNativeSeparators(m_dir->filePath(r));

            return m_dir->filePath(r);
        case SizeColumn:
            if (m_dir->isDir(r) && m_dir->fileSize(r) == 0)
                return QString {};

            return formatSize(m_dir->fileSize(r));

        case LastAccessTimeColumn:
            return formatDateTime(m_dir->fileLastAccessTime(r));
        case ModifedTimeColumn:
            return formatDateTime(m_dir->fileModifiedTime(r));
        case CreationTimeColumn:
            return formatDateTime(m_dir->fileCreationTime(r));
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
        case LastAccessTimeColumn:
            return m_dir->fileLastAccessTime(r);
        case ModifedTimeColumn:
            return m_dir->fileModifiedTime(r);
        case CreationTimeColumn:
            return m_dir->fileCreationTime(r);
        }

        return {};
    };

    switch (role)
    {
    case Qt::DisplayRole:
        return displayRole();
    case IconIDRole:
        if (c == NameColumn && m_iconProvider)
            return m_iconProvider(m_dir.get(), r);
        return {};
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
    case LastAccessTimeColumn:
        return "Last Access Time";
    case CreationTimeColumn:
        return "Creation Time";
    case ModifedTimeColumn:
        return "Modified Time";
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
        {IconIDRole, "iconId"}
    };
}

void DirectorySystemModel::setIconProvider(const IconProviderFunctor &newIconProvider)
{
    m_iconProvider = newIconProvider;
}
