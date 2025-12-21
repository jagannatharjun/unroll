#ifndef ASYNCARCHIVEIODEVICE_H
#define ASYNCARCHIVEIODEVICE_H

#include <QIODevice>
#include <QPointer>
#include "AsyncArchiveFileReader.h"

class AsyncArchiveIODevice : public QIODevice
{
    Q_OBJECT
public:
    AsyncArchiveIODevice(QString archivePath,
                         QString childPath,
                         qint64 fileSize,
                         QObject *parent = nullptr);
    ~AsyncArchiveIODevice();

    bool isSequential() const;
    bool open(OpenMode mode);
    qint64 size() const;
    bool seek(qint64 pos);
    qint64 bytesAvailable() const;

protected:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

private:
    void resetReader();

    const QString m_archivePath;
    const QString m_childPath;
    const qint64 m_fileSize;
    QPointer<AsyncArchiveFileReader> m_reader;

    QByteArray m_buf;
    qint64 m_pos;
};

#endif // ASYNCARCHIVEIODEVICE_H
