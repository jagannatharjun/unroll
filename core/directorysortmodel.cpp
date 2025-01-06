#include "directorysortmodel.hpp"

#include "directorysystemmodel.hpp"

DirectorySortModel::DirectorySortModel(QObject *parent)
    : QSortFilterProxyModel{parent}
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
}

void DirectorySortModel::sort(int column, Qt::SortOrder order)
{
    m_randomSort = false;
    QSortFilterProxyModel::sort(column, order);
    emit sortParametersChanged();
}

void DirectorySortModel::randomSort()
{
    m_randomSort = true;
    resetRandomValues();
}

bool DirectorySortModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    if (m_randomSort)
    {
        const auto rowCount = sourceModel()->rowCount();
        if (m_randomValues.size() != rowCount)
        {
            resetRandomValues();
            return false;
        }

        auto &leftRandom = m_randomValues[source_left.row()];
        auto &rightRandom = m_randomValues[source_right.row()];
        if (leftRandom == -1) leftRandom = rand();
        if (rightRandom == -1) rightRandom = rand();
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

void DirectorySortModel::resetRandomValues() const
{
    m_randomValues.resize(sourceModel()->rowCount());
    m_randomValues.fill(-1);

    QMetaObject::invokeMethod(const_cast<DirectorySortModel *> (this)
                              , &DirectorySortModel::invalidate
                              , Qt::QueuedConnection);
}
