#ifndef FILEBROWSER_HPP
#define FILEBROWSER_HPP

#include <QDir>
#include <QMediaPlayer>
#include <QObject>
#include <QWindow>

class PreviewData;

class FileBrowser : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QWindow* window READ window WRITE setWindow NOTIFY windowChanged)

public:
    explicit FileBrowser(QObject *parent = nullptr);

    QWindow *window() const;
    void setWindow(QWindow *newWindow);

    Q_INVOKABLE void setCursor(Qt::CursorShape cursorShape);

    Q_INVOKABLE QString volumeName(const QString &path) const;

    QDir cacheDir() const;

    QDir appDataPath() const;

    QString fileHistoryDBPath() const;

    QString pathHistoryDBPath() const;

    Q_INVOKABLE bool isContainer(const QString &path) const;

    Q_INVOKABLE void showFileContextMenu(const QPoint &p
                                         , const QString &filePath);

    Q_INVOKABLE bool setMediaSource(QMediaPlayer *player, const PreviewData &data);

signals:
    void windowChanged();

    void openFolder(const QString &folder);

private:
    QString dbPath(const QString &fileName) const;

    QWindow *m_window;
};

#endif // FILEBROWSER_HPP
