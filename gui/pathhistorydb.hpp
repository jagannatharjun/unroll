#ifndef PATHHISTORYDB_HPP
#define PATHHISTORYDB_HPP

#include <QObject>
#include <QHash>

class QSqlDatabase;

class PathHistoryDB : public QObject
{
    Q_OBJECT
public:
    struct HistoryData
    {
        std::optional<int> row, col;
        std::optional<int> sortcolumn;
    };

    explicit PathHistoryDB(const QString &dbPath
                           , QObject *parent = nullptr);

    ~PathHistoryDB();

    HistoryData read(const QString &url) const;
    void set(const QString &url, const HistoryData &data);

signals:

private:
    std::unique_ptr<QSqlDatabase> m_db;
    mutable QHash<QString, HistoryData> m_cache;
};

#endif // PATHHISTORYDB_HPP
