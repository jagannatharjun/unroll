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


template <typename Result>
std::optional<Result> get(QSqlQuery &q, int index)
{
    const QVariant val = q.value(index);
    if (!val.canConvert<Result>())
        return std::nullopt;

    return val.value<Result>();
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

QFuture<FileHistoryDB::Data> FileHistoryDB::read(const QString &mrl)
{
    return invokeWorker<Data>(m_worker, &FileHistoryDBWorker::read, mrl);
}


void FileHistoryDBWorker::open(const QString &db)
{
    m_db = std::unique_ptr<QSqlDatabase>(
                new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE")));

    m_db->setDatabaseName(db);
    if (!m_db->open())
    {
        qFatal("failed to open database %s", qUtf8Printable(db));
        return;
    }

    QSqlQuery q("CREATE TABLE IF NOT EXISTS files ("
                "   MRL TEXT PRIMARY KEY,"
                "   SEEN BOOL,"
                "   PROGRESS DECIMAL,"
                "   PREVIEWED BOOL"
                ")", *m_db);

    if (!q.exec())
    {
        qFatal() << "failed to execute query" << q.lastError();
        return;
    }
}

void FileHistoryDBWorker::read(QPromise<FileHistoryDB::Data> &result, const QString &mrl)
{
    if (!m_db || !m_db->isOpen())
    {
        qFatal("Database is not open");
        return;
    }

    const auto reportError = [&](const QString error)
    {
        qFatal("failed to read for '%s' - error '%s'"
               , qUtf8Printable(mrl)
               , qUtf8Printable(error));
    };


    QSqlQuery query(*m_db);
    query.prepare("SELECT SEEN, PROGRESS, PREVIEWED FROM files WHERE MRL = :mrl");
    query.bindValue(":mrl", mrl);

    if (!query.exec())
    {
        reportError(query.lastError().text());
        return;
    }

    FileHistoryDB::Data data;
    if (query.next())
    {
        data.seen = get<bool>(query, 0);
        data.progress = get<double>(query, 1);
        data.previewed = get<bool>(query, 2);
    }

    result.start();
    result.addResult(std::move(data));
    result.finish();
}


#define FileHistoryDB_IMPL(type, setter) \
    void FileHistoryDB:: setter (const QString &mrl, const type newValue) { \
        QMetaObject::invokeMethod(m_worker, #setter, Q_ARG(QString, mrl), Q_ARG(type, newValue)); \
    }

FileHistoryDB_IMPL(bool, setSeen)
FileHistoryDB_IMPL(double, setProgress)
FileHistoryDB_IMPL(bool, setPreviewed)


void FileHistoryDBWorker::setSeen(const QString &mrl, bool seen)
{
    insert(m_db, "SEEN", mrl, seen);
}

void FileHistoryDBWorker::setProgress(const QString &mrl, double progress)
{
    insert(m_db, "PROGRESS", mrl, progress);
}

void FileHistoryDBWorker::setPreviewed(const QString &mrl, const bool previewed)
{
    insert(m_db, "PREVIEWED", mrl, previewed);
}
