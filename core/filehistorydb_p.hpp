
#pragma once

#include <QObject>
#include <QFuture>
#include <QSqlDatabase>


class FileHistoryDBWorker : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

public slots:
    void open(const QString &db);

    void isSeen(QPromise<bool> &result, const QString &mrl);
    void setIsSeen(const QString &mrl, bool seen);

    void progress(QPromise<double> &result, const QString &mrl);
    void setProgress(const QString &mrl, double progress);

private:
    std::unique_ptr<QSqlDatabase> m_db;
};
