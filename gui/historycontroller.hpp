#ifndef HISTORYCONTROLLER_HPP
#define HISTORYCONTROLLER_HPP

#include <QObject>
#include <QQmlEngine>
#include <stack>

#include "viewcontroller.hpp"

class HistoryController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(ViewController* view READ view WRITE setView NOTIFY viewChanged)

public:
    explicit HistoryController(QObject *parent = nullptr);

    ViewController *view() const;
    void setView(ViewController *newView);

public slots:
    void pushUrl(const QUrl &url);
    void pushRow(int row);
    void pop();

    void updateCurrentIndex(int row, int column);

signals:
    void viewChanged();
    void resetFocus(int row, int column);

private slots:
    void urlUpdated();

private:
    struct Point
    {
        QUrl url;
        int row;
        int col;
    };

    std::stack<Point> m_history;

    ViewController *m_view = nullptr;
};

#endif // HISTORYCONTROLLER_HPP
