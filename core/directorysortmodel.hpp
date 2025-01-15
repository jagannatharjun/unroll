#ifndef DIRECTORYSORTMODEL_HPP
#define DIRECTORYSORTMODEL_HPP

#include <QSortFilterProxyModel>

class DirectorySortModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(int sortOrder READ sortOrder NOTIFY sortParametersChanged)
    Q_PROPERTY(int sortColumn READ sortColumn NOTIFY sortParametersChanged)

    Q_PROPERTY(bool randomSort
                   READ randomSort
                       WRITE setRandomSort
                           NOTIFY randomSortChanged)

public:
    explicit DirectorySortModel(QObject *parent = nullptr);

    void sort(int column, Qt::SortOrder order) override;

    void setRandomSort(bool newRandomSort);
    bool randomSort() const { return m_randomSort; }

    Q_INVOKABLE void resetRandomValues();

signals:
    void sortParametersChanged();
    void randomSortChanged();

    // QSortFilterProxyModel interface
protected:
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const;

private:
    void handleRandomValuesOnModelChange();

    bool m_randomSort = false;
    QVector<int> m_randomValues;
};

#endif // DIRECTORYSORTMODEL_HPP
