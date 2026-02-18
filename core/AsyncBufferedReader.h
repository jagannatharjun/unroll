#ifndef ASYNCBUFFEREDREADER_H
#define ASYNCBUFFEREDREADER_H

#include <QIODevice>
#include <QMutex>
#include <QVector>
#include <QWaitCondition>
#include <atomic>
#include <memory>

/**
 * @brief A QIODevice that wraps a background producer thread.
 * It consumes data from a source QIODevice and provides it via
 * the standard QIODevice API (read, seek, etc.) asynchronously.
 */
class AsyncBufferedReader : public QIODevice
{
    Q_OBJECT
public:
    static constexpr size_t default_capacity = 2 * 1024 * 1024; // 2MB

    explicit AsyncBufferedReader(QObject *parent = nullptr);
    explicit AsyncBufferedReader(size_t capacity, QObject *parent = nullptr);
    ~AsyncBufferedReader();

    /** Starts the worker thread. Takes ownership of the source device. */
    bool openSource(std::unique_ptr<QIODevice> source,
                    qint64 startPos = 0,
                    QIODevice::OpenMode openMode = QIODevice::ReadOnly);

    void abort();

    // QIODevice overrides
    bool isSequential() const override { return false; }
    qint64 size() const override;
    bool seek(qint64 pos) override;

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *, qint64) override { return -1; }

private:
    friend class AsyncBufferedReaderTest;

    // only support producer thread and once consumer thread
    void runWorker(std::unique_ptr<QIODevice> source, qint64 startPos);
    void handleSeekInWorker(QIODevice *source, qint64 &currentPos);
    void abortWorkerAndWait();

    // requires mutex to be locked
    bool makeSpaceForMoreReading();

    mutable QMutex m_mutex;
    QWaitCondition m_dataWait;
    QWaitCondition m_bufferSpaceWait;
    QWaitCondition m_seekFinishedWait;
    QWaitCondition m_threadFinishedWait;

    QVector<char> m_buffer;
    const size_t m_capacity;

    // denotes full circular queue with all the data
    size_t m_head = 0;
    size_t m_tail = 0;
    size_t m_count = 0;

    // sub circular queue of next read
    // this is maintained to allow fast backward seeks
    size_t m_readPos = 0;
    size_t m_readLeft = 0;

    std::atomic<bool> m_workerRunning{false};
    std::atomic<bool> m_aborted{false};
    std::atomic<bool> m_seekRequested{false};
    std::atomic<qint64> m_seekPos{0};

    qint64 m_totalSourceSize = 0;
    bool m_seekSuccess = false;
    bool m_sourceEof = false;
};

#endif
