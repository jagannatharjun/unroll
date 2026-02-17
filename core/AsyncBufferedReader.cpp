#include "AsyncBufferedReader.h"
#include <QDebug>
#include <QScopeGuard>
#include <QThreadPool>
#include <cstring>
#include <qelapsedtimer.h>

constexpr qint64 CHUNK_SIZE = 128 * 1024;

AsyncBufferedReader::AsyncBufferedReader(QObject *parent)
    : AsyncBufferedReader(default_capacity, parent)

{}

AsyncBufferedReader::AsyncBufferedReader(size_t capacity, QObject *parent)
    : QIODevice(parent)
    , m_capacity(capacity)
{
    m_buffer.resize(m_capacity);
}

AsyncBufferedReader::~AsyncBufferedReader()
{
    abort();
    QMutexLocker locker(&m_mutex);
    while (m_workerRunning)
        m_threadFinishedWait.wait(&m_mutex);
}

bool AsyncBufferedReader::openSource(std::unique_ptr<QIODevice> source,
                                     qint64 startPos,
                                     QIODevice::OpenMode openMode)
{
    if (!source)
        return false;

    // Ensure source is open
    if (!source->isOpen() && !source->open(QIODevice::ReadOnly))
        return false;

    m_totalSourceSize = source->size();
    m_workerRunning = true;
    m_aborted = false;
    m_sourceEof = false;
    m_head = m_tail = m_count = 0;

    QIODevice::open(openMode | QIODevice::Unbuffered);

    QThreadPool::globalInstance()->start([this, src = std::move(source), startPos]() mutable {
        runWorker(std::move(src), startPos);
    });

    return true;
}

void AsyncBufferedReader::runWorker(std::unique_ptr<QIODevice> source, qint64 startPos)
{
    auto cleanup = qScopeGuard([&] {
        QMutexLocker locker(&m_mutex);
        m_workerRunning = false;
        m_dataWait.notify_all();
        m_threadFinishedWait.notify_all();
    });

    if (startPos > 0 && !source->seek(startPos))
        return;
    qint64 currentPos = startPos;

    while (!m_aborted.load()) {
        QMutexLocker locker(&m_mutex);

        if (m_seekRequested.load()) {
            handleSeekInWorker(source.get(), currentPos);
            continue;
        }

        // Wait as long as the buffer is absolutely full
        while (m_count >= m_capacity && !m_aborted && !m_seekRequested) {
            m_bufferSpaceWait.wait(&m_mutex);
        }

        if (m_aborted || m_seekRequested)
            continue;

        // --- 2. Calculate Contiguous Space ---
        // spaceAtTail is the linear memory available before we have to wrap
        size_t spaceAtTail = m_capacity - m_tail;
        // totalBufferSpace is how much we can add before hitting m_head
        size_t totalBufferSpace = m_capacity - m_count;

        // We can only read the smaller of:
        // - Our desired chunk size
        // - The linear space until the end of the vector
        // - The total space remaining in the buffer
        size_t toRead = std::min({(size_t) CHUNK_SIZE, spaceAtTail, totalBufferSpace});

        if (toRead == 0)
            continue;

        locker.unlock();
        qint64 bytesRead = source->read(&m_buffer[m_tail], toRead);
        locker.relock();

        if (bytesRead <= 0) {
            m_sourceEof = (bytesRead == 0);
            m_dataWait.notify_all();
            break;
        }

        // --- 3. Update Tail Correctly ---
        // Because toRead was clamped by spaceAtTail,
        // (m_tail + bytesRead) will be <= m_capacity
        m_tail = (m_tail + bytesRead) % m_capacity;
        m_count += bytesRead;
        currentPos += bytesRead;

        m_dataWait.notify_all();
        QMetaObject::invokeMethod(this, &AsyncBufferedReader::readyRead, Qt::QueuedConnection);
    }
}

void AsyncBufferedReader::handleSeekInWorker(QIODevice *source, qint64 &currentPos)
{
    qint64 target = m_seekPos.load();
    qint64 bufferStart = currentPos - m_count;

    if (target >= bufferStart && target <= currentPos) {
        size_t offset = target - bufferStart;
        m_head = (m_head + offset) % m_capacity;
        m_count -= offset;
        m_seekSuccess = true;
        m_sourceEof = false;
    } else {
        m_seekSuccess = source->seek(target);
        if (m_seekSuccess) {
            m_head = m_tail = m_count = 0;
            currentPos = target;
            m_sourceEof = false;
        }
    }
    m_seekRequested = false;
    m_seekFinishedWait.notify_all();
}

qint64 AsyncBufferedReader::readData(char *data, qint64 maxlen)
{
    QMutexLocker locker(&m_mutex);
    size_t totalRead = 0;
    size_t target = static_cast<size_t>(maxlen);

    while (totalRead < target) {
        // 1. Wait if the buffer is empty but the worker is still producing
        while (m_count == 0 && m_workerRunning && !m_sourceEof && !m_aborted) {
            m_dataWait.wait(&m_mutex);
        }

        // 2. If buffer is still empty after waiting, we hit EOF/Abort
        if (m_count == 0) {
            break;
        }

        // 3. Determine how much we can pull in this specific iteration
        // We can only read what's in the buffer (m_count)
        // OR what's left to fill our request (target - totalRead)
        size_t availableToCopy = std::min<size_t>(m_count, target - totalRead);
        size_t iterationCopied = 0;

        // 4. Handle Circular Buffer Wrap-around
        while (iterationCopied < availableToCopy) {
            size_t chunk = std::min(availableToCopy - iterationCopied, m_capacity - m_head);

            std::memcpy(data + totalRead, &m_buffer[m_head], chunk);

            m_head = (m_head + chunk) % m_capacity;
            iterationCopied += chunk;
            totalRead += chunk;
        }

        // 5. Update global count and notify producer there is now room
        m_count -= availableToCopy;
        m_bufferSpaceWait.notify_all();

        // If we hit EOF or the source stopped, don't loop again even if totalRead < target
        if (m_sourceEof || m_aborted) {
            break;
        }
    }

    return static_cast<qint64>(totalRead);
}

bool AsyncBufferedReader::seek(qint64 pos)
{
    if (!QIODevice::seek(pos))
        return false;

    QMutexLocker locker(&m_mutex);
    if (!m_workerRunning)
        return false;

    m_seekPos = pos;
    m_seekRequested = true;
    m_bufferSpaceWait.notify_all();

    while (m_seekRequested && m_workerRunning) {
        m_seekFinishedWait.wait(&m_mutex);
    }

    return m_seekSuccess;
}

qint64 AsyncBufferedReader::size() const
{
    return m_totalSourceSize;
}

void AsyncBufferedReader::abort()
{
    m_aborted = true;
    m_bufferSpaceWait.notify_all();
    m_dataWait.notify_all();
    m_seekFinishedWait.notify_all();
}
