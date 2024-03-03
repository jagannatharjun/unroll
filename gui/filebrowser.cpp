#include "filebrowser.hpp"

#include <QWindow>
#include <QStorageInfo>

FileBrowser::FileBrowser(QObject *parent)
    : QObject{parent}
{

}

QWindow *FileBrowser::window() const
{
    return m_window;
}

void FileBrowser::setWindow(QWindow *newWindow)
{
    if (m_window == newWindow)
        return;

    m_window = newWindow;
    emit windowChanged();
}

void FileBrowser::setCursor(Qt::CursorShape cursorShape)
{
    if (!m_window)
    {
        qWarning("setCursor failed, windows is not set");
        return;
    }

    m_window->setCursor(cursorShape); }

QString FileBrowser::volumeName(const QString &path) const
{
    return QStorageInfo(path).name();
}
