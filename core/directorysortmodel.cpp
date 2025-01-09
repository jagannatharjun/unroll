#include "directorysortmodel.hpp"

#include "directorysystemmodel.hpp"

DirectorySortModel::DirectorySortModel(QObject *parent)
    : QSortFilterProxyModel{parent}
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);

    connect(this, &QSortFilterProxyModel::sourceModelChanged, this, [this]()
    {
        connect(sourceModel(), &QAbstractItemModel::modelReset
                , this, &DirectorySortModel::handleRandomValuesOnModelChange);
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
        const auto rowCount = sourceModel()->rowCount();
        if (m_randomValues.size() != rowCount)
        {
            resetRandomValues();
            return false;
        }

        const auto leftRandom = m_randomValues[source_left.row()];
        const auto rightRandom = m_randomValues[source_right.row()];
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
        resetRandomValues();
}

void DirectorySortModel::resetRandomValues() const
{
    m_randomValues.resize(sourceModel()->rowCount());

    srand(time(NULL));
    for (int &r: m_randomValues)
        r = rand();

    QMetaObject::invokeMethod(const_cast<DirectorySortModel *> (this)
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
