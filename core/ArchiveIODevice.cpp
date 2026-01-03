#include "ArchiveIODevice.h"
#include <QIODevice>
#include <QString>
#include <archive.h>
#include <archive_entry.h>

#include <QDebug>

ArchiveIODevice::ArchiveIODevice(const QString &archivePath,
                                 const QString &childPath,
                                 QObject *parent)
    : QIODevice(parent)
    , m_archivePath(archivePath)
    , m_childPath(childPath)
    , m_archive(nullptr)
    , m_pos(0)
    , m_entrySize(0)
{}

ArchiveIODevice::~ArchiveIODevice()
{
    close();
}

bool ArchiveIODevice::isSequential() const
{
    return false;
}

qint64 ArchiveIODevice::size() const
{
    return m_entrySize;
}

qint64 ArchiveIODevice::pos() const
{
    return m_pos;
}

bool ArchiveIODevice::open(OpenMode mode)
{
    if (mode != ReadOnly || isOpen())
        return false;

    m_archive = archive_read_new();
    archive_read_support_format_all(m_archive);
    archive_read_support_filter_all(m_archive);
    // archive_read_support_format_zip_streamable(m_archive);

    if (archive_read_open_filename(m_archive, m_archivePath.toLocal8Bit().constData(), 10240)
        != ARCHIVE_OK) {
        cleanup();
        return false;
    }

    struct archive_entry *entry;
    bool found = false;
    while (archive_read_next_header(m_archive, &entry) == ARCHIVE_OK) {
        if (QString::fromUtf8(archive_entry_pathname(entry)) == m_childPath) {
            m_entrySize = archive_entry_size(entry);
            found = true;
            qDebug("found entry");
            break;
        }
    }

    if (!found) {
        cleanup();
        return false;
    }

    return QIODevice::open(mode);
}

void ArchiveIODevice::close()
{
    cleanup();
    QIODevice::close();
}

bool ArchiveIODevice::seek(qint64 pos)
{
    if (!isOpen() || !m_archive)
        return false;
    if (pos < 0 || pos > m_entrySize)
        return false;

    if (QIODevice::pos() == pos)
        return QIODevice::seek(pos);

    // SEEK_SET: Seek to absolute position from the start of the entry
    la_int64_t result = archive_seek_data(m_archive, pos, SEEK_SET);

    if (result != ARCHIVE_OK) {
        // Abort if the archive format or filter doesn't support seeking
        setErrorString(tr("Archive does not support seeking: %1")
                           .arg(archive_error_string(m_archive)));
        qInfo() << "error" << errorString();
        return false;
    }

    m_pos = pos;
    return QIODevice::seek(pos);
}

qint64 ArchiveIODevice::readData(char *data, qint64 maxlen)
{
    if (!m_archive || m_pos >= m_entrySize)
        return 0;

    if (m_pos != pos() && !seek(pos()))
        return -1;

    auto bytesRead = archive_read_data(m_archive, data, maxlen);
    if (bytesRead < 0) {
        setErrorString(archive_error_string(m_archive));
        return -1;
    }

    m_pos += bytesRead;
    return bytesRead;
}

qint64 ArchiveIODevice::writeData(const char *, qint64)
{
    return -1;
}

void ArchiveIODevice::cleanup()
{
    if (m_archive) {
        archive_read_free(m_archive);
        m_archive = nullptr;
    }
    m_pos = 0;
}
