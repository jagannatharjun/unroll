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

    {
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
    }

    {
        QSqlQuery q2("CREATE TABLE IF NOT EXISTS random_sort_path_history ("
                     "   URL TEXT PRIMARY KEY,"
                     "   ROW INT,"
                     "   COL INT,"
                     "   RANDOMSEED INT"
                     ")", *m_db);

        if (!q2.exec())
        {
            qFatal() << "failed to create table random_sort_path_history, error:" << q2.lastError();
            return;
        }
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

void PathHistoryDB::updateColumns(const QString &url, const QMap<QString, QVariant> &columns)
{
    QStringList columnAssignments;
    for (auto it = columns.begin(); it != columns.end(); ++it)
    {
        columnAssignments.append(QStringLiteral("%1 = excluded.%1").arg(it.key()));
    }

    QString queryStr = QStringLiteral(
                           "INSERT INTO path_history (URL, %1) "
                           "VALUES (:url, %2) "
                           "ON CONFLICT(URL) DO UPDATE SET %3")
                           .arg(columns.keys().join(", "))
                           .arg(":" + columns.keys().join(", :"))
                           .arg(columnAssignments.join(", "));

    QSqlQuery q(*m_db);
    q.prepare(queryStr);
    q.bindValue(":url", url);
    for (auto it = columns.begin(); it != columns.end(); ++it)
    {
        q.bindValue(":" + it.key(), it.value());
    }

    if (!q.exec())
    {
        qFatal("failed to update columns for '%s', error: '%s'",
               qUtf8Printable(url),
               qUtf8Printable(q.lastError().text()));
    }

    // never update if there is no 'url' in existing cache
    // otherwise we will have incomplete values
    if (auto itr = m_cache.find(url); itr != m_cache.end())
    {
        for (auto it = columns.begin(); it != columns.end(); ++it)
        {
            if (it.key() == "ROW")
                itr->row = it.value().toInt();
            else if (it.key() == "COL")
                itr->col = it.value().toInt();
            else if (it.key() == "SORTCOLUMN")
                itr->sortcolumn = it.value().toInt();
            else if (it.key() == "SORTORDER")
                itr->sortorder = it.value().toInt();
        }
    }
}

void PathHistoryDB::setRowAndColumn(const QString &url, int row, int col)
{
    updateColumns(url, { {"ROW", row}, {"COL", col} });
}

void PathHistoryDB::setSortParams(const QString &url, int sortcolumn, int sortorder)
{
    updateColumns(url, { {"SORTCOLUMN", sortcolumn}, {"SORTORDER", sortorder} });
}

void PathHistoryDB::setRandomSortParams(const QString &url, int row, int col, int randomSeed)
{
    QSqlQuery q(*m_db);
    q.prepare("INSERT INTO random_sort_path_history (URL, ROW, COL, RANDOMSEED) "
              "VALUES (:url, :row, :col, :randomSeed) "
              "ON CONFLICT(URL) DO UPDATE SET "
              "ROW = excluded.ROW, COL = excluded.COL, RANDOMSEED = excluded.RANDOMSEED");

    q.bindValue(":url", url);
    q.bindValue(":row", row);
    q.bindValue(":col", col);
    q.bindValue(":randomSeed", randomSeed);

    if (!q.exec())
    {
        qWarning() << "Failed to set random sort params for" << url
                   << ", error:" << q.lastError().text();
        return;
    }

    // Update cache
    m_cacheRandomSort[url] = { row, col, randomSeed };
}

PathHistoryDB::HistoryDataForRandomSort PathHistoryDB::readForRandomSort(const QString &url) const
{
    if (auto itr = m_cacheRandomSort.find(url); itr != m_cacheRandomSort.end())
        return itr.value();

    QSqlQuery q(*m_db);
    q.prepare("SELECT ROW, COL, RANDOMSEED FROM random_sort_path_history WHERE URL = :url");
    q.bindValue(":url", url);

    if (!q.exec())
    {
        qWarning() << "Failed to read random sort params for" << url
                   << ", error:" << q.lastError().text();
        return {};
    }

    HistoryDataForRandomSort r;
    if (q.next())
    {
        r.row = get<int>(q, 0);
        r.col = get<int>(q, 1);
        r.randomSeed = get<int>(q, 2);
    }

    // Store in cache
    m_cacheRandomSort[url] = r;
    return r;
}
