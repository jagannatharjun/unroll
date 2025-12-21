#include "asyncarchiveiodevice.h"
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

    m_reader = new AsyncArchiveFileReader;
    connect(m_reader, &AsyncArchiveFileReader::dataAvailable, this, &QIODevice::readyRead);
    connect(m_reader, &AsyncArchiveFileReader::dataAvailable, this, [this]() {
        qInfo("dataAvailable QIODEvice");
    });
    connect(this, &QIODevice::readyRead, []() { qInfo("QIODevice::readyread"); });
    connect(m_reader, &AsyncArchiveFileReader::finished, this, [this]() {
        // Emit readyRead one last time in case there's remaining data
        if (!m_buf.isEmpty()) {
            emit readyRead();
        }

        emit readChannelFinished();
    });
    connect(m_reader, &AsyncArchiveFileReader::error, this, [this](const QString &message) {
        setErrorString(message);
    });

    qInfo() << "-prince startin read";
    m_reader->start(m_archivePath, m_childPath, pos());
}

qint64 AsyncArchiveIODevice::readData(char *data, qint64 maxlen)
{
    qDebug() << "AsyncArchiveIODevice::readData" << maxlen;
    if (!m_reader || maxlen <= 0) {
        return 0;
    }

    qint64 totalRead = 0;

    while (totalRead < maxlen) {
        // First, try to read from our internal buffer
        if (!m_buf.isEmpty()) {
            qint64 toRead = qMin(maxlen - totalRead, (qint64) m_buf.size());
            memcpy(data + totalRead, m_buf.constData(), toRead);
            m_buf.remove(0, toRead);
            totalRead += toRead;
            m_pos += toRead;
            continue;
        }

        // If buffer is empty, get more data from reader
        QByteArray newData = m_reader->getAvailableData();

        if (newData.isEmpty()) {
            // No more data available right now
            if (m_reader->isFinished()) {
                // Reader is finished, return what we have
                break;
            }

            // Wait for more data if we haven't read anything yet
            if (totalRead == 0) {
                // This would block - in a real implementation, you might want
                // to wait with a small timeout or return 0 to indicate no data
                break;
            } else {
                break;
            }
        }

        m_buf.append(newData);
    }

    return totalRead;
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
    if (pos() == newpos)
        return true;

    bool s = QIODevice::seek(newpos);
    if (s)
        resetReader();
    return s;
}

qint64 AsyncArchiveIODevice::bytesAvailable() const
{
    qDebug() << "bytesAvailable" << m_buf.size() + ((m_reader) ? m_reader->bytesAvailable() : 0);
    return m_buf.size() + ((m_reader) ? m_reader->bytesAvailable() : 0);
}
