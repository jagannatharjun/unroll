#include "directorysortmodel.hpp"

#include "directorysystemmodel.hpp"

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
        const auto leftRandom = m_randomValues.value(source_left.row(), -1);
        const auto rightRandom = m_randomValues.value(source_right.row(), -1);
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

    if (m_randomSort
        && (m_randomValues.size() != sourceModel()->rowCount()))
        resetRandomValues();
}

void DirectorySortModel::resetRandomValues()
{
    m_randomValues.resize(sourceModel()->rowCount());

    srand(time(NULL));
    for (int &r: m_randomValues)
        r = rand();

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
        resetRandomValues();
}
