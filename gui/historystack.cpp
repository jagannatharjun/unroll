#include "historystack.hpp"

#include <QStandardPaths>
#include <QDir>

HistoryStack::HistoryStack(QObject *parent)
    : QObject{parent}
{
}

int HistoryStack::depth() const
{
    return m_index + 1;
}

bool HistoryStack::canMoveForward() const
{
    return m_index + 1 < m_history.size();
}

void HistoryStack::pop()
{
    if (!canMoveBack())
        return;

    setIndex(m_index - 1);
}

void HistoryStack::forward()
{
    if (!canMoveForward())
        return;

    setIndex(m_index + 1);
}

void HistoryStack::pushUrl(const QUrl &url)
{
    if (m_index + 1 != m_history.size())
        m_history.erase(m_history.begin() + m_index + 1, m_history.end());

    m_history.push_back(Point {url});
    setIndex(m_index + 1);
}

HistoryStack::Point &HistoryStack::current()
{
    return m_history[m_index];
}

const HistoryStack::Point &HistoryStack::current() const
{
    return m_history[m_index];
}

void HistoryStack::setIndex(int index)
{
    m_index = index;
    emit depthChanged();
}

bool HistoryStack::canMoveBack() const
{
    return m_index > 0;
}

QUrl HistoryStack::currentUrl() const
{
    return current().url;
}


