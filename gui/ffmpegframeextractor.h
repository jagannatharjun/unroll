#ifndef FFMPEGFRAMEEXTRACTOR_H
#define FFMPEGFRAMEEXTRACTOR_H

#include <QQuickPaintedItem>
#include <QPainter>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <QProcess>
#include <QImage>

#include <QObject>
#include <QProcess>
#include <QMutex>
#include <QMutexLocker>
#include <QImage>
#include <QDebug>

class FFmpegWorker : public QObject {
    Q_OBJECT

public:
    FFmpegWorker(const QString &videoPath, int width, int height, QObject *parent = nullptr)
        : QObject(parent), videoPath(videoPath), width(width), height(height) {}

    void setVideoPath(const QString &path) {
        if (videoPath != path) {
            videoPath = path;
            restartFFmpeg();
        }
    }

    void extractFrame(const QString &timestamp) {
        if (process.state() != QProcess::Running) {
            qDebug() << "FFmpeg process is not running.";
            return;
        }

        QString seekCommand = QString("seek %1\n").arg(timestamp);
        process.write(seekCommand.toUtf8());
        process.waitForBytesWritten();

        QByteArray rawData;
        int dataSize = width * height * 4; // 4 bytes per pixel
        while (rawData.size() < dataSize) {
            if (!process.waitForReadyRead(5000)) {
                qDebug() << "Failed to read frame data.";
                return;
            }
            rawData.append(process.readAll());
        }

        if (rawData.size() >= dataSize) {
            QImage frame(reinterpret_cast<const uchar *>(rawData.data()), width, height, QImage::Format_RGBA8888);
            emit frameReady(frame.copy());
        }
    }

signals:
    void frameReady(const QImage &frame);

private:

    void startFFmpeg() {
        if (process.state() == QProcess::Running) {
            process.kill();
        }

        QStringList arguments;
        arguments << "-i" << videoPath
                  << "-vf" << "fps=1"
                  << "-pix_fmt" << "rgba"
                  << "-f" << "rawvideo"
                  << "pipe:1";

        process.setProcessChannelMode(QProcess::MergedChannels);
        process.start(R"(C:\Users\prince\AppData\Local\Microsoft\WinGet\Links\ffmpeg.exe)", arguments);

        if (!process.waitForStarted()) {
            qDebug() << "Failed to start FFmpeg: " << process.errorString();
        }

        connect(&process, &QProcess::finished, [](int exitcode)
                {
            qDebug() << "ffmpeg exited" << exitcode;
        });
    }

    void restartFFmpeg() {
        if (process.state() == QProcess::Running) {
            process.kill();
            process.waitForFinished();
        }
        startFFmpeg();
    }

    QProcess process;
    QString videoPath;
    int width;
    int height;
};

class FrameRenderer : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QString videoPath READ videoPath WRITE setVideoPath NOTIFY videoPathChanged)
    Q_PROPERTY(QString timestamp READ timestamp WRITE setTimestamp NOTIFY timestampChanged)

public:
    FrameRenderer(QQuickPaintedItem *parent = nullptr)
        : QQuickPaintedItem(parent), workerThread(new QThread), ffmpegWorker(nullptr) {
        ffmpegWorker = new FFmpegWorker("", 0, 0); // Placeholder values
        // ffmpegWorker->moveToThread(workerThread);

        connect(this, &FrameRenderer::videoPathChanged, ffmpegWorker, &FFmpegWorker::setVideoPath);
        connect(this, &FrameRenderer::extractFrame, ffmpegWorker, &FFmpegWorker::extractFrame);
        connect(ffmpegWorker, &FFmpegWorker::frameReady, this, &FrameRenderer::handleFrameReady);

        workerThread->start();
    }

    ~FrameRenderer() {
        workerThread->quit();
        workerThread->wait();
        delete ffmpegWorker;
    }

    QString videoPath() const {
        return m_videoPath;
    }

    void setVideoPath(const QString &path) {
        if (m_videoPath != path) {
            m_videoPath = path;
            emit videoPathChanged(m_videoPath);
        }
    }

    QString timestamp() const {
        return m_timestamp;
    }

    void setTimestamp(const QString &time) {
        if (m_timestamp != time) {
            m_timestamp = time;
            emit timestampChanged();
            emit extractFrame(time);
        }
    }

    void paint(QPainter *painter) override {
        if (!m_frame.isNull()) {
            painter->drawImage(boundingRect(), m_frame);
        }
    }

signals:
    void videoPathChanged(const QString &videoPath);
    void timestampChanged();
    void extractFrame(const QString &timestamp);

public slots:
    void handleFrameReady(const QImage &frame) {
        m_frame = frame;
        update(); // Trigger a repaint
    }

private:
    QString m_videoPath;
    QString m_timestamp;
    QImage m_frame;

    QThread *workerThread;
    FFmpegWorker *ffmpegWorker;
};

#endif // FFMPEGFRAMEEXTRACTOR_H
