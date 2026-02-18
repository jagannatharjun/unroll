#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QAbstractItemModel>

#include "../core/directorysystemmodel.hpp"
#include "filebrowser.hpp"
#include "viewcontroller.hpp"
#include "preferences.hpp"

#include <QQuickStyle>

#include <QDebug>
#include <QDateTime>
#include <QThread>
#include <iostream>

void myMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QString time = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");

    // ANSI Color Codes
    const char *reset   = "\033[0m";
    const char *white   = "\033[37m"; // Debug
    const char *yellow  = "\033[33m"; // Info
    const char *boldYel = "\033[1;33m"; // Warning (Bold Yellow)
    const char *red     = "\033[31m"; // Critical
    const char *magenta = "\033[35m"; // Fatal

    const char *color = white;
    const char *level = "DEBUG";

    // Adjusting colors based on your request
    switch (type) {
    case QtDebugMsg:    color = white;   level = "DEBUG"; break;
    case QtInfoMsg:     color = yellow;  level = "INFO "; break;
    case QtWarningMsg:  color = boldYel; level = "WARN "; break;
    case QtCriticalMsg: color = red;     level = "CRIT "; break;
    case QtFatalMsg:    color = magenta; level = "FATAL"; break;
    }

    // Omit file/line if unknown
    QString location = "";
    if (context.file && strlen(context.file) > 0) {
        const char *shortFile = strrchr(context.file, '/') ? strrchr(context.file, '/') + 1 :
                                    (strrchr(context.file, '\\') ? strrchr(context.file, '\\') + 1 : context.file);

        location = QString(" [%1:%2]").arg(shortFile).arg(context.line);
    }

    // Print the whole line in the chosen color
    fprintf(stderr, "%s[%s] %s [%p]%s %s%s\n",
            color,
            time.toLocal8Bit().constData(),
            level,
            QThread::currentThreadId(),
            location.toLocal8Bit().constData(),
            msg.toLocal8Bit().constData(),
            reset);

    if (type == QtFatalMsg) abort();
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(myMessageHandler);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle("FluentWinUI3");

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

    return app.exec();
}
