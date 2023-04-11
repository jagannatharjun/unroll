#include <QObject>
#include <QTest>
#include <QSignalSpy>

#include "../core/directorysystemmodel.hpp"
#include "qtestcase.h"

#include <QJsonValue>

class TestDirectorySystemModel : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;


private slots:


    void test()
    {
        const QDir root = QFileInfo(__FILE__).dir();
        const auto archivepath = root.absoluteFilePath("archivedir");

        DirectorySystemModel m;
        QSignalSpy s(&m, &DirectorySystemModel::isLoadingChanged);

        QCOMPARE(m.isLoading(), false);
        m.open(QUrl::fromLocalFile(archivepath));
        QVERIFY(s.wait()); // wait till loading finished

        QCOMPARE(m.rowCount(), 3);

        // name : {path, size}
    }

};




QTEST_MAIN(TestDirectorySystemModel)
#include "test_directorysystemmodel.moc"
