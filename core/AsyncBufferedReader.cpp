#include "AsyncBufferedReader.h"
#include <QDebug>
#include <QScopeGuard>
#include <QThreadPool>
#include <QTimer>
#include <cstring>
#include <qelapsedtimer.h>

constexpr qint64 CHUNK_SIZE = 256 * 1024;

// use macro to not get quotes in qDebug output
#define formatMiB(bytes) (QString("%1 MiB").arg(QString::number(static_cast<double>(bytes) / (1024. * 1024), 'f')).toStdString().c_str())


qint64 AsyncBufferedReader::idealBufferCapacity(qint64 sourceSize)
{
    return std::clamp<qint64>(sourceSize * .6,
                              qMin(sourceSize, 200 * 1024 * 1024),
                              700 * 1024 * 1024);
}

AsyncBufferedReader::AsyncBufferedReader(QObject *parent)
    : AsyncBufferedReader(default_capacity, parent)
{}

AsyncBufferedReader::AsyncBufferedReader(AsyncBufferedReader::Buffer &&buffer,
                                         size_t capacity,
                                         QObject *parent)
    : AsyncBufferedReader(capacity, parent)
{
    m_buffer = std::move(buffer);
}

AsyncBufferedReader::AsyncBufferedReader(size_t capacity, QObject *parent)
    : QIODevice(parent)
    , m_capacity(capacity)
{
    auto timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        QMutexLocker lock(&m_mutex);
        if (!m_workerRunning)
            return;

        qDebug() << this << "buffer size" << formatMiB(m_count) << "read left"
                 << formatMiB(m_readLeft);
    });

    timer->start(5000);
}

AsyncBufferedReader::~AsyncBufferedReader()
{
    abortWorkerAndWait();
}

bool AsyncBufferedReader::openSource(std::unique_ptr<QIODevice> source,
                                     qint64 startPos,
                                     QIODevice::OpenMode openMode)
{
    if (!source)
        return false;

    // Ensure source is open
    if (!source->isOpen() && !source->open(QIODevice::ReadOnly)) {
        qWarning("AsyncBufferedReader::openSource failed to open source");
        return false;
    }

    // support reopening
    abortWorkerAndWait();

    m_totalSourceSize = source->size();
    m_workerRunning = true;
    m_aborted = false;
    m_sourceEof = false;
    m_head = m_tail = m_count = m_readPos = m_readLeft = 0;

    QIODevice::open(openMode);
    QIODevice::seek(startPos);

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

    if (m_buffer.capacity() < m_capacity)
        m_buffer.alloc(m_capacity);

    qint64 currentPos = startPos;

    QMutexLocker locker(&m_mutex);
    while (!m_aborted.load()) {
        if (m_seekRequested) {
            handleSeekInWorker(source.get(), currentPos);
            continue;
        }

        // Wait as long as the buffer is absolutely full
        while ((m_sourceEof || m_count == m_capacity) && !m_aborted && !m_seekRequested) {
            m_bufferSpaceWait.wait(&m_mutex);
        }

        if (m_aborted || m_seekRequested || m_sourceEof)
            continue;

        // --- 2. Calculate Contiguous Space ---
        // spaceAtTail is the linear memory available before we have to wrap
        size_t spaceAtTail = m_capacity - m_tail;
        // totalBufferSpace is how much we can add before hitting m_head
        size_t totalBufferSpace = m_capacity - m_count;

        size_t recommendedReadSize = 0;
        if (m_count == 0)
            recommendedReadSize = 32 * 1024;
        else if (m_count < 1024 * 1024)
            recommendedReadSize = 128 * 1024;
        else
            recommendedReadSize = 1024 * 1024;
        assert(recommendedReadSize > 0);

        // We can only read the smaller of:
        // - Our desired chunk size
        // - The linear space until the end of the vector
        // - The total space remaining in the buffer
        size_t toRead = std::min({recommendedReadSize, spaceAtTail, totalBufferSpace});

        if (toRead == 0)
            continue;

        locker.unlock();
        qint64 bytesRead = source->read(&m_buffer[m_tail], toRead);
        locker.relock();

        if (bytesRead <= 0) {
            m_sourceEof = true;
            if (bytesRead < 0)
                qWarning() << "source failed to read" << source->errorString();
            m_dataWait.notify_all();
            continue;
        }

        // --- 3. Update Tail ---
        // Because toRead was clamped by spaceAtTail,
        // (m_tail + bytesRead) will be <= m_capacity
        m_tail = (m_tail + bytesRead) % m_capacity;
        m_count += bytesRead;
        m_readLeft += bytesRead;
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
        m_readPos = (m_head + offset) % m_capacity;
        m_readLeft = m_count - offset;
        m_seekSuccess = true;
        m_sourceEof = false;
        makeSpaceForMoreReading();
    } else {
        qDebug() << this << "seek outside buffer, relativepos" << formatMiB(currentPos - target)
                 << "discarding - count" << formatMiB(m_count) << "readleft"
                 << formatMiB(m_readLeft);
        m_seekSuccess = source->seek(target);
        if (m_seekSuccess) {
            m_head = m_tail = m_count = m_readPos = m_readLeft = 0;
            currentPos = target;
            m_sourceEof = false;
        }
    }
    m_seekRequested = false;
    m_seekFinishedWait.notify_all();
}

void AsyncBufferedReader::abortWorkerAndWait()
{
    abort();
    QMutexLocker locker(&m_mutex);
    while (m_workerRunning)
        m_threadFinishedWait.wait(&m_mutex);
}

bool AsyncBufferedReader::canMakeSpaceForMoreReading() {
    return (m_readLeft < m_capacity / 1.25f && m_count == m_capacity);
}

bool AsyncBufferedReader::makeSpaceForMoreReading()
{
    if (canMakeSpaceForMoreReading()) {
        qDebug() << "AsyncBufferedReader::readData discarding front buffer" << m_readLeft
                 << m_capacity << m_count;
        m_head = m_readPos;
        m_count = m_readLeft.load();
        return true;
    }

    return false;
}

qint64 AsyncBufferedReader::readData(char *data, qint64 maxlen)
{
    qint64 target = maxlen;
    while (m_readLeft > target && target > 0)
    {
        const size_t availableAtTail = m_capacity - m_readPos;
        const size_t chunk = std::min<size_t>(availableAtTail, target);
        std::memcpy(data + (maxlen - target), &m_buffer[m_readPos], chunk);
        target -= chunk;
        m_readPos = (m_readPos + chunk) % m_capacity;
        m_readLeft -= chunk;
    }

    if (target == 0)
    {
        if (canMakeSpaceForMoreReading())
        {
            QMutexLocker locker(&m_mutex);
            if (makeSpaceForMoreReading())
                m_bufferSpaceWait.notify_all();
        }
        return maxlen;
    }

    QMutexLocker locker(&m_mutex);
    qint64 totalRead = 0;

    while (totalRead < target) {
        // 1. Wait if the buffer is empty but the worker is still producing
        while (m_readLeft == 0 && m_workerRunning && !m_sourceEof && !m_aborted) {
            m_dataWait.wait(&m_mutex);
        }

        // 2. If buffer is still empty after waiting, we hit EOF/Abort
        if (m_readLeft == 0) {
            break;
        }

        // 3. Determine how much we can pull in this specific iteration
        // We can only read what's in the buffer (m_readLeft)
        // OR what's left to fill our request (target - totalRead)
        size_t availableToCopy = std::min<size_t>(m_readLeft, target - totalRead);
        size_t iterationCopied = 0;

        // 4. Handle Circular Buffer Wrap-around
        while (iterationCopied < availableToCopy) {
            const size_t availableAtTail = m_capacity - m_readPos;
            const size_t chunk = std::min(availableToCopy - iterationCopied, availableAtTail);

            locker.unlock();
            // Consumer is blocked (this thread), no seek request possible
            std::memcpy(data + totalRead, &m_buffer[m_readPos], chunk);
            locker.relock();

            m_readPos = (m_readPos + chunk) % m_capacity;
            m_readLeft -= chunk;
            iterationCopied += chunk;
            totalRead += chunk;
        }

        // 5. Update global count and notify producer there is now room
        if (makeSpaceForMoreReading())
            m_bufferSpaceWait.notify_all();

        // If we hit EOF or the source stopped, don't loop again even if totalRead < target
        if (m_sourceEof || m_aborted) {
            break;
        }
    }

    return (m_sourceEof || !m_workerRunning || m_aborted) && (totalRead == 0) ? - 1 : static_cast<qint64>(totalRead);
}

bool AsyncBufferedReader::seek(qint64 pos)
{
    if (!QIODevice::seek(pos))
        return false;

    QMutexLocker locker(&m_mutex);
    if (!m_workerRunning) {
        return false;
    }

    m_seekPos = pos;
    m_seekRequested = true;
    m_bufferSpaceWait.notify_all();

    while (m_seekRequested && m_workerRunning) {
        m_seekFinishedWait.wait(&m_mutex);
    }

    return m_seekSuccess;
}

void AsyncBufferedReader::close()
{
    QIODevice::close();
    abort();
}

AsyncBufferedReader::Buffer AsyncBufferedReader::closeAndReleaseBuffer()
{
    abortWorkerAndWait();
    QIODevice::close();
    m_count = 0;
    return std::move(m_buffer);
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
