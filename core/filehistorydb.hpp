#pragma once

#include <QObject>
#include <QThread>
#include <QFuture>

class FileHistoryDBWorker;

class FileHistoryDB : public QObject
{
    Q_OBJECT
public:
    explicit FileHistoryDB(const QString &source
                            ,QObject *parent = nullptr);

    ~FileHistoryDB();

    QFuture<bool> isSeen(const QString &mrl);
    void setIsSeen(const QString &mrl, const bool seen);

    QFuture<double> progress(const QString &mrl);
    void setProgress(const QString &mrl, const double progress);

private:
    QThread m_workerThread;
    FileHistoryDBWorker *m_worker;
};

