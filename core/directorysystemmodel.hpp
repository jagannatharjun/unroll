#ifndef DIRECTORYSYSTEMMODEL_HPP
#define DIRECTORYSYSTEMMODEL_HPP

#include <QAbstractListModel>
#include <QFutureWatcher>

#include "asyncdirsystem.hpp"

class DirectorySystemModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
public:
    enum Roles
    {
        NameRole = Qt::UserRole + 1,
        PathRole,
        SizeRole,
        IsDirRole
    };

    explicit DirectorySystemModel(QObject *parent = nullptr);

    bool isLoading() const;

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;

public slots:
    void open(const QUrl &url);
    void openindex(int index);

signals:
    void isLoadingChanged();

private slots:
    void handleLoadingFinished();

private:
    using DirResult = std::shared_ptr<Directory>;

    AsyncDirSystem m_loader;
    DirResult m_dir;
    QFutureWatcher<DirResult> m_watcher;
};

#endif // DIRECTORYSYSTEMMODEL_HPP
