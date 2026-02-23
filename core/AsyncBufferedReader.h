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
    class Buffer
    {
        Q_DISABLE_COPY(Buffer)
    public:
        Buffer() = default;

        Buffer(size_t capacity)
            : m_buffer((char *) std::malloc(capacity))
            , m_capacity(capacity)
        {}

        Buffer(Buffer &&buffer)
            : m_buffer(std::exchange(buffer.m_buffer, nullptr))
            , m_capacity(std::exchange(buffer.m_capacity, 0))
        {}

        Buffer &operator=(Buffer &&buffer)
        {
            if (this == &buffer)
                return *this;
            if (m_buffer)
                std::free(m_buffer);
            m_buffer = std::exchange(buffer.m_buffer, nullptr);
            m_capacity = std::move(buffer.m_capacity);
            return *this;
        }

        ~Buffer()
        {
            if (m_buffer)
                std::free(m_buffer);
        }

        void alloc(size_t cap)
        {
            if (m_capacity == 0) {
                m_buffer = (char *) std::malloc(cap);
                m_capacity = cap;
            } else {
                m_buffer = (char *)std::realloc(m_buffer, cap);
                m_capacity = cap;
            }
        }

        size_t capacity() const { return m_capacity; }

        char &operator[](const size_t index)
        {
            assert(index >= 0 && index < m_capacity);
            return m_buffer[index];
        }

        const char &operator[](const size_t index) const
        {
            assert(index >= 0 && index < m_capacity);
            return m_buffer[index];
        }

    private:
        char *m_buffer = nullptr;
        size_t m_capacity = 0;
    };

    static qint64 idealBufferCapacity(qint64 sourceSize);

    static constexpr size_t default_capacity = 2 * 1024 * 1024; // 2MB

    explicit AsyncBufferedReader(QObject *parent = nullptr);
    explicit AsyncBufferedReader(Buffer &&buffer, size_t capacity, QObject *parent = nullptr);
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
    void close() override;
    Buffer closeAndReleaseBuffer();

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *, qint64) override { return -1; }

private:
    friend class AsyncBufferedReaderTest;

    // only support producer thread and once consumer thread
    void runWorker(std::unique_ptr<QIODevice> source, qint64 startPos);
    void handleSeekInWorker(QIODevice *source, qint64 &currentPos);
    void abortWorkerAndWait();

    // thread safe
    bool canMakeSpaceForMoreReading();

    // requires mutex to be locked
    bool makeSpaceForMoreReading();

    mutable QMutex m_mutex;
    QWaitCondition m_dataWait;
    QWaitCondition m_bufferSpaceWait;
    QWaitCondition m_seekFinishedWait;
    QWaitCondition m_threadFinishedWait;

    Buffer m_buffer;
    const size_t m_capacity;

    // denotes full circular queue with all the data
    size_t m_head = 0;
    size_t m_tail = 0;
    std::atomic<size_t> m_count = 0; // m_buffer ptr is only valid if count>0, only reading from m_count is thread safe, modification should be under lock

    // sub circular queue of next read
    // this is maintained to allow fast backward seeks
    std::atomic<size_t> m_readPos = 0;
    std::atomic<size_t> m_readLeft = 0;

    bool m_workerRunning {false};
    std::atomic<bool> m_aborted {false};
    bool m_seekRequested{false};
    std::atomic<qint64> m_seekPos{0};

    qint64 m_totalSourceSize = 0;
    bool m_seekSuccess = false;
    bool m_sourceEof = false;
};

#endif
