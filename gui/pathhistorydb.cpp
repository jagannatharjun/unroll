#include "pathhistorydb.hpp"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <qsqlerror.h>

namespace
{
template <typename Result>
std::optional<Result> get(QSqlQuery &q, int index)
{
    const QVariant val = q.value(index);
    if (!val.canConvert<Result>())
        return std::nullopt;

    return val.value<Result>();
}
}

PathHistoryDB::PathHistoryDB(const QString &dbPath, QObject *parent)
    : QObject{parent}
{
    m_db.reset(
        new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE"
                                                   , u"pathhistory_sqlite_connection"_qs)));

    m_db->setDatabaseName(dbPath);
    if (!m_db->open())
    {
        qFatal("failed to open database '%s'", qUtf8Printable(dbPath));
        return;
    }

    QSqlQuery q("CREATE TABLE IF NOT EXISTS path_history ("
                "   URL TEXT PRIMARY KEY,"
                "   ROW INT,"
                "   COL INT,"
                "   SORTCOLUMN INT"
                ")", *m_db);

    if (!q.exec())
    {
        qFatal() << "failed to create table path_history, error:" << q.lastError();
        return;
    }
}

PathHistoryDB::~PathHistoryDB() = default;

PathHistoryDB::HistoryData PathHistoryDB::read(const QString &url) const
{
    if (auto itr = m_cache.find(url); itr != m_cache.end())
        return itr.value();

    QSqlQuery q(*m_db);
    q.prepare(u"SELECT ROW, COL, SORTCOLUMN FROM path_history WHERE URL = :url"_qs);
    q.bindValue(u":url"_qs, url);

    if (!q.exec())
    {
        qFatal("failed to read for '%s', error: '%s'"
                    , qUtf8Printable(url)
                    , qUtf8Printable(q.lastError().text()));

        return {};
    }

    HistoryData r;
    if (q.next())
    {
        r.row = get<int>(q, 0);
        r.col = get<int>(q, 1);
        r.sortcolumn = get<int>(q, 2);
    }

    m_cache[url] = r;
    return r;
}

void PathHistoryDB::set(const QString &url, const HistoryData &data)
{
    QSqlQuery q(*m_db);
    q.prepare(u"INSERT OR REPLACE INTO path_history (URL, ROW, COL, SORTCOLUMN) "
              u"VALUES (:url, :row, :col, :sortcolumn)"_qs);

    q.bindValue(u":url"_qs, url);
    q.bindValue(u":row"_qs, data.row.value_or(0));
    q.bindValue(u":col"_qs, data.col.value_or(0));
    q.bindValue(u":sortcolumn"_qs, data.sortcolumn.value_or(0));

    if (!q.exec())
    {
        qFatal("failed to set data for '%s', error: '%s'",
               qUtf8Printable(url),
               qUtf8Printable(q.lastError().text()));

        return;
    }

    m_cache[url] = data;
}
