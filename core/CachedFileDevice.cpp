#include "CachedFileDevice.h"
#include <QDebug>

CachedFileDevice::CachedFileDevice(QIODevice *source, qint64 chunkSize, QObject *parent)
    : QIODevice(parent)
    , m_source(source)
    , m_chunkSize(chunkSize)
    , m_pos(0)
{}

CachedFileDevice::~CachedFileDevice()
{
    CachedFileDevice::close();
}

bool CachedFileDevice::open(OpenMode mode)
{
    // This device only supports ReadOnly
    if (mode != QIODevice::ReadOnly) {
        setErrorString("Only ReadOnly mode is supported.");
        return false;
    }

    if (!m_source || !m_source->isOpen()) {
        setErrorString("Source device is not open.");
        return false;
    }

    if (!m_cacheFile.open()) {
        setErrorString("Failed to create temporary cache file.");
        return false;
    }

    if (!m_cacheFile.resize(m_source->size())) {
        setErrorString("Failed to resize the cache file");
        return false;
    }

    return QIODevice::open(mode);
}

void CachedFileDevice::close()
{
    m_cacheFile.close();
    m_cachedChunks.clear();
    m_pos = 0;
    m_source = nullptr;
    QIODevice::close();
}

qint64 CachedFileDevice::size() const
{
    return m_source ? m_source->size() : 0;
}

bool CachedFileDevice::atEnd() const
{
    return m_pos >= size();
}

bool CachedFileDevice::seek(qint64 pos)
{
    if (pos < 0 || pos > size()) {
        return false;
    }
    m_pos = pos;
    // We don't seek the cache file here; we seek it right before reading
    return QIODevice::seek(pos);
}

qint64 CachedFileDevice::readData(char *data, qint64 maxlen)
{
    if (m_pos >= size()) {
        return 0; // EOF reached
    }

    // Determine how much we can actually read
    qint64 len = qMin(maxlen, size() - m_pos);
    if (len <= 0)
        return 0;

    // Fetch required chunks from source if not already in cache
    if (!ensureCacheAvailable(m_pos, len)) {
        return -1;
    }

    // Read from the local temporary file
    if (!m_cacheFile.seek(m_pos)) {
        return -1;
    }

    qint64 bytesRead = m_cacheFile.read(data, len);
    if (bytesRead > 0) {
        m_pos += bytesRead;
    }

    return bytesRead;
}

qint64 CachedFileDevice::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    return -1; // Not supported
}

bool CachedFileDevice::ensureCacheAvailable(qint64 pos, qint64 len)
{
    const qint64 totalSize = size();
    const qint64 startChunk = pos / m_chunkSize;
    const qint64 endChunk = (pos + len - 1) / m_chunkSize;

    for (qint64 i = startChunk; i <= endChunk; ++i) {
        if (!m_cachedChunks.contains(i)) {
            qint64 chunkStart = i * m_chunkSize;

            // Handle the potential partial chunk at the very end of the file
            qint64 bytesToRead = qMin(m_chunkSize, totalSize - chunkStart);

            if (bytesToRead <= 0)
                break;

            // Sync source position
            if (m_source->pos() != chunkStart) {
                if (!m_source->seek(chunkStart))
                    return false;
            }

            QByteArray buffer(bytesToRead, Qt::Uninitialized);
            qint64 bytesRead = 0;

            // Pull data into buffer
            while (bytesToRead != bytesRead) {
                const auto currentRead = m_source->read(buffer.data() + bytesRead,
                                                        (bytesToRead - bytesRead));
                if (currentRead == -1)
                    return false;

                bytesRead += currentRead;
            }

            // Write buffer to the temporary cache file
            if (!m_cacheFile.seek(chunkStart))
                return false;

            if (m_cacheFile.write(buffer) != bytesToRead)
                return false;

            m_cachedChunks.insert(i);
        }
    }

    m_cacheFile.flush();
    return true;
}
