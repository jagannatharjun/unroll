#ifndef ARCHIVEIODEVICE_H
#define ARCHIVEIODEVICE_H

#include <QIODevice>

class ArchiveIODevice : public QIODevice
{
public:
    ArchiveIODevice(const QString &archivePath, const QString &childPath, QObject *parent = nullptr);

    ~ArchiveIODevice();

    bool isSequential() const override;
    qint64 size() const override;
    qint64 pos() const override;

    bool open(OpenMode mode) override;

    void close() override;

    bool seek(qint64 pos) override;

protected:
    qint64 readData(char *data, qint64 maxlen) override;

    qint64 writeData(const char *, qint64) override;

private:
    void cleanup();

    struct archive *m_archive;
    QString m_archivePath;
    QString m_childPath;
    qint64 m_pos;
    qint64 m_entrySize;
};

#endif // ARCHIVEIODEVICE_H
