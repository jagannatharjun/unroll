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

class DirectorySystemModel::DBHandler
{
public:
    using UpdateCB = std::function<void(QPersistentModelIndex idx, QList<int> role)>;

    DBHandler(DirectorySystemModel *parent
              , std::shared_ptr<FileHistoryDB> db
              , UpdateCB cb)
        : m_parent {parent}
        , m_db {std::move(db)}
    {
        m_cb[0] = std::move(cb);
        m_cb[1] = [this](QPersistentModelIndex idx, QList<int> role)
        {
            return this->onFileSeen(idx, role);
        };
    }

    auto db() const { return m_db; }

    void clear() { m_data.clear(); m_seenCount.reset(); m_parentSeen.reset(); }

#define DBHandler_IMPL(TYPE, MEMBER, SETTER, ROLE, DEFAULT) \
    TYPE MEMBER(QPersistentModelIndex idx, const QString &mrl) \
    { \
        auto itr = m_data.find(mrl); \
        if (itr == m_data.end()) \
        { \
            read(idx, mrl); \
        } \
        return itr != m_data.end() && itr-> MEMBER.has_value() ? itr->MEMBER.value() : DEFAULT; \
    } \
    void SETTER(QPersistentModelIndex idx, const QString &mrl, TYPE value) \
    { \
        m_db->SETTER(mrl, value); \
        QList<int> r{ROLE, DirectorySystemModel::ShowNewIndicatorRole}; \
        if (ROLE == DirectorySystemModel::SeenRole && (value != m_data[mrl].MEMBER)) updateSeen(value); \
        m_data[mrl].MEMBER = value; \
        callUpdateCB(idx, r); \
    }

    DBHandler_IMPL(bool, seen, setSeen
                   , DirectorySystemModel::SeenRole, false)

    DBHandler_IMPL(double, progress, setProgress
                   , DirectorySystemModel::ProgressRole, 0)

    DBHandler_IMPL(bool, previewed, setPreviewed
                   , DirectorySystemModel::PreviewedRole, false)

    bool showNewIndicator(QPersistentModelIndex idx, const QString &mrl)
    {
        auto itr = m_data.find(mrl);
        if (itr == m_data.end())
        {
            read(idx, mrl);
            return false;
        }

        return !itr->seen.value_or(false)
                && !itr->previewed.value_or(false)
                && (itr->progress.value_or(0.0) < 0.02);
    }

private:
    void read(const QPersistentModelIndex &idx, const QString &mrl)
    {
        if (m_reading.contains(mrl)) return;

        m_reading.insert(mrl);
        m_db->read(mrl).then(m_parent, [this, mrl, idx](const FileHistoryDB::Data &data)
        {
            m_reading.remove(mrl);
            if (!idx.isValid()) return;

            m_data[mrl] = data;

            // only notify if something was changed, QML views doesn't like us otherwise
            QList<int> changed({DirectorySystemModel::ShowNewIndicatorRole});

            if (data.seen)
                changed.push_back(DirectorySystemModel::SeenRole);

            if (data.progress)
                changed.push_back(DirectorySystemModel::ProgressRole);

            if (data.previewed)
                changed.push_back(DirectorySystemModel::PreviewedRole);

            if (changed.empty())
                return;

            callUpdateCB(idx, changed);
        });
    }

    void onFileSeen(QPersistentModelIndex idx, QList<int> role)
    {
        const auto fileCount = m_parent->directory()->fileCount();
        const bool dataForAllAvailable = (m_data.size() == fileCount);
        if (dataForAllAvailable)
        {
            m_seenCount = std::count_if(m_data.begin(), m_data.end(), [](auto d) { return d.seen; });

            updateParentSeen();
        }
    }

    void callUpdateCB(const QPersistentModelIndex &idx, const QList<int> &role)
    {
        for (const auto &cb : m_cb)
        {
            cb(idx, role);
        }
    }

    // should only be called when data is changed
    void updateSeen(bool seen)
    {
        if (!m_seenCount.has_value())
            return;

        if (seen)
            ++m_seenCount.value();
        else
            --m_seenCount.value();

        updateParentSeen();
    }

    // should only be called when data is changed
    void updateParentSeen()
    {
        assert(m_seenCount.has_value());

        auto dir = m_parent->directory();
        const bool seen = (m_seenCount == dir->fileCount());

        if (!m_parentSeen.has_value() || m_parentSeen != seen)
        {
            m_parentSeen = seen;
            m_db->setSeen(dir->path(), seen);

            qDebug() << "updating" << dir->path() << seen;
        }
    }

    DirectorySystemModel *m_parent;
    std::shared_ptr<FileHistoryDB> m_db;
    UpdateCB m_cb[2];
    QHash<QString, FileHistoryDB::Data> m_data;
    QSet<QString> m_reading;

    std::optional<std::intmax_t> m_seenCount;
    std::optional<bool> m_parentSeen;
};

DirectorySystemModel::DirectorySystemModel(QObject *parent)
    : QAbstractTableModel{parent}
{
}

void DirectorySystemModel::setDirectory(std::shared_ptr<Directory> dir)
{
    beginResetModel();

    m_dir = dir;
    if (m_dbHandler)
        m_dbHandler->clear();

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
    case ProgressRole:
    case PreviewedRole:
    case ShowNewIndicatorRole:
    {
        if (m_dir->isDir(r)
            && (role != SeenRole))
            return {};

        switch (role)
        {
        case SeenRole:
            return m_dbHandler->seen(QPersistentModelIndex(index), m_dir->filePath(r));
        case ProgressRole:
            return m_dbHandler->progress(QPersistentModelIndex(index), m_dir->filePath(r));
        case PreviewedRole:
            return m_dbHandler->previewed(QPersistentModelIndex(index), m_dir->filePath(r));
        case ShowNewIndicatorRole:
            return m_dbHandler->showNewIndicator(QPersistentModelIndex(index), m_dir->filePath(r));
        }
    }
    }

    return {};
}

bool DirectorySystemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!m_dbHandler || !checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid))
        return false;

    const int r = index.row();
    qDebug() << "setData" << index.data(PathRole) << (Roles)role << value;
    if (m_dir->isDir(r))
        return false;

    if (role == SeenRole)
    {
        if (value.metaType() != QMetaType(QMetaType::Bool))
            return false;

        m_dbHandler->setSeen(index, m_dir->filePath(r), value.toBool());
    }
    else if (role == ProgressRole)
    {
        bool ok = false;
        const double progress = value.toDouble(&ok);
        if (!ok)
            return false;

        m_dbHandler->setProgress(index, m_dir->filePath(r), progress);
    }
    else if (role == PreviewedRole)
    {
        if (value.metaType() != QMetaType(QMetaType::Bool))
            return false;

        m_dbHandler->setPreviewed(index, m_dir->filePath(r), value.toBool());
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
        {ProgressRole, "progress"},
        {PreviewedRole, "previewed"},
        {ShowNewIndicatorRole, "showNewIndicator"}
    };
}

std::shared_ptr<FileHistoryDB> DirectorySystemModel::fileHistoryDB() const
{
    return m_dbHandler ? m_dbHandler->db() : nullptr;
}

void DirectorySystemModel::setFileHistoryDB(const std::shared_ptr<FileHistoryDB> &newHistoryDB)
{
    const auto updateHandler = [this](const QPersistentModelIndex &idx, QList<int> role)
    {
        emit dataChanged(index(idx.row(), 0)
                         , index(idx.row(), ColumnCount - 1), role);
    };

    m_dbHandler.reset(new DBHandler(this, newHistoryDB, updateHandler));
}

void DirectorySystemModel::setIconProvider(const IconProviderFunctor &newIconProvider)
{
    m_iconProvider = newIconProvider;
}
