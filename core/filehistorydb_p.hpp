
#pragma once

#include <QObject>
#include <QFuture>
#include <QSqlDatabase>

#include "filehistorydb.hpp"


class FileHistoryDBWorker : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

public slots:
    void open(const QString &db);

    void read(QPromise<FileHistoryDB::Data> &result, const QString &mrl);

    void seen(QPromise<bool> &result, const QString &mrl);
    void setSeen(const QString &mrl, bool seen);

    void progress(QPromise<double> &result, const QString &mrl);
    void setProgress(const QString &mrl, double progress);

    void previewed(QPromise<bool> &result, const QString &mrl);
    void setPreviewed(const QString &mrl, bool previewed);


private:
    std::unique_ptr<QSqlDatabase> m_db;
};
