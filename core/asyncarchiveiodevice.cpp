#include "asyncarchiveiodevice.h"
#include "directorysystem.hpp"
#include <qdebug.h>

AsyncArchiveIODevice::AsyncArchiveIODevice(QString archivePath,
                                           QString childPath,
                                           qint64 fileSize,
                                           QObject *parent)
    : m_archivePath{archivePath}
    , m_childPath{childPath}
    , m_fileSize{fileSize}
{}

AsyncArchiveIODevice::~AsyncArchiveIODevice()
{
    if (m_reader) {
        m_reader->abort();
        m_reader->deleteLater();
    }
}

void AsyncArchiveIODevice::resetReader()
{
    if (m_reader) {
        m_reader->disconnect(this);
        m_reader->deleteLater();
        m_reader = nullptr;
    }

    m_buf.clear();
    m_bufferPos = 0;
    m_readerStartPos = pos();

    m_reader = new AsyncArchiveFileReader;
    connect(m_reader, &AsyncArchiveFileReader::dataAvailable, this, &QIODevice::readyRead);
    connect(m_reader, &AsyncArchiveFileReader::finished, this, [this]() {
        // Emit readyRead one last time in case there's remaining data
        if (m_buf.size() - m_bufferPos > 0) {
            emit readyRead();
        }

        emit readChannelFinished();
    });
    connect(m_reader, &AsyncArchiveFileReader::error, this, [this](const QString &message) {
        setErrorString(message);
    });

    qInfo() << "AsyncArchiveIODevice::resetReader startin read" << pos();
    m_reader->start(m_archivePath, m_childPath, m_readerStartPos);
}

bool AsyncArchiveIODevice::repositionReader()
{
    const qint64 currentPos = pos();
    const qint64 expectedBufferStart = m_readerStartPos + m_bufferPos;

    // Check if position has been changed externally (not matching our buffer state)
    if (currentPos != expectedBufferStart) {
        qDebug() << "Position mismatch detected - currentPos:" << currentPos
                 << "expectedBufferStart:" << expectedBufferStart;

        qint64 bytesToSkip = currentPos - expectedBufferStart;

        if (bytesToSkip > 0) {
            // Forward seek - need to discard data to catch up
            qDebug() << "Forward seek detected - need to skip" << bytesToSkip << "bytes";

            QElapsedTimer readTimer;
            readTimer.start();

            while (bytesToSkip > 0) {
                // Try to skip within current buffer first
                qint64 availableInBuffer = m_buf.size() - m_bufferPos;
                qint64 skipInBuffer = qMin(bytesToSkip, availableInBuffer);

                if (skipInBuffer > 0) {
                    m_bufferPos += skipInBuffer;
                    bytesToSkip -= skipInBuffer;
                    qDebug() << "Skipped" << skipInBuffer
                             << "bytes in buffer, remaining:" << bytesToSkip;
                }

                // If we still need to skip more, fetch and discard new data
                if (bytesToSkip > 0 && m_bufferPos >= m_buf.size()) {
                    if (readTimer.elapsed() > 500) {
                        qDebug() << "Reached time limit to forward seek, resetting reader";
                        resetReader();
                        break;
                    }

                    QByteArray newData = m_reader->getAvailableData();
                    if (newData.isEmpty()) {
                        // No more data available to skip
                        qDebug() << "No more data available, cannot skip remaining" << bytesToSkip
                                 << "bytes";
                        return false;
                    }

                    m_readerStartPos += m_buf.size(); // Update position for old buffer
                    m_buf = newData;
                    m_bufferPos = 0;
                    qDebug() << "Fetched new buffer of size" << m_buf.size();
                }

                // Safety check to avoid infinite loop
                if (m_bufferPos >= m_buf.size() && bytesToSkip > 0) {
                    break;
                }
            }
        } else if (bytesToSkip < 0) {
            // Backward seek - check if it's within current buffer
            qint64 offsetFromReaderStart = currentPos - m_readerStartPos;

            if (offsetFromReaderStart >= 0 && offsetFromReaderStart < m_buf.size()) {
                // Can handle backward seek within buffer
                qDebug() << "Backward seek within buffer - adjusting position to"
                         << offsetFromReaderStart;
                m_bufferPos = offsetFromReaderStart;
            } else {
                // Backward seek outside buffer - must reset reader
                qDebug() << "Backward seek outside buffer - resetting reader";
                resetReader();
            }
        }
    }

    return true;
}

qint64 AsyncArchiveIODevice::readData(char *data, qint64 maxlen)
{
    if (!m_reader || maxlen <= 0) {
        return 0;
    }

    if (!repositionReader())
        return -1;

    qint64 totalRead = 0;
    // If we've exhausted the buffer, get more data
    if (m_bufferPos >= m_buf.size() && totalRead == 0) {
        m_readerStartPos += m_buf.size(); // Update position for old buffer
        m_buf = m_reader->getAvailableData();
        m_bufferPos = 0;
        if (m_buf.isEmpty()) // No more data available
            return -1;       // nothing to read
    }

    // First, try to read from our internal buffer
    qint64 toRead = qMin(maxlen - totalRead, (qint64) m_buf.size() - m_bufferPos);
    if (toRead > 0) { // Only copy if there's data to copy
        memcpy(data + totalRead, m_buf.constData() + m_bufferPos, toRead);
        totalRead += toRead;
        m_bufferPos += toRead;
    }

    return totalRead == 0 && (!m_reader || m_reader->isFinished()) ? -1 : totalRead;
}

qint64 AsyncArchiveIODevice::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    return -1;
}
bool AsyncArchiveIODevice::isSequential() const
{
    return false;
}

bool AsyncArchiveIODevice::open(OpenMode mode)
{
    if (mode != QIODevice::ReadOnly)
        return false;

    resetReader();
    return QIODevice::open(mode);
}

qint64 AsyncArchiveIODevice::size() const
{
    return m_fileSize;
}

bool AsyncArchiveIODevice::seek(qint64 newpos)
{
    if (newpos >= m_fileSize)
        return false;

    const qint64 oldPos = pos();
    bool s = QIODevice::seek(newpos);
    if (s && oldPos != pos())
        return repositionReader();

    return s;
}

qint64 AsyncArchiveIODevice::bytesAvailable() const
{
    return (m_buf.size() - m_bufferPos) + ((m_reader) ? m_reader->bytesAvailable() : 0);
}
