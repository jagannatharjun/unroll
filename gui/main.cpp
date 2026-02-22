#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QAbstractItemModel>

#include "../core/directorysystemmodel.hpp"
#include "filebrowser.hpp"
#include "viewcontroller.hpp"
#include "preferences.hpp"

#include <QQuickStyle>

#include <QCoreApplication>
#include <QDateTime>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <QQueue>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

struct LogMessage {
    QtMsgType type;
    QString time;
    QString level;
    QString color;
    void* threadId;
    QString location;
    QString msg;
};

class LogWorker : public QObject {
public:
    void enqueue(LogMessage m) {
        QMutexLocker locker(&mutex);
        queue.enqueue(m);
        condition.wakeOne();
    }

public:
    void process() {
        while (true) {
            LogMessage m;
            {
                QMutexLocker locker(&mutex);
                while (queue.isEmpty()) {
                    if (stopping) return;
                    condition.wait(&mutex);
                }
                m = queue.dequeue();
            }

            char buf[1024];
            snprintf(buf, sizeof buf, "%s[%s] %s [%p]%s %s\033[0m\n",
                    m.color.toLocal8Bit().constData(),
                    m.time.toLocal8Bit().constData(),
                    m.level.toLocal8Bit().constData(),
                    m.threadId,
                    m.location.toLocal8Bit().constData(),
                    m.msg.toLocal8Bit().constData());

#ifdef Q_OS_WIN
            // Output to the IDE's debug console
            OutputDebugStringA(buf);
#else
            // Fallback for Linux/macOS
            fprintf(stderr, "%s", buf);
#endif

            if (m.type == QtFatalMsg) std::abort();
        }
    }

    void stop() {
        QMutexLocker locker(&mutex);
        stopping = true;
        condition.wakeAll();
    }

private:
    QQueue<LogMessage> queue;
    QMutex mutex;
    QWaitCondition condition;
    bool stopping = false;
};

// Global pointer to the logger (or use a singleton)
static LogWorker* g_logger = nullptr;

void myMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    if (!g_logger) return;

    LogMessage m;
    m.type = type;
    m.time = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    m.threadId = QThread::currentThreadId();

    // Setup color and level
    switch (type) {
    case QtDebugMsg:    m.color = "\033[37m";   m.level = "DEBUG"; break;
    case QtInfoMsg:     m.color = "\033[33m";   m.level = "INFO "; break;
    case QtWarningMsg:  m.color = "\033[1;33m"; m.level = "WARN "; break;
    case QtCriticalMsg: m.color = "\033[31m";   m.level = "CRIT "; break;
    case QtFatalMsg:    m.color = "\033[35m";   m.level = "FATAL"; break;
    }

    if (context.file) {
        const char *shortFile = strrchr(context.file, '/') ? strrchr(context.file, '/') + 1 :
                                    (strrchr(context.file, '\\') ? strrchr(context.file, '\\') + 1 : context.file);
        m.location = QString(" [%1:%2]").arg(shortFile).arg(context.line);
    }

    m.msg = msg;
    g_logger->enqueue(m);
}

int main(int argc, char *argv[])
{

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle("FluentWinUI3");

    QThread logThread;
    g_logger = new LogWorker();
    g_logger->moveToThread(&logThread);

    QObject::connect(&logThread, &QThread::started, g_logger, &LogWorker::process);
    logThread.start();

    auto originalMessageHandler = qInstallMessageHandler(myMessageHandler);

#ifdef DB_TEST
    qDebug("DB_TEST defined, test databases will be used");
#endif

    qRegisterMetaType<QAbstractItemModel *>();
    qmlRegisterType<DirectorySystemModel>("filebrowser", 0, 1, "DirectorySystemModel");
    qmlRegisterType<ViewController>("filebrowser", 0, 1, "ViewController");
    qmlRegisterType<PreviewData>("filebrowser", 0, 1, "PreviewData");

    FileBrowser mainCtx;
    Preferences pref(mainCtx.appDataPath()
                     , mainCtx.pathHistoryDBPath());


    qmlRegisterSingletonInstance<FileBrowser>("filebrowser", 0, 1, "FileBrowser" , &mainCtx);

    qmlRegisterSingletonInstance<Preferences>("filebrowser", 0, 1, "Preferences" , &pref);

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    int result = app.exec();

    // FIXME: even after app.exec the logger message handler was getting called
    qInstallMessageHandler(originalMessageHandler);
    g_logger->stop();
    logThread.quit();
    logThread.wait();
    delete g_logger;

    return result;
}
