#ifndef ASYNCARCHIVEFILEREADER_H
#define ASYNCARCHIVEFILEREADER_H

#include <QObject>

#include <QByteArray>
#include <QMutex>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QWaitCondition>
#include <atomic>

class AsyncArchiveFileReaderImpl : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

    void start(const QString &archivePath, const QString &childPath, qint64 startpos);

    void abort() { m_aborted = true; }

signals:
    void read(char *data, qint64 size);
    void foundFile(qint64 size);
    void finished();
    void error(const QString &error);

private:
    std::atomic_bool m_aborted = false;
};

class AsyncArchiveFileReader : public QObject
{
    Q_OBJECT
public:
    explicit AsyncArchiveFileReader(QObject *parent = nullptr);
    ~AsyncArchiveFileReader() override;

    // Non-copyable, non-movable
    AsyncArchiveFileReader(const AsyncArchiveFileReader &) = delete;
    AsyncArchiveFileReader &operator=(const AsyncArchiveFileReader &) = delete;

    void start(const QString &archiveFile, const QString &childPath, qint64 startPos = 0);

    bool isFinished() const
    {
        QMutexLocker locker(&m_mutex);
        return m_isFinished;
    }

    qint64 fileSize() const;
    QByteArray getAvailableData();
    qint64 bytesAvailable() const;
    bool waitForFileSize(int timeoutMs = 5000);
    void abort();

signals:
    void dataAvailable();
    void finished();
    void error(const QString &message);

private:
    mutable QMutex m_mutex;
    QWaitCondition m_bufferNotFull;
    QWaitCondition m_sizeReady;
    QWaitCondition m_finishedCondition;

    QByteArray m_buffer;
    qint64 m_maxBufferSize = 1024 * 1024; // 1MB

    bool m_aborted{false};
    qint64 m_fileSize{-1};
    bool m_isFinished{false};
    QPointer<AsyncArchiveFileReaderImpl> m_reader;
};

#endif // ASYNCARCHIVEFILEREADER_H
