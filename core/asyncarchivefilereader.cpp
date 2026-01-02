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
        runExtractionTask(archiveFile, childPath, startPos);
    });
}

void AsyncArchiveFileReader::runExtractionTask(QString archivePath,
                                               QString childPath,
                                               qint64 startPos)
{
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
        emit error(QString::fromUtf8(archive_error_string(a)));
        return;
    }

    bool fileFound = false;
    struct archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        if (m_aborted.load())
            return;

        if (QLatin1StringView(archive_entry_pathname(entry)) == childPath) {
            fileFound = true;

            if (!seekToFile(a, entry, startPos, [this]() { return m_aborted.load(); })) {
                emit error("Failed to seek to start position");
                return;
            }

            // --- ZERO-COPY PRODUCER LOOP ---
            while (!m_aborted.load()) {
                QMutexLocker locker(&m_mutex);

                // Wait until we have at least READ_CHUNK_SIZE space available
                while ((m_capacity - m_count) < READ_CHUNK_SIZE && !m_aborted.load()) {
                    m_canProduce.wait(&m_mutex);
                }
                if (m_aborted.load())
                    return;

                // Find the contiguous linear space at the end of the buffer
                size_t linearSpace = m_capacity - m_tail;
                size_t toRead = std::min<size_t>(linearSpace, READ_CHUNK_SIZE);

                // IMPORTANT: Unlock during I/O to allow the consumer to drain the buffer
                // while libarchive is decompressing data.
                locker.unlock();
                la_ssize_t bytesRead = archive_read_data(a, &m_buffer[m_tail], toRead);
                locker.relock();

                if (bytesRead < 0) {
                    emit error(QString::fromUtf8(archive_error_string(a)));
                    return;
                }
                if (bytesRead == 0)
                    break; // EOF

                m_tail = (m_tail + bytesRead) % m_capacity;
                m_count += bytesRead;

                m_dataAvailable.notify_all();

                // Using QueuedConnection ensures the UI thread handles this
                // when it's next "awake" and the object is guaranteed to exist.
                QMetaObject::invokeMethod(this,
                                          &AsyncArchiveFileReader::dataAvailable,
                                          Qt::QueuedConnection);
            }
            return;
        }
        archive_read_data_skip(a);
    }

    if (!fileFound && !m_aborted.load()) {
        emit error("File not found in archive");
    }
}

QByteArray AsyncArchiveFileReader::getAvailableData()
{
    QMutexLocker locker(&m_mutex);
    while (m_count == 0 && m_workerRunning && !m_aborted.load()) {
        m_dataAvailable.wait(&m_mutex);
    }

    if (m_count == 0)
        return QByteArray();

    QByteArray result;
    result.resize(m_count);

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
    return result;
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
