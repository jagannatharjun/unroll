#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QAbstractItemModel>

#include "../core/directorysystemmodel.hpp"
#include "filebrowser.hpp"
#include "viewcontroller.hpp"
#include "historycontroller.hpp"
#include "preferences.hpp"

#include <QQuickStyle>

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle("Fusion");

#ifdef DB_TEST
    qDebug("DB_TEST defined, test databases will be used");
#endif

    qRegisterMetaType<QAbstractItemModel *>();
    qmlRegisterType<DirectorySystemModel>("filebrowser", 0, 1, "DirectorySystemModel");
    qmlRegisterType<ViewController>("filebrowser", 0, 1, "ViewController");
    qmlRegisterType<HistoryController>("filebrowser", 0, 1, "HistoryController");
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
