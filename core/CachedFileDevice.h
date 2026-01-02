#ifndef CACHEDFILEDEVICE_H
#define CACHEDFILEDEVICE_H

#include <QIODevice>
#include <QSet>
#include <QTemporaryFile>

class CachedFileDevice : public QIODevice
{
    Q_OBJECT
public:
    /**
     * @param source The underlying device to read from.
     * @param chunkSize The size of each cache block in bytes.
     */
    explicit CachedFileDevice(QIODevice *source,
                              qint64 chunkSize = 4 * 1024 * 1024,
                              QObject *parent = nullptr);
    ~CachedFileDevice();

    // QIODevice overrides
    bool open(OpenMode mode) override;
    void close() override;
    bool isSequential() const override { return false; }
    qint64 size() const override;
    qint64 pos() const override { return m_pos; }
    bool seek(qint64 pos) override;
    bool atEnd() const override;

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    bool ensureCacheAvailable(qint64 pos, qint64 len);

    QIODevice *m_source;
    QTemporaryFile m_cacheFile;
    qint64 m_chunkSize;
    qint64 m_pos;
    QSet<qint64> m_cachedChunks;
    uchar *m_cachePtr = nullptr; // Pointer to the mapped memory
};

#endif // CACHEDFILEDEVICE_H
