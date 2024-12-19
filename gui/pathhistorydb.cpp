#include "pathhistorydb.hpp"

#include "../core/dbutil.hpp"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <qsqlerror.h>

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
                "   SORTCOLUMN INT,"
                "   SORTORDER INT"
                ")", *m_db);

    if (!q.exec())
    {
        qFatal() << "failed to create table path_history, error:" << q.lastError();
        return;
    }

    enableAutoVacuum(*m_db);
}

PathHistoryDB::~PathHistoryDB() = default;

PathHistoryDB::HistoryData PathHistoryDB::read(const QString &url) const
{
    if (auto itr = m_cache.find(url); itr != m_cache.end())
        return itr.value();

    QSqlQuery q(*m_db);
    q.prepare(u"SELECT ROW, COL, SORTCOLUMN, SORTORDER "
              u"FROM path_history WHERE URL = :url"_qs);
    q.bindValue(u":url"_qs, url);

    if (!q.exec())
    {
        qFatal("PathHistoryDB::read, failed to read for '%s', error: '%s'"
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
        r.sortorder = get<int>(q, 3);
    }

    m_cache[url] = r;
    return r;
}

void PathHistoryDB::setRowAndColumn(const QString &url
                                    , const int row
                                    , const int col)
{
    QSqlQuery q(*m_db);
    q.prepare(u"INSERT INTO path_history (URL, ROW, COL) "
              u"VALUES (:url, :row, :col) "
              u"ON CONFLICT(URL) DO UPDATE SET "
              u"ROW = excluded.ROW, COL = excluded.COL"_qs);

    q.bindValue(u":url"_qs, url);
    q.bindValue(u":row"_qs, row);
    q.bindValue(u":col"_qs, col);

    if (!q.exec())
    {
        qFatal("failed to update ROW and COL for '%s', error: '%s'",
               qUtf8Printable(url),
               qUtf8Printable(q.lastError().text()));
    }

    // update cache with new values
    auto data = m_cache[url];
    data.row = row;
    data.col = col;
    m_cache[url] = data;
}

void PathHistoryDB::setSortParams(const QString &url, int sortcolumn, int sortorder)
{
    QSqlQuery q(*m_db);
    q.prepare(u"INSERT INTO path_history (URL, SORTCOLUMN, SORTORDER) "
              u"VALUES (:url, :sortcolumn, :sortorder) "
              u"ON CONFLICT(URL) DO UPDATE SET "
              u"SORTCOLUMN = excluded.SORTCOLUMN, SORTORDER = excluded.SORTORDER"_qs);

    q.bindValue(u":url"_qs, url);
    q.bindValue(u":sortcolumn"_qs, sortcolumn);
    q.bindValue(u":sortorder"_qs, sortorder);

    if (!q.exec())
    {
        qFatal("failed to update SORTCOLUMN and SORTORDER for '%s', error: '%s'",
               qUtf8Printable(url),
               qUtf8Printable(q.lastError().text()));
    }

    // update cache with new values
    auto data = m_cache[url];
    data.sortcolumn = sortcolumn;
    data.sortorder = sortorder;
    m_cache[url] = data;
}
