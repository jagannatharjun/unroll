#ifndef ASYNCARCHIVEFILEREADER_H
#define ASYNCARCHIVEFILEREADER_H

#include <QByteArray>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QWaitCondition>
#include <atomic>
#include <vector>

class AsyncArchiveFileReader : public QObject
{
    Q_OBJECT
public:
    explicit AsyncArchiveFileReader(QObject *parent = nullptr);
    ~AsyncArchiveFileReader();

    void start(const QString &archiveFile, const QString &childPath, qint64 startPos = 0);
    void abort();

    // Consumer Methods
    QByteArray getAvailableData();
    qint64 bytesAvailable() const;
    bool isFinished() const { return !m_workerRunning; }

signals:
    void dataAvailable();
    void finished();
    void error(const QString &message);

private:
    void runExtractionTask(QString archivePath, QString childPath, qint64 startPos);

    mutable QMutex m_mutex;
    mutable QWaitCondition m_dataAvailable;
    QWaitCondition m_canProduce;
    QWaitCondition m_workerStopped;

    std::atomic<bool> m_workerRunning{false};
    std::atomic<bool> m_aborted{false};

    // Ring Buffer state
    std::vector<char> m_buffer;
    size_t m_head = 0;
    size_t m_tail = 0;
    size_t m_count = 0;
    const size_t m_capacity = 64 * 1024 * 1024; // 64MB
};

#endif
