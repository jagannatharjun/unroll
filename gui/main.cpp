#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QAbstractItemModel>
#include "viewcontroller.hpp"
#include "historycontroller.hpp"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QGuiApplication app(argc, argv);

    qRegisterMetaType<QAbstractItemModel *>();
    qmlRegisterType<ViewController>("filebrowser", 0, 1, "ViewController");
    qmlRegisterType<HistoryController>("filebrowser", 0, 1, "HistoryController");
    qmlRegisterType<PreviewData>("filebrowser", 0, 1, "PreviewData");

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
