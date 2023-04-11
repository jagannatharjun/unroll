#include <QObject>
#include <qtest.h>

#include "../core/filesystem.hpp"
#include "../core/hybriddirsystem.hpp"
#include "qtestcase.h"
#include <QDir>

#include <QFile>
#include <array>
#include <QString>

class TestFileSystem : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

    void test(DirectorySystem &system)
    {
        QString testdir = "./test-dir/";
        QDir d(testdir);
        if (d.exists())
            d.removeRecursively();

        d.mkpath(d.absolutePath());


        QByteArray buf;
        const auto write = [&](const QString f, int s) {
            QFile io(d.absoluteFilePath(f));
            assert(io.open(QIODevice::WriteOnly));

            buf.resize(s);
            for (int i = 0; i < buf.size(); ++i)
                buf[i] = rand() % 256;

            io.write(buf);
        };

        std::array<std::pair<QString, int>, 3> testfiles =
            {
                std::pair {"this is a file", 98},
                {"this is a second file", 87},
                {"thirdfile", 69}
            };

        for (const auto &f : testfiles)
            write(f.first, f.second);

        auto fd = system.open(QUrl::fromLocalFile("./test-dir/"));

        QCOMPARE(fd->fileCount(), testfiles.size());

        std::vector<bool> v(testfiles.size(), false);

        for (int i = 0; i < fd->fileCount(); ++i) {
            for (int j = 0; j < testfiles.size(); ++j) {
                const auto &f = testfiles[j];
                if (fd->fileName(i) == f.first) {
                    QVERIFY(!fd->isDir(i));
                    QCOMPARE(fd->fileSize(i), f.second);
                    QVERIFY(!v[j]);

                    v[j] = true;
                }
            }
        }

    }

private slots:
    void testFileSystem()
    {
        FileSystem s;
        test(s);
    }

    void testHybridSystem()
    {
        HybridDirSystem s;
        test(s);
    }


};



QTEST_MAIN(TestFileSystem)
#include "test_filesystem.moc"
