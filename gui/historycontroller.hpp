#ifndef HISTORYCONTROLLER_HPP
#define HISTORYCONTROLLER_HPP

#include <QObject>
#include <QQmlEngine>

#include "viewcontroller.hpp"


class HistoryController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(ViewController* view READ view WRITE setView NOTIFY viewChanged)

    Q_PROPERTY(bool canMoveBack READ canMoveBack NOTIFY depthChanged FINAL)
    Q_PROPERTY(bool canMoveForward READ canMoveForward NOTIFY depthChanged FINAL)


public:
    explicit HistoryController(QObject *parent = nullptr);

    ViewController *view() const;
    void setView(ViewController *newView);

    int depth() const;

    bool canMoveForward() const;

    bool canMoveBack() const;

public slots:
    void pop();

    void forward();

    void updateCurrentIndex(int row, int column);

signals:
    void depthChanged();
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

    Point &current();
    const Point &current() const;

    void setIndex(int index);

    std::vector<Point> m_history;
    int m_index = - 1;

    ViewController *m_view = nullptr;
};

#endif // HISTORYCONTROLLER_HPP
