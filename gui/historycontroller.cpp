#include "historycontroller.hpp"

#include <QStandardPaths>
#include <QDir>

HistoryController::HistoryController(QObject *parent)
    : QObject{parent}
{
}

int HistoryController::depth() const
{
    return m_index + 1;
}

bool HistoryController::canMoveForward() const
{
    return m_index + 1 < m_history.size();
}

void HistoryController::pop()
{
    if (!canMoveBack())
        return;

    setIndex(m_index - 1);
}

void HistoryController::forward()
{
    if (!canMoveForward())
        return;

    setIndex(m_index + 1);
}

void HistoryController::pushUrl(const QUrl &url)
{
    if (m_index + 1 != m_history.size())
        m_history.erase(m_history.begin() + m_index + 1, m_history.end());

    m_history.push_back(Point {url});
    setIndex(m_index + 1);
}

HistoryController::Point &HistoryController::current()
{
    return m_history[m_index];
}

const HistoryController::Point &HistoryController::current() const
{
    return m_history[m_index];
}

void HistoryController::setIndex(int index)
{
    m_index = index;
    emit depthChanged();
}

bool HistoryController::canMoveBack() const
{
    return m_index > 0;
}

QUrl HistoryController::currentUrl() const
{
    return current().url;
}


