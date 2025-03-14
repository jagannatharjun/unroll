#ifndef HISTORYCONTROLLER_HPP
#define HISTORYCONTROLLER_HPP

#include <QObject>
#include <QQmlEngine>


class HistoryController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool canMoveBack READ canMoveBack NOTIFY depthChanged FINAL)
    Q_PROPERTY(bool canMoveForward READ canMoveForward NOTIFY depthChanged FINAL)

public:
    explicit HistoryController(QObject *parent = nullptr);

    int depth() const;

    bool canMoveForward() const;

    bool canMoveBack() const;

    QUrl currentUrl() const;

    int size() const { return m_history.size(); }

public slots:
    void pop();

    void forward();

    void pushUrl(const QUrl &url);

signals:
    void depthChanged();

    void preferencesChanged();

private:
    struct Point
    {
        QUrl url;
    };

    void setIndex(int index);

    Point &current();
    const Point &current() const;

    std::vector<Point> m_history;
    int m_index = - 1;
};

#endif // HISTORYCONTROLLER_HPP
