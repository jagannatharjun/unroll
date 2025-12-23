#ifndef ASYNCARCHIVEFILEREADER_H
#define ASYNCARCHIVEFILEREADER_H

#include <QByteArray>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QWaitCondition>
#include <atomic>
#include <boost/circular_buffer.hpp>

class AsyncArchiveFileReader : public QObject
{
    Q_OBJECT
public:
    explicit AsyncArchiveFileReader(QObject *parent = nullptr);
    ~AsyncArchiveFileReader();

    // Starts the background extraction thread
    void start(const QString &archiveFile, const QString &childPath, qint64 startPos = 0);

    // Aborts extraction and wakes all waiting threads
    void abort();

    // Consumer Methods
    qint64 read(char *data, qint64 maxLen); // Read a specific amount
    QByteArray getAvailableData();          // Read everything currently in buffer
    bool isFinished() const { return !m_workerRunning; }

    // Returns the number of bytes currently buffered and ready for consumption
    qint64 bytesAvailable();

signals:
    void dataAvailable();
    void finished();
    void error(const QString &message);

private:
    void runExtractionTask(QString archivePath, QString childPath, qint64 startPos);

    // Thread Safety & State
    mutable QMutex m_mutex;
    QWaitCondition m_dataAvailable; // Consumer waits for data
    QWaitCondition m_canProduce;    // Producer waits for space
    QWaitCondition m_workerStopped; // Destructor waits for thread exit

    std::atomic<bool> m_workerRunning{false};
    std::atomic<bool> m_aborted{false};

    // The Circular Buffer (Bytes)
    boost::circular_buffer<char> m_byteBuffer;
};

#endif // ASYNCARCHIVEFILEREADER_H
