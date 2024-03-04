#include "historycontroller.hpp"

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
    if (m_history.empty() || current().url != m_view->url())
    {
        // new history point
        if (m_index + 1 != m_history.size())
            m_history.erase(m_history.begin() + m_index + 1, m_history.end());

        ++m_index;
        m_history.push_back(Point {m_view->url(), -1, -1});
        emit depthChanged();
        emit resetFocus(0, 0);
        return;
    }

    // a previous url is updated, restore indexes
    const auto top = current();

    auto model = m_view->model();
    const auto index = model->index(top.row, top.col);

    if (!model->checkIndex(index))
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

    auto &top = current();
    top.row =  index.row();
    top.col = index.column();
}

bool HistoryController::canMoveBack() const
{
    return m_index > 0;
}
