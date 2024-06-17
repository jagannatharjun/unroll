#include "historycontroller.hpp"

#include <QStandardPaths>
#include <QDir>

HistoryController::HistoryController(QObject *parent)
    : QObject{parent}
{

}

ViewController *HistoryController::view() const
{
    return m_view;
}

void HistoryController::setView(ViewController *newView)
{
    if (m_view == newView)
        return;

    if (m_view)
        m_view->disconnect(this);

    m_view = newView;
    emit viewChanged();

    connect(m_view, &ViewController::urlChanged
            , this, &HistoryController::urlUpdated);

    restorePreviousSession();
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


void HistoryController::urlUpdated()
{
    if (m_preferences)
    {
        m_preferences->setLastSessionUrl(m_view->url());
    }

    if (m_history.empty() || current().url != m_view->url())
    {
        // new history point
        if (m_index + 1 != m_history.size())
            m_history.erase(m_history.begin() + m_index + 1, m_history.end());

        QVector<int> lastIdx;
        if (m_preferences)
        {
            lastIdx = m_preferences->urlLastIndex(m_view->url());
        }

        if (lastIdx.size() != 2)
            lastIdx = {0, 0};

        ++m_index;
        m_history.push_back(Point {m_view->url(), lastIdx[0], lastIdx[1]});

        emit depthChanged();
        emit resetFocus(lastIdx[0], lastIdx[1]);

        return;
    }

    // a previous url is updated, restore indexes
    const auto top = current();

    auto model = m_view->model();
    const auto index = model->index(top.row, top.col);

    if (!model->checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid))
    {
        emit resetFocus(0, 0);
    }
    else
    {
        emit resetFocus(index.row(), index.column());
    }
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

    // TODO: handle if load fail here
    m_view->openUrl(current().url);
}

void HistoryController::updateCurrentIndex(int row, int column)
{
    if (m_history.empty())
        return; // no url has been pushed yet, why currentUpated is called then?

    auto index = m_view->model()->index(row, column);
    if (!index.isValid())
    {
        qDebug("invalid index in updateCurrentIndex");
        return;
    }

    if (m_preferences)
    {
        m_preferences->setUrlLastIndex(m_view->url(), {row, column});
    }

    auto &top = current();
    top.row =  index.row();
    top.col = index.column();
}

void HistoryController::restorePreviousSession()
{
    if (!m_history.empty()
            || !m_view
            || !m_preferences)
        return;

    ++m_index;

    auto url = m_preferences->lastSessionUrl();
    if (url.isEmpty())
    {
        auto home = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
        if (home.isEmpty())
            url = QDir(".").absolutePath();
        else
            url = home.first();
    }

    auto lastIdx = m_preferences->urlLastIndex(url);
    if (lastIdx.empty())
        lastIdx = {-1, -1};

    m_history.push_back(Point {url, lastIdx[0], lastIdx[1]});
    m_view->openUrl(url);

    emit depthChanged();
}

bool HistoryController::canMoveBack() const
{
    return m_index > 0;
}

Preferences *HistoryController::pref() const
{
    return m_preferences;
}

void HistoryController::setPref(Preferences *newPreferences)
{
    if (m_preferences == newPreferences)
        return;

    m_preferences = newPreferences;
    emit preferencesChanged();
    restorePreviousSession();
}
