#ifndef HISTORYCONTROLLER_HPP
#define HISTORYCONTROLLER_HPP

#include <QObject>
#include <QQmlEngine>

#include "viewcontroller.hpp"
#include "preferences.hpp"

class PathHistoryDB;

class HistoryController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(ViewController* view READ view WRITE setView NOTIFY viewChanged)
    Q_PROPERTY(Preferences* preferences READ pref WRITE setPref NOTIFY preferencesChanged FINAL)

    Q_PROPERTY(bool canMoveBack READ canMoveBack NOTIFY depthChanged FINAL)
    Q_PROPERTY(bool canMoveForward READ canMoveForward NOTIFY depthChanged FINAL)


public:
    explicit HistoryController(QObject *parent = nullptr);

    ViewController *view() const;
    void setView(ViewController *newView);

    int depth() const;

    bool canMoveForward() const;

    bool canMoveBack() const;

    Preferences *pref() const;
    void setPref(Preferences *newPreferences);

public slots:
    void pop();

    void forward();

    Q_INVOKABLE void updateCurrentIndex(int row, int column);

    Q_INVOKABLE void updateSortParams(int sortColumn, int sortOrder);

    void restorePreviousSession();

signals:
    void depthChanged();
    void viewChanged();
    void resetFocus(int row, int column
                    , int sortOder, int sortColumn);

    void resetFocusRandomSort(int row, int column
                                , int sortOder, int sortColumn);

    void fileBrowserChanged();

    void preferencesChanged();

private slots:
    void onViewLoadingChanged();

private:
    struct Point
    {
        QUrl url;
        int row;
        int col;
        int sortcolumn;
        int sortorder;
    };

    Point &current();
    const Point &current() const;

    void setIndex(int index);
    std::pair<int, int> lastRowAndColumn(const QString &url);
    std::pair<int, int> lastSortParams(const QString &url);

    std::unique_ptr<PathHistoryDB> m_pathHistory;
    std::vector<Point> m_history;
    int m_index = - 1;

    ViewController *m_view = nullptr;
    Preferences *m_preferences = nullptr;
};

#endif // HISTORYCONTROLLER_HPP
