
#ifndef VIEWCONTROLLER_HPP
#define VIEWCONTROLLER_HPP

#include <memory>
#include <QObject>
#include <QFutureWatcher>
#include <QAbstractItemModel>

class QAbstractItemModel;
class DirectorySystemModel;
class DirectorySystem;
class Directory;

class AsyncSystemManager;

class ViewController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* model READ model CONSTANT)
    Q_PROPERTY(QString url READ url NOTIFY urlChanged)

public:
    ViewController(QObject *parent = nullptr);
    ~ViewController();

    QAbstractItemModel *model();
    QString url() const;


public slots:
    void openUrl(const QUrl &url);

    void clicked(int index);
    void doubleClicked(int index);

signals:
    void urlChanged();

private slots:
    void updateModel();

private:
    void openIndex(const int index);

    std::shared_ptr<Directory> validParent(const int index);

    std::shared_ptr<DirectorySystem> m_system;
    std::unique_ptr<DirectorySystemModel> m_model;
    QFutureWatcher<std::shared_ptr<Directory>> m_urlWatcher;
};

#endif // VIEWCONTROLLER_HPP
