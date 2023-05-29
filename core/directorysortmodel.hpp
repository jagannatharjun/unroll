#ifndef DIRECTORYSORTMODEL_HPP
#define DIRECTORYSORTMODEL_HPP

#include <QSortFilterProxyModel>

class DirectorySortModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(int sortOrder READ sortOrder NOTIFY sortParametersChanged)
    Q_PROPERTY(int sortColumn READ sortColumn NOTIFY sortParametersChanged)

public:
    explicit DirectorySortModel(QObject *parent = nullptr);

    void sort(int column, Qt::SortOrder order) override;

signals:
    void sortParametersChanged();

    // QSortFilterProxyModel interface
protected:
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const;
};

#endif // DIRECTORYSORTMODEL_HPP
