#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QAbstractItemModel>

#include "../core/directorysystemmodel.hpp"
#include "filebrowser.hpp"
#include "viewcontroller.hpp"
#include "historycontroller.hpp"
#include "preferences.hpp"
#include "ffmpegframeextractor.h"

#include <QQuickStyle>

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle("Fusion");

    qRegisterMetaType<QAbstractItemModel *>();
    qmlRegisterType<DirectorySystemModel>("filebrowser", 0, 1, "DirectorySystemModel");
    qmlRegisterType<ViewController>("filebrowser", 0, 1, "ViewController");
    qmlRegisterType<HistoryController>("filebrowser", 0, 1, "HistoryController");
    qmlRegisterType<PreviewData>("filebrowser", 0, 1, "PreviewData");
    qmlRegisterType<FrameRenderer>("filebrowser", 0, 1, "FrameRenderer");

    qmlRegisterSingletonType<FileBrowser>("filebrowser", 0, 1, "FileBrowser" ,[](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject *
    {
        return new FileBrowser();
    });

    qmlRegisterSingletonType<Preferences>("filebrowser", 0, 1, "Preferences" ,[](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject *
    {
        return new Preferences();
    });

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
