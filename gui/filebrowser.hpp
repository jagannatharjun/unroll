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

signals:
    void windowChanged();
private:
    QWindow *m_window;
};

#endif // FILEBROWSER_HPP
