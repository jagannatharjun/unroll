#ifndef DIRECTORYSYSTEMMODEL_HPP
#define DIRECTORYSYSTEMMODEL_HPP

#include <QAbstractListModel>
#include <memory>

class Directory;

class DirectorySystemModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles
    {
        NameRole = Qt::UserRole + 1,
        PathRole,
        SizeRole,
        IsDirRole
    };

    explicit DirectorySystemModel(QObject *parent = nullptr);
    ~DirectorySystemModel();

    void setDirectory(std::shared_ptr<Directory> dir);
    std::shared_ptr<Directory> directory();

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;

private:
    std::shared_ptr<Directory> m_dir;
};

#endif // DIRECTORYSYSTEMMODEL_HPP
