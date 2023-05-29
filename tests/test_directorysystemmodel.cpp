#include <QObject>
#include <QTest>

#include "../core/directorysystemmodel.hpp"
#include "../core/hybriddirsystem.hpp"
#include "qtestcase.h"


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
        HybridDirSystem system;


        m.setDirectory(system.open(QUrl::fromLocalFile(archivepath)));
        QCOMPARE(m.rowCount(), 4);

        // name : {path, size}
    }

};




QTEST_MAIN(TestDirectorySystemModel)
#include "test_directorysystemmodel.moc"
