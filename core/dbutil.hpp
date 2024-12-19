#ifndef DBUTIL_HPP
#define DBUTIL_HPP

#include <optional>
#include <QVariant>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QSqlDatabase>

template <typename Result>
static std::optional<Result> get(QSqlQuery &q, int index)
{
    const QVariant val = q.value(index);
    if (!val.canConvert<Result>())
        return std::nullopt;

    return val.value<Result>();
}

// Helper function to execute a single SQL command
static bool executeQuery(QSqlDatabase &db, const QString &queryStr)
{
    QSqlQuery query(db);
    if (!query.exec(queryStr))
    {
        qWarning() << "SQL execution failed:" << queryStr
                   << "Error:" << query.lastError().text();
        return false;
    }
    return true;
}

// Function to check the current auto-vacuum mode
static int getAutoVacuumMode(QSqlDatabase &db)
{
    QSqlQuery query(db);
    if (query.exec("PRAGMA auto_vacuum") && query.next())
    {
        return query.value(0).toInt();
    }
    qWarning() << "Failed to query auto-vacuum mode. Error:" << query.lastError().text();
    return -1;
}

// Function to enable auto-vacuum
static bool enableAutoVacuum(QSqlDatabase &db)
{
    if (!executeQuery(db, "PRAGMA auto_vacuum = FULL"))
    {
        qWarning() << "Failed to enable auto-vacuum.";
        return false;
    }
    // if (!executeQuery(db, "VACUUM"))
    // {
    //     qWarning() << "Failed to vacuum the database.";
    //     return false;
    // }
    return true;
}

#endif // DBUTIL_HPP
