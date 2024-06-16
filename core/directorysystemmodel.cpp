#include "directorysystemmodel.hpp"

#include "directorysystem.hpp"
#include "filehistorydb.hpp"

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
    case SeenRole:
        return getSeen(QPersistentModelIndex(index), m_dir->filePath(r));
    case ProgressRole:
        return getProgress(QPersistentModelIndex(index), m_dir->filePath(r));
    }

    return {};
}

bool DirectorySystemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!m_historyDB || !checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid))
        return false;

    const int r = index.row();
    if (m_dir->isDir(r))
        return false;

    if (role == SeenRole)
    {
        if (value.metaType() != QMetaType(QMetaType::Bool))
            return false;

        m_historyDB->setIsSeen(m_dir->filePath(r), value.toBool());
        m_data[m_dir->filePath(r)].seen = value.toBool();
        emit dataChanged(index, this->index(index.row(), ColumnCount - 1), {SeenRole});

    } else if (role == ProgressRole) {
        bool ok = false;
        const double progress = value.toDouble(&ok);
        if (!ok)
            return false;

        m_historyDB->setProgress(m_dir->filePath(r), progress);
        m_data[m_dir->filePath(r)].progress = progress;
        emit dataChanged(index, this->index(index.row(), ColumnCount - 1), {ProgressRole});
    }

    return false;
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
        {IconIDRole, "iconId"},
        {SeenRole, "seen"},
        {ProgressRole, "progress"}
    };
}

std::shared_ptr<FileHistoryDB> DirectorySystemModel::fileHistoryDB() const
{
    return m_historyDB;
}

void DirectorySystemModel::setFileHistoryDB(const std::shared_ptr<FileHistoryDB> &newHistoryDB)
{
    m_historyDB = newHistoryDB;
}

bool DirectorySystemModel::getSeen(const QPersistentModelIndex &idx, const QString &mrl) const
{
    if (!m_historyDB)
        return false;

    auto itr = m_data.find(mrl);
    if (itr == m_data.end() || !itr->seen)
    {
        m_historyDB->isSeen(mrl).then(const_cast<DirectorySystemModel *>(this)
                                      , [this, idx, mrl](bool seen) mutable
        {
            if (!idx.isValid())
                return;

            m_data[mrl].seen = seen;
            emit const_cast<DirectorySystemModel *>(this)->dataChanged(idx, index(idx.row(), ColumnCount - 1), {SeenRole});
        });
    }

    return itr != m_data.end() ? itr->seen.value() : false;
}

double DirectorySystemModel::getProgress(const QPersistentModelIndex &idx, const QString &mrl) const
{
    if (!m_historyDB)
        return -1;

    auto itr = m_data.find(mrl);
    if (itr == m_data.end() || !itr->seen)
    {
        m_historyDB->progress(mrl).then(const_cast<DirectorySystemModel *>(this)
                                        , [this, idx, mrl](double progress) mutable
        {
            if (!idx.isValid())
                return;

            m_data[mrl].progress = progress;
            emit const_cast<DirectorySystemModel *>(this)->dataChanged(idx, index(idx.row(), ColumnCount - 1), {ProgressRole});
        });
    }

    return itr != m_data.end() ? itr->progress.value() : -1;
}

void DirectorySystemModel::setIconProvider(const IconProviderFunctor &newIconProvider)
{
    m_iconProvider = newIconProvider;
}
