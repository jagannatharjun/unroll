#ifndef DIRECTORYOPENER_HPP
#define DIRECTORYOPENER_HPP

#include <QObject>
#include <Qfuture>

class Directory;
class DirectorySystem;;
class QThreadPool;

class DirectoryOpener : public QObject
{
    Q_OBJECT

public:
    explicit DirectoryOpener(std::shared_ptr<QThreadPool> pool
                             , std::shared_ptr<DirectorySystem> system
                             , QObject *parent = nullptr);

    std::shared_ptr<Directory> dir() const;

public slots:
    void openUrl(const QUrl &url);
    void openPath(const QString &path);
    void leanOpenPath(const QString &path);
    void openChild(const int child);
    void openParentPath();

signals:
    void directoryChanged();

private:
    void nextDir(QFuture<std::shared_ptr<Directory>> &&future);

    std::shared_ptr<QThreadPool> m_pool;
    std::shared_ptr<DirectorySystem> m_system;

    uintmax_t m_currentRequest = 0;
    std::shared_ptr<Directory> m_dir;
};

#endif // DIRECTORYOPENER_HPP
