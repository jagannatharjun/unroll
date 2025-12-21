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

    m_buf.clear();
    m_bufferPos = 0;

    m_reader = new AsyncArchiveFileReader;
    connect(m_reader, &AsyncArchiveFileReader::dataAvailable, this, &QIODevice::readyRead);
    connect(m_reader, &AsyncArchiveFileReader::dataAvailable, this, []() {
        qInfo("dataAvailable QIODEvice");
    });
    connect(this, &QIODevice::readyRead, []() { qInfo("QIODevice::readyread"); });
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

        qint64 toRead = qMin(maxlen - totalRead, (qint64) m_buf.size() - m_bufferPos);
        if (toRead >= 0) {
            memcpy(data + totalRead, m_buf.constData() + m_bufferPos, toRead);
            totalRead += toRead;
            m_bufferPos += toRead;
        }

        Q_ASSERT(m_bufferPos <= m_buf.size());
        if (m_bufferPos != m_buf.size())
            break;

        // If buffer is empty, get more data from reader
        m_buf = m_reader->getAvailableData();
        m_bufferPos = 0;
        if (m_buf.isEmpty())
            break;
    }

    if (totalRead == 0)
        return m_reader && m_reader->isFinished() ? -1 : 0;

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
    if (newpos >= m_fileSize)
        return false;

    const qint64 oldPos = pos();
    bool s = QIODevice::seek(newpos);
    if (s && oldPos != newpos)
        resetReader();

    return s;
}

qint64 AsyncArchiveIODevice::bytesAvailable() const
{
    return (m_buf.size() - m_bufferPos) + ((m_reader) ? m_reader->bytesAvailable() : 0);
}
