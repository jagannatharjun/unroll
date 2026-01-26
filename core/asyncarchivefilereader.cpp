#include "asyncarchivefilereader.h"
#include <QDebug>
#include <QScopeGuard>
#include <QThreadPool>
#include <algorithm>
#include <archive.h>
#include <archive_entry.h>

constexpr int READ_CHUNK_SIZE = 512 * 1024;

// Helper for manual seeking in libarchive
bool seekToFile(struct archive *a,
                struct archive_entry *entry,
                qint64 pos,
                std::function<bool()> isAborted)
{
    if (pos == 0)
        return true;

    la_int64_t actualPos = archive_seek_data(a, pos, SEEK_SET);
    if (actualPos < 0) {
        qWarning("Archive seek failed, falling back to manual skip.");
        qint64 toSkip = pos;
        char skipBuffer[8192];
        while (toSkip > 0 && !isAborted()) {
            la_ssize_t bytesRead = archive_read_data(a,
                                                     skipBuffer,
                                                     qMin(toSkip, (qint64) sizeof(skipBuffer)));
            if (bytesRead <= 0)
                return false;
            toSkip -= bytesRead;
        }
        return toSkip == 0;
    }
    return actualPos == pos;
}

AsyncArchiveFileReader::AsyncArchiveFileReader(QObject *parent)
    : QObject(parent)
{
    m_buffer.resize(m_capacity);
}

AsyncArchiveFileReader::~AsyncArchiveFileReader()
{
    abort();
    QMutexLocker locker(&m_mutex);
    while (m_workerRunning) {
        m_workerStopped.wait(&m_mutex);
    }
}

void AsyncArchiveFileReader::start(const QString &archiveFile,
                                   const QString &childPath,
                                   qint64 startPos)
{
    QMutexLocker locker(&m_mutex);
    if (m_workerRunning)
        return;

    m_workerRunning = true;
    m_aborted = false;
    m_head = 0;
    m_tail = 0;
    m_count = 0;

    QThreadPool::globalInstance()->start([this, archiveFile, childPath, startPos]() {
        qInfo() << "running on thread" << QThread::currentThreadId();
        runExtractionTask(archiveFile, childPath, startPos);
    });
}

void AsyncArchiveFileReader::runExtractionTask(QString archivePath,
                                               QString childPath,
                                               qint64 startPos)
{
    const auto raiseError = [this](const QString &error)
    {
        // Use invokeMethod to safely signal completion from a thread
        QMetaObject::invokeMethod(this, &AsyncArchiveFileReader::error, error);
    };

    struct archive *a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);
    archive_read_support_format_zip_seekable(a);

    auto cleanup = qScopeGuard([&] {
        archive_read_close(a);
        archive_read_free(a);

        QMutexLocker locker(&m_mutex);
        m_workerRunning = false;
        m_dataAvailable.notify_all();
        m_workerStopped.notify_all();

        // Use invokeMethod to safely signal completion from a thread
        QMetaObject::invokeMethod(this, &AsyncArchiveFileReader::finished, Qt::QueuedConnection);
    });

    if (archive_read_open_filename_w(a, archivePath.toStdWString().c_str(), 10240) != ARCHIVE_OK) {
        raiseError(QString::fromUtf8(archive_error_string(a)));
        return;
    }

    bool fileFound = false;
    struct archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        if (m_aborted.load())
            return;

        if (QLatin1StringView(archive_entry_pathname(entry)) != childPath) {
            continue;
        }

        fileFound = true;

        if (!seekToFile(a, entry, startPos, [this]() { return m_aborted.load(); })) {
            raiseError("Failed to seek to start position");
            return;
        }

        qint64 currentPos = startPos;

        while (!m_aborted.load()) {
            QMutexLocker locker(&m_mutex);

            // 1. Check if a seek was requested
            if (m_seekRequested.load()) {
                qint64 pos = m_seekPos.load();

                // The buffer contains data from [bufferStartPos] to [currentPos]
                const qint64 bufferStartPos = currentPos - m_count;
                if (bufferStartPos <= pos && currentPos >= pos) {
                    const qint64 bytesToSkip = pos - bufferStartPos;
                    qDebug() << "Seek request inside the buffer, bytes to skip" << bytesToSkip;
                    m_head = (m_head + bytesToSkip) % m_capacity;
                    m_count -= bytesToSkip;
                    m_seekSuccess = true;
                } else {
                    const la_int64_t actualPos = archive_seek_data(a, pos, SEEK_SET);

                    m_seekSuccess = (actualPos >= 0 && actualPos == pos);

                    if (m_seekSuccess) {
                        m_head = 0;
                        m_tail = 0;
                        m_count = 0;
                        currentPos = pos;
                    }
                }

                m_seekRequested = false;
                m_seekDone.notify_all(); // Wake up the specific seek waiter
                continue;                // Re-evaluate loop condition and buffer space
            }

            // 2. Wait until we have space OR a seek is requested
            while (((m_capacity - m_count) < READ_CHUNK_SIZE)
                   && !m_aborted.load()
                   && !m_seekRequested) {
                m_canProduce.wait(&m_mutex);
            }

            if (m_aborted.load())
                return;
            if (m_seekRequested)
                continue; // Go back to top to handle seek

            // 3. Perform Read
            size_t linearSpace = m_capacity - m_tail;
            size_t toRead = std::min<size_t>(linearSpace, READ_CHUNK_SIZE);

            locker.unlock();
            const la_ssize_t bytesRead = archive_read_data(a, &m_buffer[m_tail], toRead);
            locker.relock();

            if (bytesRead < 0) {
                raiseError(QString::fromUtf8(archive_error_string(a)));
                return;
            }
            if (bytesRead == 0)
                break; // EOF

            m_tail = (m_tail + bytesRead) % m_capacity;
            m_count += bytesRead;
            currentPos += bytesRead;

            m_dataAvailable.notify_all();

            // Using QueuedConnection ensures the UI thread handles this
            // when it's next "awake" and the object is guaranteed to exist.
            QMetaObject::invokeMethod(this,
                                      &AsyncArchiveFileReader::dataAvailable,
                                      Qt::QueuedConnection);
        }
        return;
    }

    if (!fileFound && !m_aborted.load()) {
        raiseError("File not found in archive");
    }
}

void AsyncArchiveFileReader::getAvailableData(QByteArray &result)
{
    QMutexLocker locker(&m_mutex);
    while (m_count == 0 && m_workerRunning && !m_aborted.load()) {
        m_dataAvailable.wait(&m_mutex);
    }

    result.resize(m_count);
    if (m_count == 0) {
        return;
    }

    size_t totalToCopy = m_count;
    size_t copied = 0;

    while (copied < totalToCopy) {
        size_t linearData = m_capacity - m_head;
        size_t toCopy = std::min<size_t>(totalToCopy - copied, linearData);

        std::memcpy(result.data() + copied, &m_buffer[m_head], toCopy);

        m_head = (m_head + toCopy) % m_capacity;
        copied += toCopy;
    }

    m_count = 0;
    m_canProduce.notify_all();
}

QByteArray AsyncArchiveFileReader::getAvailableData()
{
    QByteArray data;
    getAvailableData(data);
    return data;
}

qint64 AsyncArchiveFileReader::bytesAvailable() const
{
    QMutexLocker locker(&m_mutex);
    while (m_count == 0 && m_workerRunning && !m_aborted.load()) {
        m_dataAvailable.wait(&m_mutex);
    }
    return static_cast<qint64>(m_count);
}

void AsyncArchiveFileReader::abort()
{
    m_aborted = true;
    m_canProduce.notify_all();
    m_dataAvailable.notify_all();
}

bool AsyncArchiveFileReader::seek(qint64 pos)
{
    QMutexLocker locker(&m_mutex);

    if (!m_workerRunning) return false;

    m_seekPos = pos;
    m_seekRequested = true;

    // Wake the worker in case it's sleeping because the buffer is full
    m_canProduce.notify_all();

    // Wait on the dedicated seek condition, NOT the production condition
    while (m_seekRequested && !m_aborted && m_workerRunning) {
        m_seekDone.wait(&m_mutex);
    }

    return m_seekSuccess && m_workerRunning;
}
