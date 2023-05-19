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

        QVERIFY(!s.open(QUrl::fromLocalFile("notexistentfile"))); // test with url that doesn't exist
        QVERIFY(!s.open(QUrl::fromLocalFile(__FILE__))); // test with not a archive file

        QVERIFY(s.canopen(archiveurl));
        auto f = s.open(archiveurl);

        QCOMPARE(f->name(), "archivetest.zip");

        struct Node { QString path; qint64 size; QByteArray content {}; };

        const auto test = [&](QHash<QString, Node> files, Directory *d = nullptr)
        {
            if (!d)
                d = f.get();

            QVERIFY(d);
            QCOMPARE(d->fileCount(), files.size());

            for (int i = 0; i < d->fileCount(); ++i) {
                QVERIFY(files.contains(d->fileName(i)));

                const auto &node = files[d->fileName(i)];
                QCOMPARE(d->fileSize(i), node.size);
                QCOMPARE(d->filePath(i), node.path);
                if (!node.content.isNull())
                {
                    QString path;

                    {
                        auto iosource = s.iosource(d, i);
                        path = iosource->readPath();

                        QFile readsource(path);
                        QVERIFY(readsource.open(QIODevice::ReadOnly));

                        const QByteArray content = readsource.readAll();
                        QCOMPARE(content, node.content);
                    }

                    QVERIFY(!QFileInfo::exists(path));
                }

                // remove the node, so duplicate can be caught
                files.remove(d->fileName(i));
            }
        };


        d = QDir(archivepath);
        QHash<QString, Node> root
            {
                {"test.txt", {d.absoluteFilePath("test.txt"), 13}},
                {"lol", {d.absoluteFilePath("lol"), 16}}
            };
        test(root);

        QCOMPARE(f->fileName(1), "test.txt");
        QVERIFY(!s.open(f.get(), 1)); // try to open file

        QCOMPARE(f->fileName(0), "lol");
        f = s.open(f.get(), 0);
        QHash<QString, Node> level2 {{"tar", {d.absoluteFilePath("lol/tar"), 16}}};
        test(level2);

        auto second = s.open(f->url());
        test(level2, second.get());

        QCOMPARE(f->fileName(0), "tar");
        QHash<QString, Node> level3 {{"new.txt", {d.absoluteFilePath("lol/tar/new.txt"),16, "lolpoisonutrypop"}}};

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
