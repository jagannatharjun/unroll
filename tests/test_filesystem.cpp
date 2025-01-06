#include <QObject>
#include <qtest.h>

#include "../core/filesystem.hpp"
#include "../core/hybriddirsystem.hpp"
#include "qtestcase.h"
#include <QDir>

#include <QFile>
#include <QString>


QString withoutTrallingBacklash(const QString &path)
{
    if (path.endsWith('/'))
        return path.chopped(1);
    return path;
}

class TestFileSystem : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

    const QString testDir = "./test-dir/";
    const QString testDirLevel2 = "level2/";

    // following only contains files
    const QList<std::pair<QString, int>> testFiles =
        {
            std::pair {"this is a file", 98},
            {"this is a second file", 87},
            {"thirdfile", 69}
        };

    // following only contains files
    const QList<std::pair<QString, int>> testfilesLevel2 =
        {
            std::pair {"this is a file2", 198},
            {"this is a second file123", 187},
            {"thirdfileOfLevel2", 169},
            {"thirdfile2", 169}
        };

    const QList<std::pair<QString, int>> level1
        = QList{std::pair{testDirLevel2, 0}} + testFiles;

    void check(Directory *fd, const QList<std::pair<QString, int>> &files)
    {
        QDir d(testDir);
        QCOMPARE(fd->fileCount(), files.size());

        QList<bool> v(files.size(), false);

        for (int i = 0; i < fd->fileCount(); ++i) {
            for (int j = 0; j < files.size(); ++j) {
                const auto &f = files[j];
                const bool isDir = f.first.endsWith('/');
                const QString name = isDir ? f.first.chopped(1) : f.first;
                if (fd->fileName(i) == name) {
                    QCOMPARE(fd->isDir(i), isDir);
                    QCOMPARE(fd->fileSize(i), f.second);
                    QVERIFY(!v[j]);

                    v[j] = true;

                    // don't break, all files must be unique
                }
            }
        }

        for (int i = 0; i < v.size(); ++i) {
            const QString msg =
                QString("failed to find,i: %1, '%2'")
                    .arg(QString::number(i), d.absoluteFilePath(files[i].first));

            QVERIFY2(v[i] , qPrintable(msg));
        }
    };

    void test(DirectorySystem &system)
    {
        QDir d(testDir);

        const auto url = QUrl::fromLocalFile("./test-dir/");
        auto fd = system.open(url);
        check(fd.get(), level1);

        QCOMPARE(fd->filePath(0), withoutTrallingBacklash(d.absoluteFilePath(testDirLevel2)));
        auto fd2 = system.open(fd.get(), 0);
        check(fd2.get(), testfilesLevel2);
    }

private slots:

    void initTestCase()
    {
        QDir d(testDir);
        if (d.exists())
            d.removeRecursively();

        d.mkpath(d.absolutePath());
        d.mkpath(d.absoluteFilePath(testDirLevel2));

        QByteArray buf;
        const auto write = [&](const QString f, int s)
        {
            QFile io(d.absoluteFilePath(f));
            QVERIFY(io.open(QIODevice::WriteOnly));

            buf.resize(s);
            for (int i = 0; i < buf.size(); ++i)
                buf[i] = rand() % 256;

            io.write(buf);
        };

        for (const auto &f : testFiles)
            write(f.first, f.second);

        for (const auto &f : testfilesLevel2)
            write(testDirLevel2 + f.first, f.second);
    }

    void testFileSystem()
    {
        FileSystem s;
        test(s);
    }

    void testFileSystemLean()
    {
        FileSystem s;

        const auto leanfiles = testFiles + testfilesLevel2;

        const auto dir = u"./test-dir/"_qs;
        auto fd = s.leanOpen(dir);
        check(fd.get(), leanfiles);

        {
            auto fd2 = s.leanOpen(fd->path());
            check(fd2.get(), leanfiles);
        }

        auto fd3 = s.open(fd->url());
        check(fd3.get(), level1);

        auto fd4 = s.leanOpen(dir);
        check(fd4.get(), leanfiles);
    }

    void testHybridSystem()
    {
        HybridDirSystem s;
        test(s);
    }

};



QTEST_MAIN(TestFileSystem)
#include "test_filesystem.moc"
