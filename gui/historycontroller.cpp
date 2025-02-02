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

    auto sortModel = m_view->sortModel();

    connect(sortModel, &DirectorySortModel::randomSortChanged
            , this, &HistoryController::urlUpdated);

    connect(sortModel, &DirectorySortModel::randomSeedChanged
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

    qDebug() << "pop" << m_index << m_history.size();
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
    const auto viewUrl = m_view->url();
    if (viewUrl.isEmpty())
        return;

    const auto sortModel = m_view->sortModel();
    if (m_preferences)
    {
        m_preferences->setLastSessionUrl(viewUrl);
        m_preferences->setLastSessionRandomSort(sortModel->randomSort());
    }

    QUrl currentUrl;

    const auto stateMatches = [this, sortModel]()
    {
        const auto &cur = current();
        if ((cur.index() == 1) != sortModel->randomSort())
            return false;

        const bool sameUrl = std::visit([this](auto &&v) { return m_view->urlEx() == v.url; }, current());
        return sameUrl;
    };

    if (m_history.empty() || !stateMatches())
    {
        qDebug("adding new dir, '%s', randomSort: %d", qPrintable(viewUrl), sortModel->randomSort());
        if (!m_history.empty())
            qDebug("current dir, '%s', randomSort: %d", qPrintable(std::visit([](auto &&v) { return v.url.toString();}, current())), sortModel->randomSort());

        PointVariant var;
        if (sortModel->randomSort())
        {
            const auto h =  m_pathHistory->readForRandomSort(viewUrl);
            var = PointForRandomSort {viewUrl, h.row.value_or(0), h.col.value_or(0), h.randomSeed.value_or(-1)};
        }
        else
        {
            const auto h = m_pathHistory->read(viewUrl);
            var = Point {viewUrl, h.row.value_or(0), h.col.value_or(0), h.sortcolumn.value_or(-1), h.sortorder.value_or(-1)};
        }

        // new history point
        if (m_index + 1 != m_history.size())
            m_history.erase(m_history.begin() + m_index + 1, m_history.end());

        ++m_index;
        m_history.push_back(var);

        emit depthChanged();
        emitResetFocus(var);

        return;
    }

    // a previous url is updated, restore indexes
    const auto top = current();

    auto model = m_view->model();
    const auto index = std::visit([model](auto &&p){ return model->index(p.row, p.col);}, top);

    if (!model->checkIndex(index
                           , QAbstractItemModel::CheckIndexOption::IndexIsValid))
    {
        emit resetFocus(0, 0, -1, -1);
    }
    else
    {
        emitResetFocus(top);
    }
}

HistoryController::PointVariant &HistoryController::current()
{
    return m_history[m_index];
}

const HistoryController::PointVariant &HistoryController::current() const
{
    return m_history[m_index];
}

void HistoryController::setIndex(int index)
{
    m_index = index;
    emit depthChanged();

    // TODO: handle if load fail here
    const auto &cur = current();
    if (cur.index() == 0)
        m_view->openUrl(std::get<0>(cur).url);
    else
        m_view->openUrlWithRandomSort(std::get<1>(cur).url, std::get<1>(cur).randomSeed);
}

void HistoryController::emitResetFocus(const PointVariant &var)
{
    if (var.index() == 0)
    {
        const auto p = std::get<0>(var);
        emit resetFocus(p.row, p.col, p.sortcolumn, p.sortorder);
    }
    else
    {
        const auto p = std::get<1>(var);
        emit resetFocus(p.row, p.col, -1, -1);
    }
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
    if (current().index() == 0)
    {
        auto &top = std::get<0>(current());
        top.row =  index.row();
        top.col = index.column();
    }
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

    if (current().index() == 0)
    {
        auto &top = std::get<0>(current());
        top.sortcolumn = sortColumn;
        top.sortorder = sortOrder;
    }
}

void HistoryController::updateRandomSortParams(int row, int column, int randomSeed)
{
    qDebug() << "updateRandomSortParams" << row << column << randomSeed;
    if (m_history.empty())
        return; // no url has been pushed yet, why currentUpated is called then?

    if (m_pathHistory)
    {
        m_pathHistory->setRandomSortParams(m_view->url(), row, column, randomSeed);
    }

    if (current().index() == 1)
    {
        auto &top = std::get<1>(current());
        top.row = row;
        top.col = column;
        top.randomSeed = randomSeed;
    }
}

void HistoryController::restorePreviousSession()
{
    if (!m_history.empty()
            || !m_view
            || !m_preferences)
        return;

    auto url = m_preferences->lastSessionUrl();
    if (url.isEmpty())
    {
        auto home = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
        if (home.isEmpty())
            url = QDir(".").absolutePath();
        else
            url = home.first();
    }

    if (m_preferences->lastSessionRandomSort())
        m_view->openUrlWithRandomSort(url
                                      , m_pathHistory->readForRandomSort(url).randomSeed.value_or(0));
    else
        m_view->openUrl(url);
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
