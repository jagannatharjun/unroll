#ifndef DIRECTORYSORTMODEL_HPP
#define DIRECTORYSORTMODEL_HPP

#include <QSortFilterProxyModel>

class DirectorySortModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(int sortOrder READ sortOrder NOTIFY sortParametersChanged)
    Q_PROPERTY(int sortColumn READ sortColumn NOTIFY sortParametersChanged)

    Q_PROPERTY(bool onlyShowVideoFile READ onlyShowVideoFile WRITE setOnlyShowVideoFile NOTIFY onlyShowVideoFileChanged FINAL)

    Q_PROPERTY(bool randomSort
                   READ randomSort
                       WRITE setRandomSort
                           NOTIFY randomSortChanged)

public:
    explicit DirectorySortModel(QObject *parent = nullptr);

    void sort(int column, Qt::SortOrder order) override;

    void setRandomSort(bool newRandomSort);
    void setRandomSortEx(bool newRandomSort, int32_t randomSeed);
    bool randomSort() const { return m_randomSort; }
    int32_t randomSeed() const { return m_randomSeed; }

    Q_INVOKABLE void resetRandomSeed();

    bool onlyShowVideoFile() const;
    void setOnlyShowVideoFile(bool newOnlyShowVideoFile);

signals:
    void sortParametersChanged();
    void randomSortChanged();

    // QSortFilterProxyModel interface
    void onlyShowVideoFileChanged();

protected:
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const;

private:
    void handleRandomValuesOnModelChange();

    bool m_randomSort = false;
    int32_t m_randomSeed = 0x1234; // Single seed value
    bool m_onlyShowVideoFile = false;


protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
};

#endif // DIRECTORYSORTMODEL_HPP
