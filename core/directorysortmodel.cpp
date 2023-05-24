#include "directorysortmodel.hpp"

DirectorySortModel::DirectorySortModel(QObject *parent)
    : QSortFilterProxyModel{parent}
{

}

void DirectorySortModel::sort(int column, Qt::SortOrder order)
{
    QSortFilterProxyModel::sort(column, order);
    emit sortParametersChanged();
}
