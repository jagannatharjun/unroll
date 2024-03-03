#ifndef FILEBROWSER_HPP
#define FILEBROWSER_HPP

#include <QObject>
#include <QWindow>

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

signals:
    void windowChanged();
private:
    QWindow *m_window;
};

#endif // FILEBROWSER_HPP
