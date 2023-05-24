#ifndef DIRECTORYSYSTEMMODEL_HPP
#define DIRECTORYSYSTEMMODEL_HPP

#include <QAbstractListModel>
#include <memory>

class Directory;

class DirectorySystemModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Roles
    {
        NameRole = Qt::UserRole + 1,
        PathRole,
        SizeRole,
        IsDirRole,

        // Meta Roles
        DataRole
    };

    Q_ENUM(Roles);

    enum Columns
    {
        NameColumn,
        PathColumn,
        SizeColumn,

        ColumnCount
    };

    explicit DirectorySystemModel(QObject *parent = nullptr);
    ~DirectorySystemModel();

    void setDirectory(std::shared_ptr<Directory> dir);
    std::shared_ptr<Directory> directory();

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QHash<int, QByteArray> roleNames() const;

private:
    std::shared_ptr<Directory> m_dir;

};

#endif // DIRECTORYSYSTEMMODEL_HPP
