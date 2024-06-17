#include "filehistorydb.hpp"
#include "filehistorydb_p.hpp"

#include <QSqlDatabase>
#include <QMetaObject>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

namespace
{

template<typename Result>
QFuture<Result> invokeWorker(FileHistoryDBWorker *obj,
                             void (FileHistoryDBWorker::*method)(QPromise<Result>&, const QString&),
                             const QString &mrl)
{
    QPromise<Result> promise;
    QFuture<Result> future = promise.future();
    QMetaObject::invokeMethod(obj, [obj, promise = std::move(promise), mrl, method]() mutable
    {
        std::invoke(method, obj, std::ref(promise), mrl);
    });

    return future;
}

template<typename ResultType>
void select(QPromise<ResultType> &result
            , std::unique_ptr<QSqlDatabase> &db
            , const QString &col
            , const QString &mrl, ResultType defaultValue)
{
    if (!db || !db->isOpen())
    {
        qFatal("Database is not open");
        return;
    }

    const auto reportError = [&](const QString error)
    {
        qFatal("failed to get '%s' for '%s' - error '%s'"
               , qUtf8Printable(col)
               , qUtf8Printable(mrl)
               , qUtf8Printable(error));
    };


    QSqlQuery query(*db);
    query.prepare(QString("SELECT %1 FROM files WHERE MRL = :mrl").arg(col));
    query.bindValue(":mrl", mrl);

    if (!query.exec())
    {
        reportError(query.lastError().text());
        return;
    }

    result.start();

    if (query.next())
    {
        const QVariant val = query.value(0);
        if (!val.canConvert<ResultType>())
        {
            reportError("invalid value type");
            return;
        }

        result.addResult(val.value<ResultType>());
    }
    else
    {
        // this is necessary otherwise it may cause crash
        result.addResult(defaultValue);
    }

    result.finish();
}

template<typename T>
void insert(std::unique_ptr<QSqlDatabase> &db
        , const QString &col
        , const QString &mrl, const T val)
{
    if (!db || !db->open())
    {
        qFatal("Database is not open");
        return;
    }

    QString stat = QString("INSERT INTO files (MRL, %1) VALUES (:mrl, :val)"
                           "ON CONFLICT(MRL) DO UPDATE SET %1 = :val").arg(col);
    QSqlQuery query(*db);
    query.prepare(stat);
    query.bindValue(":mrl", mrl);
    query.bindValue(":val", val);

    if (!query.exec())
    {
        qWarning("Failed to update '%s', mrl '%s' status: %s"
                 , qUtf8Printable(col)
                 , qUtf8Printable(mrl)
                 , qUtf8Printable(query.lastError().text()));
    }
}

}

FileHistoryDB::FileHistoryDB(const QString &source
                             , QObject *parent)
    : QObject{parent}
{
    m_workerThread.start();

    m_worker = new FileHistoryDBWorker;
    m_worker->moveToThread(&m_workerThread);
    QMetaObject::invokeMethod(m_worker, "open"
                              , Qt::AutoConnection
                              , Q_ARG(QString, source));
}

FileHistoryDB::~FileHistoryDB()
{
    m_worker->deleteLater();
    m_worker = nullptr;

    m_workerThread.quit();
    m_workerThread.wait();
}

QFuture<bool> FileHistoryDB::isSeen(const QString &mrl)
{
    return invokeWorker<bool>(m_worker, &FileHistoryDBWorker::isSeen, mrl);
}

void FileHistoryDB::setIsSeen(const QString &mrl, const bool seen)
{
    bool s = QMetaObject::invokeMethod(m_worker, "setIsSeen"
                              , Q_ARG(QString, mrl)
                              , Q_ARG(bool, seen));
    assert(s);
}

QFuture<double> FileHistoryDB::progress(const QString &mrl)
{
    return invokeWorker<double>(m_worker, &FileHistoryDBWorker::progress, mrl);
}

void FileHistoryDB::setProgress(const QString &mrl, const double progress)
{
    bool s = QMetaObject::invokeMethod(m_worker, "setProgress"
                              , Q_ARG(QString, mrl)
                              , Q_ARG(double, progress));
    assert(s);
}

void FileHistoryDBWorker::open(const QString &db)
{
    m_db = std::unique_ptr<QSqlDatabase>(new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE")));

    m_db->setDatabaseName(db);
    if (!m_db->open())
    {
        qFatal("failed to open database %s", qUtf8Printable(db));
        return;
    }

    QSqlQuery q("CREATE TABLE IF NOT EXISTS files ("
                "   MRL TEXT PRIMARY KEY,"
                "   SEEN BOOL,"
                "   PROGRESS DECIMAL"
                ")", *m_db);

    if (!q.exec())
    {
        qFatal() << "failed to execute query" << q.lastError();
        return;
    }
}


void FileHistoryDBWorker::isSeen(QPromise<bool> &result, const QString &mrl)
{
    select(result, m_db, "SEEN", mrl, false);
}

void FileHistoryDBWorker::setIsSeen(const QString &mrl, bool seen)
{
    insert(m_db, "SEEN", mrl, seen);
}

void FileHistoryDBWorker::progress(QPromise<double> &result, const QString &mrl)
{
    select(result, m_db, "PROGRESS", mrl, 0.0);
}

void FileHistoryDBWorker::setProgress(const QString &mrl, double progress)
{
    insert(m_db, "PROGRESS", mrl, progress);
}
