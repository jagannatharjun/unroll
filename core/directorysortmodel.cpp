#include "directorysortmodel.hpp"

#include "directorysystemmodel.hpp"
#include "filetype.hpp"
#include <QDateTime>

DirectorySortModel::DirectorySortModel(QObject *parent)
    : QSortFilterProxyModel{parent}
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);

    connect(this, &QSortFilterProxyModel::sourceModelChanged, this, [this]()
    {
        const auto src = sourceModel();
        const auto triggerOn = [&](auto signal)
        {
            connect(src, signal
                    , this, &DirectorySortModel::handleRandomValuesOnModelChange);
        };

        triggerOn(&QAbstractItemModel::modelReset);
        triggerOn(&QAbstractItemModel::layoutChanged);
        triggerOn(&QAbstractItemModel::rowsInserted);
        triggerOn(&QAbstractItemModel::rowsMoved);
        triggerOn(&QAbstractItemModel::rowsRemoved);
    });
}

void DirectorySortModel::sort(int column, Qt::SortOrder order)
{
    setRandomSort(false);

    QSortFilterProxyModel::sort(column, order);
    emit sortParametersChanged();
}

bool DirectorySortModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    if (m_randomSort)
    {
        // Combine seed with the row number to generate a random value
        const auto leftRandom = qHash(m_randomSeed ^ source_left.row());
        const auto rightRandom = qHash(m_randomSeed ^ source_right.row());

        return leftRandom < rightRandom;
    }

    if (sortColumn() == DirectorySystemModel::NameColumn)
    {
        const auto leftDir = source_left.data(DirectorySystemModel::IsDirRole).toBool();
        const auto rightDir = source_right.data(DirectorySystemModel::IsDirRole).toBool();
        if (leftDir != rightDir)
        {
            return sortOrder() == Qt::AscendingOrder ? leftDir : rightDir;
        }
    }

    return QSortFilterProxyModel::lessThan(source_left, source_right);
}

void DirectorySortModel::handleRandomValuesOnModelChange()
{
    if (sender() != sourceModel())
        return;

    if (m_randomSort)
        resetRandomSeed();
}

bool DirectorySortModel::onlyShowVideoFile() const
{
    return m_onlyShowVideoFile;
}

void DirectorySortModel::setOnlyShowVideoFile(bool newOnlyShowVideoFile)
{
    if (m_onlyShowVideoFile == newOnlyShowVideoFile)
        return;
    m_onlyShowVideoFile = newOnlyShowVideoFile;
    invalidate();
    emit onlyShowVideoFileChanged();
}

bool DirectorySortModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    const auto src = sourceModel();
    const auto path = src->data(src->index(source_row, 0), DirectorySystemModel::PathRole).toString();
    if (path.endsWith(".!qB"))
        return false;

    if (m_onlyShowVideoFile
        && (FileType::findType(path) != FileType::VideoFile))
        return false;

    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

void DirectorySortModel::resetRandomSeed()
{
    m_randomSeed = static_cast<quint32>(QDateTime::currentMSecsSinceEpoch() & 0xFFFFFFFF /* convert number to int32*/);

    QMetaObject::invokeMethod(this
                              , &DirectorySortModel::invalidate
                              , Qt::QueuedConnection);
}

void DirectorySortModel::setRandomSort(bool newRandomSort)
{
    if (m_randomSort == newRandomSort)
        return;

    m_randomSort = newRandomSort;
    emit randomSortChanged();

    if (m_randomSort)
        resetRandomSeed();
}
