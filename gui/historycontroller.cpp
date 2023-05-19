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

    auto selectionModel = m_view->selectionModel();
    connect(selectionModel, &QItemSelectionModel::currentChanged
            , this, &HistoryController::currentUpdated);
}

void HistoryController::pushUrl(const QUrl &url)
{
    // try to open url first, if it succeded
    // controller.url will updated, and only then
    // history will be updated

    m_view->openUrl(url);
}

void HistoryController::pushRow(int row)
{
    m_view->openRow(row);
}

void HistoryController::pop()
{
    if (m_history.size() <= 1)
        return;

    m_history.pop();

    // TODO: handle if load fail here
    m_view->openUrl(m_history.top().url);
}

void HistoryController::urlUpdated()
{
    if (m_history.empty() || m_history.top().url != m_view->url())
    {
        // new history point
        m_history.push(Point {m_view->url(), -1, -1});
        emit resetFocus(0, 0);
        return;
    }

    // a previous url is updated, restore indexes
    const auto top = m_history.top();

    auto model = m_view->model();
    const auto index = model->index(top.row, top.col);

    auto selectionModel = m_view->selectionModel();
    if (!model->checkIndex(index))
    {
        selectionModel->clear();
        emit resetFocus(0, 0);
    }
    else
    {
        selectionModel->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
        emit resetFocus(index.row(), index.column());
    }
}

void HistoryController::currentUpdated()
{
    if (m_history.empty())
        return; // no url has been pushed yet, why currentUpated is called then?

    auto current = m_view->selectionModel()->currentIndex();
    const auto validCurrent = current.isValid();

    auto &top = m_history.top();
    top.row = validCurrent ? current.row() : - 1;
    top.col  = validCurrent ? current.column() : -1;
}
