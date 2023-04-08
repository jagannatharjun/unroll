#include <QObject>
#include <qtest.h>

#include "../core/archivesystem.hpp"
#include "qtestcase.h"
#include <QDir>

#include <QFile>
#include <array>
#include <QString>
#include <QDebug>

class ArchiveFileSystem : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;


private slots:

    void testArchiveFileSystem()
    {
        ArchiveSystem s;
        const auto archivepath = QFileInfo(__FILE__).dir().absoluteFilePath("archivetest.zip");
        auto f = s.open(QUrl::fromLocalFile(archivepath));


        QCOMPARE(f->name(), "archivetest.zip");
        const auto test = [&](QHash<QString, qint64> files)
        {
            QVERIFY(f);

            for (int i = 0; i < f->fileCount(); ++i) {
                QVERIFY(files.contains(f->fileName(i)));
                QCOMPARE(files[f->fileName(i)], f->fileSize(i));

                // remove the node, so duplicate can be caught
                files.remove(f->fileName(i));
            }
        };

        QHash<QString, qint64> root {{"test.txt", 13}, {"lol", 16}};
        test(root);

        f = s.open(f.get(), 0);
        QHash<QString, qint64> level2 {{"tar", 16}};
        test(level2);

        f = s.open(f.get(), 0);
        QHash<QString, qint64> level3 {{"new.txt", 16}};
        test(level3);
    }

};



QTEST_MAIN(ArchiveFileSystem)
#include "test_archivesystem.moc"
