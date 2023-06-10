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

    void matchArchiveTestTree(std::shared_ptr<Directory> &root, const QDir d, DirectorySystem &system)
    {
        QCOMPARE(root->name(), "archivetest.zip");

        std::shared_ptr<Directory> current = root;

        struct Node { QString path; qint64 size; QByteArray content {}; };

        const auto test = [&](QHash<QString, Node> files, Directory *d = nullptr)
        {
            if (!d)
                d = current.get();

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
                        QCOMPARE(d->isDir(i), false);

                        auto iosource = system.iosource(d, i);
                        QVERIFY(iosource);

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


        QHash<QString, Node> level0
        {
            {"test.txt", {d.absoluteFilePath("test.txt"), 13}},
            {"lol", {d.absoluteFilePath("lol"), 16}}
        };

        test(level0);

        QCOMPARE(current->fileName(1), "test.txt");
        QVERIFY(!system.open(current.get(), 1)); // try to open file

        QCOMPARE(current->fileName(0), "lol");
        QCOMPARE(current->isDir(0), true);
        current = system.open(current.get(), 0);

        QHash<QString, Node> level2 {{"tar", {d.absoluteFilePath("lol/tar"), 16}}};
        test(level2);

        auto second = system.open(current->url());
        QVERIFY(second);
        QCOMPARE(second->url(), current->url());
        QCOMPARE(second->name(), current->name());
        test(level2, second.get());

        auto secondpath = system.open(current->path());
        QVERIFY(secondpath);
        QCOMPARE(secondpath->url(), second->url());
        QCOMPARE(secondpath->path(), second->path());
        QCOMPARE(secondpath->name(), second->name());
        test(level2, secondpath.get());


        QCOMPARE(current->fileName(0), "tar");
        QHash<QString, Node> level3 {{"new.txt", {d.absoluteFilePath("lol/tar/new.txt"),16, "lolpoisonutrypop"}}};

        auto old = std::move(current);
        current = system.open(old->fileUrl(0));
        test(level3);

        current = system.open(old.get(), 0);
        test(level3);
    }

    void test(DirectorySystem &s)
    {
        auto d = QDir(QFileInfo(__FILE__).dir().absoluteFilePath("archivedir"));
        const auto archivepath = d.absoluteFilePath("archivetest.zip");
        const auto archiveurl = QUrl::fromLocalFile(archivepath);

        QVERIFY(!s.open(QUrl::fromLocalFile("notexistentfile"))); // test with url that doesn't exist
        QVERIFY(!s.open(QUrl::fromLocalFile(__FILE__))); // test with not a archive file

        auto f = std::shared_ptr(s.open(archiveurl));

        QCOMPARE(f->name(), "archivetest.zip");

        d = QDir(archivepath);
        matchArchiveTestTree(f, d, s);

        // test reproducibility
        f = s.open(f->url());
        QVERIFY(f);

        matchArchiveTestTree(f, d, s);
    }

    void testRecursiveArchive(DirectorySystem &s)
    {
        auto d = QDir(QFileInfo(__FILE__).dir().absoluteFilePath("archivedir"));
        const auto archivepath = d.absoluteFilePath("archivedir.zip");
        const auto archiveurl = QUrl::fromLocalFile(archivepath);

        auto f = s.open(archiveurl);
        QVERIFY(f);

        QCOMPARE(f->fileName(0), "archivetest.zip");

        auto recurRoot = std::shared_ptr(s.open(f->fileUrl(0)));
        QVERIFY(recurRoot);

        d = QDir(QDir(archivepath).absoluteFilePath("archivetest.zip"));
        matchArchiveTestTree(recurRoot, d, s);

        auto second = std::shared_ptr(s.open(recurRoot->url()));
        QVERIFY(second);
        matchArchiveTestTree(second, d, s);

        auto p = std::shared_ptr(s.open(second->path()));
        QVERIFY(p);
        matchArchiveTestTree(p, d, s);
    }

private slots:
    void testArchiveFileSystem()
    {
        ArchiveSystem s;
        test(s);
    }

    void testRecursiveArchive()
    {
        ArchiveSystem s;
        testRecursiveArchive(s);
    }

    void testHybridFileSystem()
    {
        HybridDirSystem s;
        test(s);
    }
};



QTEST_MAIN(TestArchiveFileSystem)
#include "test_archivesystem.moc"
