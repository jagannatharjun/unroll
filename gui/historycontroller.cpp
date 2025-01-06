#include "historycontroller.hpp"

#include <QStandardPaths>
#include <QDir>
#include "pathhistorydb.hpp"

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

    const auto dbPath = m_view->fileBrowser()->pathHistoryDBPath();
    m_pathHistory = std::make_unique<PathHistoryDB>(dbPath);

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

    if (m_history.empty()
        || (current().url != m_view->url()))
    {
        qDebug("adding new dir, '%s'", qPrintable(m_view->url()));
        // new history point
        if (m_index + 1 != m_history.size())
            m_history.erase(m_history.begin() + m_index + 1, m_history.end());

        auto [row, col] = lastRowAndColumn(m_view->url());
        if (row == -1)
            row = 0;

        if (col == -1)
            col = 0;

        const auto [sortCol, sortOrder] = lastSortParams(m_view->url());

        ++m_index;
        m_history.push_back(
            Point {
              m_view->url()
            , row
            , col
            , sortCol
            , sortOrder});

        emit depthChanged();
        emit resetFocus(row, col, sortCol, sortOrder);

        return;
    }

    // a previous url is updated, restore indexes
    const auto top = current();

    auto model = m_view->model();
    const auto index = model->index(top.row, top.col);

    if (!model->checkIndex(index
                           , QAbstractItemModel::CheckIndexOption::IndexIsValid))
    {
        emit resetFocus(0, 0, -1, -1);
    }
    else
    {
        emit resetFocus(index.row(), index.column()
                        , top.sortcolumn, top.sortorder);
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

std::pair<int, int> HistoryController::lastRowAndColumn(const QString &url)
{
    auto historyData = m_pathHistory->read(url);
    const int lastRow = historyData.row.value_or(-1);
    const int lastCol = historyData.col.value_or(-1);
    return {lastRow, lastCol};
}

std::pair<int, int> HistoryController::lastSortParams(const QString &url)
{
    auto historyData = m_pathHistory->read(url);
    const int lastSortCol = historyData.sortcolumn.value_or(-1);
    const int lastSortOrder = historyData.sortorder.value_or(-1);
    return {lastSortCol, lastSortOrder};
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

    if (m_pathHistory)
    {
        m_pathHistory->setRowAndColumn(m_view->url(), row, column);
    }

    // update values of top
    auto &top = current();
    top.row =  index.row();
    top.col = index.column();
}

void HistoryController::updateSortParams(int sortColumn, int sortOrder)
{
    qDebug() << "sort paramupdate" << sortColumn << sortOrder;
    if (m_history.empty())
        return; // no url has been pushed yet, why currentUpated is called then?

    if (m_pathHistory)
    {
        m_pathHistory->setSortParams(m_view->url(), sortColumn, sortOrder);
    }

    // update values of top
    auto &top = current();
    top.sortcolumn = sortColumn;
    top.sortorder = sortOrder;
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

    const auto [row, col] = lastRowAndColumn(url);
    const auto [sortcol, sortorder] = lastSortParams(url);

    m_history.push_back(Point {url, row, col, sortcol, sortorder});
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
