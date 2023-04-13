#include <QObject>
#include <qtest.h>

#include "../core/archivesystem.hpp"
#include "../core/hybriddirsystem.hpp"
#include "qtestcase.h"
#include <QDir>

#include <QFile>
#include <array>
#include <QString>
#include <QDebug>

class TestArchiveFileSystem : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

    void test(DirectorySystem &s)
    {
        auto d = QDir(QFileInfo(__FILE__).dir().absoluteFilePath("archivedir"));
        const auto archivepath = d.absoluteFilePath("archivetest.zip");
        const auto archiveurl = QUrl::fromLocalFile(archivepath);

        QVERIFY(s.canopen(archiveurl));
        auto f = s.open(archiveurl);

        QCOMPARE(f->name(), "archivetest.zip");

        struct Node { QString path; qint64 size; };

        const auto test = [&](QHash<QString, Node> files)
        {
            QVERIFY(f);
            QCOMPARE(f->fileCount(), files.size());

            for (int i = 0; i < f->fileCount(); ++i) {
                QVERIFY(files.contains(f->fileName(i)));
                QCOMPARE(f->fileSize(i), files[f->fileName(i)].size);
                QCOMPARE(f->filePath(i), files[f->fileName(i)].path);

                // remove the node, so duplicate can be caught
                files.remove(f->fileName(i));
            }
        };


        d = QDir(archivepath);
        QHash<QString, Node> root
            {
                {"test.txt", {d.absoluteFilePath("test.txt"), 13}},
                {"lol", {d.absoluteFilePath("lol"), 16}}
            };
        test(root);

        QCOMPARE(f->fileName(0), "lol");
        f = s.open(f.get(), 0);
        QHash<QString, Node> level2 {{"tar", {d.absoluteFilePath("lol/tar"), 16}}};
        test(level2);


        QCOMPARE(f->fileName(0), "tar");
        QHash<QString, Node> level3 {{"new.txt", {d.absoluteFilePath("lol/tar/new.txt"),16}}};

        auto old = std::move(f);
        f = s.open(old->fileUrl(0));
        test(level3);

        f = s.open(old.get(), 0);
        test(level3);
    }

private slots:
    void testArchiveFileSystem()
    {
        ArchiveSystem s;
        test(s);
    }

    void testHybridFileSystem()
    {
        HybridDirSystem s;
        test(s);
    }
};



QTEST_MAIN(TestArchiveFileSystem)
#include "test_archivesystem.moc"
