#include "asyncarchivefilereader.h"

#include <QThreadPool>

#include <QDebug>
#include <QThreadPool>
#include "asyncarchivefilereader.h"
#include "directorysystem.hpp"
#include <archive.h>
#include <archive_entry.h>

constexpr int READ_SIZE = 128 * 1024;
constexpr int BUFFER_CAPACITY = READ_SIZE * 64;

AsyncArchiveFileReader::AsyncArchiveFileReader(QObject *parent)
    : QObject(parent)
{
    m_isFinished = true;

    // Reserve buffer capacity upfront
    m_buffer.reserve(BUFFER_CAPACITY);

    auto reader = new AsyncArchiveFileReaderImpl;
    m_reader = reader;

    connect(
        reader,
        &AsyncArchiveFileReaderImpl::foundFile,
        this,
        [this](qint64 fileSize) {
            {
                QMutexLocker locker(&m_mutex);
                m_fileSize = fileSize;
            }

            m_sizeReady.notify_all();
        },
        Qt::DirectConnection);

    connect(
        reader,
        &AsyncArchiveFileReaderImpl::read,
        this,
        [this](char *data, qint64 size) {
            qDebug() << " AsyncArchiveFileReaderImpl::read" << size;
            {
                QMutexLocker locker(&m_mutex);
                m_buffer.append(data, size);
            }

            m_bufferEmpty.notify_all();
            emit dataAvailable();
            // QMetaObject::invokeMethod(this, &AsyncArchiveFileReader::dataAvailable);

            {
                QMutexLocker locker(&m_mutex);
                while (m_buffer.size() >= m_maxBufferSize && !m_aborted)
                    m_bufferNotFull.wait(&m_mutex);
            }
        },
        Qt::DirectConnection);

    connect(
        reader,
        &AsyncArchiveFileReaderImpl::finished,
        this,
        [this]() {
            {
                QMutexLocker locker(&m_mutex);
                m_isFinished = true;
            }

            m_bufferEmpty.notify_all();
            m_sizeReady.notify_all();
            m_finishedCondition.notify_all();
            QMetaObject::invokeMethod(this, &AsyncArchiveFileReader::finished, Qt::QueuedConnection);
        },
        Qt::DirectConnection);

    connect(reader, &AsyncArchiveFileReaderImpl::error, this, [](const QString &error) {
        qInfo() << "error while processing" << error;
    });
    connect(reader, &AsyncArchiveFileReaderImpl::error, this, &AsyncArchiveFileReader::error);
}

AsyncArchiveFileReader::~AsyncArchiveFileReader()
{
    abort();

    QMutexLocker locker(&m_mutex);
    while (!m_isFinished) {
        m_finishedCondition.wait(&m_mutex);
    }
}

void AsyncArchiveFileReader::start(const QString &archiveFile,
                                   const QString &childPath,
                                   qint64 startPos)
{
    if (!m_isFinished)
        return;

    QMutexLocker locker(&m_mutex);
    m_isFinished = false;

    QThreadPool::globalInstance()->start(
        [reader = this->m_reader, archiveFile, childPath, startPos]() {
            reader->start(archiveFile, childPath, startPos);
        });
}

qint64 AsyncArchiveFileReader::fileSize() const
{
    QMutexLocker locker(&m_mutex);
    return m_fileSize;
}

QByteArray AsyncArchiveFileReader::getAvailableData()
{
    QByteArray data;

    {
        QMutexLocker locker(&m_mutex);

        while (m_buffer.isEmpty() && !m_aborted && !m_isFinished) {
            m_bufferEmpty.wait(&m_mutex);
        }

        data = std::move(m_buffer);
        m_buffer.clear();
        m_buffer.reserve(m_maxBufferSize); // Maintain capacity
    }

    m_bufferNotFull.wakeOne();
    return data;
}

qint64 AsyncArchiveFileReader::bytesAvailable()
{
    QMutexLocker locker(&m_mutex);
    while (m_buffer.isEmpty() && !m_aborted && !m_isFinished)
        m_bufferEmpty.wait(&m_mutex);

    return m_buffer.size();
}

bool AsyncArchiveFileReader::waitForFileSize(int timeoutMs)
{
    QMutexLocker locker(&m_mutex);
    if (m_isFinished)
        return true;

    // Wait for extraction loop to find the header
    bool result = m_sizeReady.wait(&m_mutex, timeoutMs);
    return result && m_fileSize != -1;
}

void AsyncArchiveFileReader::abort()
{
    m_aborted = true;
    m_reader->abort();

    m_bufferNotFull.notify_all();
}

bool seekToFile(struct archive *a,
                struct archive_entry *entry,
                qint64 pos,
                std::function<bool()> abort)
{
    // Attempt to seek to start position
    if (pos == 0) {
        return true; // No seeking needed
    }

    la_int64_t actualPos = archive_seek_data(a, pos, SEEK_SET);
    qInfo() << "seekToFile" << pos << actualPos;

    if (actualPos < 0) {
        qWarning("archive_seek_data failed, manual seek");
        // Seek not supported, fallback to read-and-discard
        qint64 toSkip = pos;
        char skipBuffer[8192];

        while (toSkip > 0 && !abort()) {
            la_ssize_t toRead = qMin(toSkip, 8192);
            la_ssize_t bytesRead = archive_read_data(a, skipBuffer, toRead);

            if (bytesRead <= 0) {
                return false; // Error or EOF
            }

            toSkip -= bytesRead;
        }

        return toSkip == 0;
    }

    return actualPos == pos;
}

void AsyncArchiveFileReaderImpl::start(const QString &archivePath,
                                       const QString &childPath,
                                       qint64 startpos)
{
    struct archive *a = archive_read_new();
    if (!a) {
        emit error("Failed to create archive reader");
        emit finished();
        return;
    }

    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);

    int ret = archive_read_open_filename_w(a, archivePath.toStdWString().c_str(), 10240);
    if (ret != ARCHIVE_OK) {
        emit error(QString("Failed to open archive: %1").arg(archive_error_string(a)));

        archive_read_free(a);
        emit finished();
        return;
    }

    struct archive_entry *entry;
    bool fileFound = false;

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        if (m_aborted) {
            break;
        }

        if (QLatin1StringView(archive_entry_pathname(entry)) == childPath) {
            fileFound = true;
            qInfo() << "AsyncArchiveFileReaderImpl::start found file" << childPath;

            // Store file size
            qint64 size = archive_entry_size(entry);
            emit foundFile(size);

            // Seek to start position
            if (!seekToFile(a, entry, startpos, [this]() { return m_aborted.load(); })) {
                emit error("Failed to seek to start position");
                break;
            }

            // Read data in chunks
            char chunk[READ_SIZE] = {};
            la_ssize_t bytesRead;

            while ((bytesRead = archive_read_data(a, chunk, READ_SIZE)) > 0 && !m_aborted) {
                emit read(chunk, bytesRead);
            }

            if (bytesRead < 0) {
                emit error(QString("Read error: %1").arg(archive_error_string(a)));
            }

            break;
        } else {
            // Skip this entry
            archive_read_data_skip(a);
        }
    }

    if (!fileFound && !m_aborted.load(std::memory_order_relaxed)) {
        emit error(QString("File not found in archive: %1").arg(childPath));
    }

    archive_read_close(a);
    archive_read_free(a);
    emit finished();
}
