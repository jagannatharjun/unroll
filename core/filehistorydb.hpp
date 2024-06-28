#pragma once

#include <QObject>
#include <QThread>
#include <QFuture>

class FileHistoryDBWorker;

class FileHistoryDB : public QObject
{
    Q_OBJECT
public:
    struct Data
    {
        std::optional<bool> seen;
        std::optional<double> progress;
        std::optional<bool> previewed;
    };

    explicit FileHistoryDB(const QString &source
                            ,QObject *parent = nullptr);

    ~FileHistoryDB();

    QFuture<Data> read(const QString &mrl);

    QFuture<bool> seen(const QString &mrl);
    void setSeen(const QString &mrl, const bool seen);

    QFuture<double> progress(const QString &mrl);
    void setProgress(const QString &mrl, const double progress);

    QFuture<bool> previewed(const QString &mrl);
    void setPreviewed(const QString &mrl, const bool previewed);

private:
    QThread m_workerThread;
    FileHistoryDBWorker *m_worker;
};

