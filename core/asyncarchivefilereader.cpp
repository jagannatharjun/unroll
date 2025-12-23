#include "asyncarchivefilereader.h"
#include <QDebug>
#include <QScopeGuard>
#include <QThreadPool>
#include <archive.h>
#include <archive_entry.h>

// Configuration
constexpr int BUFFER_CAPACITY = 32 * 1024 * 1024; // 16MB fixed memory
constexpr int READ_CHUNK_SIZE = 256 * 1024;       // 256KB read increments

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
    m_byteBuffer.set_capacity(BUFFER_CAPACITY);
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
    m_byteBuffer.clear();

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

    auto cleanup = qScopeGuard([&] {
        archive_read_close(a);
        archive_read_free(a);

        QMutexLocker locker(&m_mutex);
        m_workerRunning = false;
        m_dataAvailable.notify_all(); // Wake consumer to let them know no more is coming
        m_workerStopped.notify_all(); // Wake destructor
        emit finished();
    });

    if (archive_read_open_filename_w(a, archivePath.toStdWString().c_str(), 10240) != ARCHIVE_OK) {
        emit error(QString::fromUtf8(archive_error_string(a)));
        return;
    }

    struct archive_entry *entry;
    bool fileFound = false;

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        if (m_aborted.load())
            return;

        if (QLatin1StringView(archive_entry_pathname(entry)) == childPath) {
            fileFound = true;

            if (!seekToFile(a, entry, startPos, [this]() { return m_aborted.load(); })) {
                emit error("Failed to seek to start position");
                return;
            }

            char tempReadBuf[READ_CHUNK_SIZE];

            // --- PRODUCER LOOP ---
            while (!m_aborted.load()) {
                la_ssize_t bytesRead = archive_read_data(a, tempReadBuf, READ_CHUNK_SIZE);
                if (bytesRead < 0) {
                    emit error(QString::fromUtf8(archive_error_string(a)));
                    return;
                }
                if (bytesRead == 0)
                    break; // End of file

                QMutexLocker locker(&m_mutex);

                // Backpressure: Wait if buffer cannot fit the new chunk
                while (m_byteBuffer.capacity() - m_byteBuffer.size() < (size_t) bytesRead
                       && !m_aborted.load()) {
                    m_canProduce.wait(&m_mutex);
                }

                if (m_aborted.load())
                    return;

                // Push bytes into circular buffer
                m_byteBuffer.insert(m_byteBuffer.end(), tempReadBuf, tempReadBuf + bytesRead);

                m_dataAvailable.notify_all();
                emit dataAvailable();
            }
            return;
        }
        archive_read_data_skip(a);
    }

    if (!fileFound && !m_aborted.load()) {
        emit error("File not found in archive");
    }
}

qint64 AsyncArchiveFileReader::read(char *data, qint64 maxLen)
{
    if (maxLen <= 0)
        return 0;

    QMutexLocker locker(&m_mutex);

    // Block until data arrives or worker stops
    while (m_byteBuffer.empty() && m_workerRunning && !m_aborted.load()) {
        m_dataAvailable.wait(&m_mutex);
    }

    if (m_byteBuffer.empty())
        return 0;

    qint64 bytesToCopy = qMin(maxLen, (qint64) m_byteBuffer.size());

    // boost::circular_buffer iterators handle wrap-around during std::copy
    std::copy(m_byteBuffer.begin(), m_byteBuffer.begin() + bytesToCopy, data);
    m_byteBuffer.erase_begin(bytesToCopy);

    m_canProduce.notify_all(); // Wake producer
    return bytesToCopy;
}

QByteArray AsyncArchiveFileReader::getAvailableData()
{
    QMutexLocker locker(&m_mutex);

    while (m_byteBuffer.empty() && m_workerRunning && !m_aborted.load()) {
        m_dataAvailable.wait(&m_mutex);
    }

    if (m_byteBuffer.empty())
        return QByteArray();

    QByteArray result(m_byteBuffer.size(), Qt::Uninitialized);
    std::copy(m_byteBuffer.begin(), m_byteBuffer.end(), result.data());

    m_byteBuffer.clear();
    m_canProduce.notify_all();
    return result;
}

qint64 AsyncArchiveFileReader::bytesAvailable()
{
    QMutexLocker locker(&m_mutex);

    // Block if the buffer is empty, but only if the worker is still
    // actively extracting and we haven't aborted.
    while (m_byteBuffer.empty() && m_workerRunning && !m_aborted.load()) {
        m_dataAvailable.wait(&m_mutex);
    }

    // Once woken, return whatever is currently in the buffer.
    // If the worker finished or aborted, this might still be 0.
    return static_cast<qint64>(m_byteBuffer.size());
}

void AsyncArchiveFileReader::abort()
{
    m_aborted = true;
    m_canProduce.notify_all();
    m_dataAvailable.notify_all();
}
