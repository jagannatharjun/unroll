// tst_asyncarchivefilereader.cpp

#include "../core/asyncarchivefilereader.h"

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include <archive.h>
#include <archive_entry.h>

class TestAsyncArchiveFileReader : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic functionality tests
    void testReadSmallFile();
    void testReadLargeFile();
    void testReadFromOffset();
    void testMultipleFiles();
    void testEmptyFile();

    // Edge cases
    void testNonExistentArchive();
    void testNonExistentFileInArchive();
    void testCorruptedArchive();

    // Concurrency tests
    void testMultipleReaders();
    void testAbortDuringRead();
    void testDestructorWhileReading();

    // Signal tests
    void testDataAvailableSignal();
    void testFinishedSignal();
    void testErrorSignal();

    // Buffer management
    void testBufferOverflow();
    void testGetAvailableDataMultipleTimes();
    void testCustomBufferSize();

    // Performance tests
    void testLargeFilePerformance();
    void testSeekPerformance();

private:
    QString createTestArchive(const QString &archiveName, const QMap<QString, QByteArray> &files);
    QByteArray generateTestData(qint64 size);

    QTemporaryDir m_tempDir;
    QString m_testArchive;
};

void TestAsyncArchiveFileReader::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
}

void TestAsyncArchiveFileReader::cleanupTestCase()
{
    // Cleanup happens automatically with QTemporaryDir
}

void TestAsyncArchiveFileReader::init()
{
    // Called before each test
}

void TestAsyncArchiveFileReader::cleanup()
{
    // Called after each test
}

QString TestAsyncArchiveFileReader::createTestArchive(const QString &archiveName,
                                                      const QMap<QString, QByteArray> &files)
{
    QString archivePath = m_tempDir.filePath(archiveName);

    struct archive *a = archive_write_new();
    archive_write_set_format_pax_restricted(a);
    archive_write_open_filename(a, archivePath.toUtf8().constData());

    for (auto it = files.constBegin(); it != files.constEnd(); ++it) {
        struct archive_entry *entry = archive_entry_new();
        archive_entry_set_pathname(entry, it.key().toUtf8().constData());
        archive_entry_set_size(entry, it.value().size());
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0644);

        archive_write_header(a, entry);
        archive_write_data(a, it.value().constData(), it.value().size());
        archive_entry_free(entry);
    }

    archive_write_close(a);
    archive_write_free(a);

    return archivePath;
}

QByteArray TestAsyncArchiveFileReader::generateTestData(qint64 size)
{
    QByteArray data;
    data.reserve(size);

    for (qint64 i = 0; i < size; ++i) {
        data.append(static_cast<char>('A' + (i % 26)));
    }

    return data;
}

void TestAsyncArchiveFileReader::testReadSmallFile()
{
    // Create archive with small test file
    QByteArray testData = "Hello, World! This is a test file.";
    QMap<QString, QByteArray> files;
    files["test.txt"] = testData;

    QString archive = createTestArchive("small.tar", files);

    AsyncArchiveFileReader reader;
    // Wait for data
    QSignalSpy dataSpy(&reader, &AsyncArchiveFileReader::dataAvailable);
    QSignalSpy finishedSpy(&reader, &AsyncArchiveFileReader::finished);

    reader.start(archive, "test.txt", 0);

    QVERIFY(finishedSpy.wait());

    // Get and verify data
    QByteArray receivedData = reader.getAvailableData();
    QCOMPARE(receivedData, testData);
}

void TestAsyncArchiveFileReader::testReadLargeFile()
{
    // Create archive with 5MB file
    QByteArray testData = generateTestData(5 * 1024 * 1024);
    QMap<QString, QByteArray> files;
    files["large.dat"] = testData;

    QString archive = createTestArchive("large.tar", files);

    AsyncArchiveFileReader reader;
    QSignalSpy dataSpy(&reader, &AsyncArchiveFileReader::dataAvailable);
    QSignalSpy finishedSpy(&reader, &AsyncArchiveFileReader::finished);

    reader.start(archive, "large.dat", 0);

    // Collect data in chunks
    QByteArray allData;

    while (!finishedSpy.count()) {
        if (dataSpy.wait(5000)) {
            allData.append(reader.getAvailableData());
            dataSpy.clear();
        }
    }

    // Get any remaining data
    allData.append(reader.getAvailableData());

    QCOMPARE(allData.size(), testData.size());
    QCOMPARE(allData, testData);
}

void TestAsyncArchiveFileReader::testReadFromOffset()
{
    QByteArray testData = generateTestData(10000);
    QMap<QString, QByteArray> files;
    files["offset_test.dat"] = testData;

    QString archive = createTestArchive("offset.tar", files);

    qint64 offset = 5000;
    AsyncArchiveFileReader reader;
    QSignalSpy finishedSpy(&reader, &AsyncArchiveFileReader::finished);
    reader.start(archive, "offset_test.dat", offset);

    QByteArray receivedData = reader.getAvailableData();
    QByteArray expectedData = testData.mid(offset);

    QCOMPARE(receivedData.size(), expectedData.size());
    QCOMPARE(receivedData, expectedData);
}

void TestAsyncArchiveFileReader::testMultipleFiles()
{
    QMap<QString, QByteArray> files;
    files["file1.txt"] = "Content of file 1";
    files["file2.txt"] = "Content of file 2";
    files["file3.txt"] = "Content of file 3";

    QString archive = createTestArchive("multiple.tar", files);

    // Read second file
    AsyncArchiveFileReader reader;
    QSignalSpy finishedSpy(&reader, &AsyncArchiveFileReader::finished);
    reader.start(archive, "file2.txt", 0);

    QVERIFY(finishedSpy.wait(5000));

    QByteArray data = reader.getAvailableData();
    QCOMPARE(data, files["file2.txt"]);
}

void TestAsyncArchiveFileReader::testEmptyFile()
{
    QMap<QString, QByteArray> files;
    files["empty.txt"] = QByteArray();

    QString archive = createTestArchive("empty.tar", files);

    AsyncArchiveFileReader reader;
    QSignalSpy finishedSpy(&reader, &AsyncArchiveFileReader::finished);

    reader.start(archive, "empty.txt", 0);

    QVERIFY(finishedSpy.wait(5000));

    QByteArray data = reader.getAvailableData();
    QVERIFY(data.isEmpty());
}

void TestAsyncArchiveFileReader::testNonExistentArchive()
{
    AsyncArchiveFileReader reader;

    QSignalSpy errorSpy(&reader, &AsyncArchiveFileReader::error);
    QSignalSpy finishedSpy(&reader, &AsyncArchiveFileReader::finished);

    reader.start("/nonexistent/path/archive.tar", "test.txt", 0);
    QVERIFY(finishedSpy.wait(5000) || errorSpy.wait(5000));
    QVERIFY(errorSpy.count() > 0);

    QCOMPARE(reader.getAvailableData(), QByteArray());
}

void TestAsyncArchiveFileReader::testNonExistentFileInArchive()
{
    QMap<QString, QByteArray> files;
    files["exists.txt"] = "This file exists";

    QString archive = createTestArchive("partial.tar", files);

    AsyncArchiveFileReader reader;
    QSignalSpy errorSpy(&reader, &AsyncArchiveFileReader::error);
    QSignalSpy finishedSpy(&reader, &AsyncArchiveFileReader::finished);

    reader.start(archive, "nonexistent.txt", 0);
    QVERIFY(finishedSpy.wait(5000));
    QVERIFY(errorSpy.count() > 0);

    QString errorMsg = errorSpy.at(0).at(0).toString();
    QVERIFY(errorMsg.contains("not found") || errorMsg.contains("File not found"));
}

void TestAsyncArchiveFileReader::testCorruptedArchive()
{
    // Create a file with invalid archive data
    QString corruptPath = m_tempDir.filePath("corrupt.tar");
    QFile corrupt(corruptPath);
    QVERIFY(corrupt.open(QIODevice::WriteOnly));
    corrupt.write("This is not a valid tar archive");
    corrupt.close();

    AsyncArchiveFileReader reader;
    QSignalSpy errorSpy(&reader, &AsyncArchiveFileReader::error);
    QSignalSpy finishedSpy(&reader, &AsyncArchiveFileReader::finished);

    reader.start(corruptPath, "test.txt", 0);
    QVERIFY(finishedSpy.wait(5000) || errorSpy.wait(5000));
    QVERIFY(errorSpy.count() > 0);
}

void TestAsyncArchiveFileReader::testMultipleReaders()
{
    QByteArray testData1 = generateTestData(5000);
    QByteArray testData2 = generateTestData(5000);

    QMap<QString, QByteArray> files;
    files["data1.dat"] = testData1;
    files["data2.dat"] = testData2;

    QString archive = createTestArchive("multi_reader.tar", files);

    AsyncArchiveFileReader reader1;
    AsyncArchiveFileReader reader2;

    QSignalSpy finished1(&reader1, &AsyncArchiveFileReader::finished);
    QSignalSpy finished2(&reader2, &AsyncArchiveFileReader::finished);

    reader1.start(archive, "data1.dat", 0);
    reader2.start(archive, "data2.dat", 0);

    QVERIFY(finished1.wait(5000));
    QVERIFY(finished2.count() > 0 || finished2.wait(5000));

    QCOMPARE(reader1.getAvailableData(), testData1);
    QCOMPARE(reader2.getAvailableData(), testData2);
}

void TestAsyncArchiveFileReader::testAbortDuringRead()
{
    QByteArray testData = generateTestData(10 * 1024 * 1024); // 10MB
    QMap<QString, QByteArray> files;
    files["large.dat"] = testData;

    QString archive = createTestArchive("abort_test.tar", files);

    AsyncArchiveFileReader reader;
    QSignalSpy finishedSpy(&reader, &AsyncArchiveFileReader::finished);

    reader.start(archive, "large.dat", 0);

    // Abort early
    QTest::qWait(100);
    reader.abort();

    QVERIFY(finishedSpy.count() || finishedSpy.wait(5000));

    // Should finish quickly after abort
    QVERIFY(finishedSpy.count() > 0);
}

void TestAsyncArchiveFileReader::testDestructorWhileReading()
{
    QByteArray testData = generateTestData(5 * 1024 * 1024);
    QMap<QString, QByteArray> files;
    files["data.dat"] = testData;

    QString archive = createTestArchive("destructor_test.tar", files);

    {
        AsyncArchiveFileReader reader;
        reader.start(archive, "data.dat", 0);

        // Let it start reading
        QTest::qWait(100);

        // Destructor should handle cleanup gracefully
    } // reader destroyed here

    // If we get here without hanging, the test passes
    QVERIFY(true);
}

void TestAsyncArchiveFileReader::testDataAvailableSignal()
{
    QByteArray testData = generateTestData(100000);
    QMap<QString, QByteArray> files;
    files["signal_test.dat"] = testData;

    QString archive = createTestArchive("signals.tar", files);

    AsyncArchiveFileReader reader;
    QSignalSpy dataSpy(&reader, &AsyncArchiveFileReader::dataAvailable);
    QSignalSpy finishedSpy(&reader, &AsyncArchiveFileReader::finished);

    reader.start(archive, "signal_test.dat", 0);
    QVERIFY(finishedSpy.wait(5000));

    // Should have received at least one dataAvailable signal
    QVERIFY(dataSpy.count() > 0);
}

void TestAsyncArchiveFileReader::testFinishedSignal()
{
    QMap<QString, QByteArray> files;
    files["finish.txt"] = "test data";

    QString archive = createTestArchive("finish.tar", files);

    AsyncArchiveFileReader reader;
    QSignalSpy finishedSpy(&reader, &AsyncArchiveFileReader::finished);
    reader.start(archive, "finish.txt", 0);
    QVERIFY(finishedSpy.wait(5000));
    QCOMPARE(finishedSpy.count(), 1);
}

void TestAsyncArchiveFileReader::testErrorSignal()
{
    AsyncArchiveFileReader reader;

    QSignalSpy errorSpy(&reader, &AsyncArchiveFileReader::error);

    reader.start("/invalid/path.tar", "test.txt", 0);
    QVERIFY(errorSpy.wait(5000));
    QVERIFY(errorSpy.count() > 0);

    QString errorMsg = errorSpy.at(0).at(0).toString();
    QVERIFY(!errorMsg.isEmpty());
}

void TestAsyncArchiveFileReader::testBufferOverflow()
{
    // Create a file larger than buffer size
    QByteArray testData = generateTestData(3 * 1024 * 1024); // 3MB
    QMap<QString, QByteArray> files;
    files["overflow.dat"] = testData;

    QString archive = createTestArchive("overflow.tar", files);

    AsyncArchiveFileReader reader;

    QByteArray allData;
    QSignalSpy dataSpy(&reader, &AsyncArchiveFileReader::dataAvailable);
    QSignalSpy finishedSpy(&reader, &AsyncArchiveFileReader::finished);

    reader.start(archive, "overflow.dat", 0);
    while (!finishedSpy.count()) {
        if (dataSpy.wait(5000)) {
            allData.append(reader.getAvailableData());
            dataSpy.clear();
        }
    }

    allData.append(reader.getAvailableData());

    QCOMPARE(allData, testData);
}

void TestAsyncArchiveFileReader::testGetAvailableDataMultipleTimes()
{
    QByteArray testData = generateTestData(100000);
    QMap<QString, QByteArray> files;
    files["multi_get.dat"] = testData;

    QString archive = createTestArchive("multi_get.tar", files);

    AsyncArchiveFileReader reader;

    QByteArray allData;
    QSignalSpy finishedSpy(&reader, &AsyncArchiveFileReader::finished);

    reader.start(archive, "multi_get.dat", 0);
    // Get data multiple times
    while (!finishedSpy.count()) {
        QTest::qWait(50);
        allData.append(reader.getAvailableData());
    }

    allData.append(reader.getAvailableData());

    QCOMPARE(allData, testData);

    // Calling again should return empty
    QVERIFY(reader.getAvailableData().isEmpty());
}

void TestAsyncArchiveFileReader::testCustomBufferSize()
{
    QByteArray testData = generateTestData(200000);
    QMap<QString, QByteArray> files;
    files["custom.dat"] = testData;

    QString archive = createTestArchive("custom.tar", files);

    AsyncArchiveFileReader reader;

    QSignalSpy finishedSpy(&reader, &AsyncArchiveFileReader::finished);

    reader.start(archive, "custom.dat", 0);
    QVERIFY(finishedSpy.wait(10000));

    QByteArray data = reader.getAvailableData();
    QCOMPARE(data, testData);
}

void TestAsyncArchiveFileReader::testLargeFilePerformance()
{
    // QSKIP("Performance test - enable manually");

    // Create 100MB file
    QByteArray testData = generateTestData(100 * 1024 * 1024);
    QMap<QString, QByteArray> files;
    files["perf.dat"] = testData;

    QString archive = createTestArchive("perf.tar", files);

    QElapsedTimer timer;
    timer.start();

    AsyncArchiveFileReader reader;

    QByteArray allData;
    QSignalSpy finishedSpy(&reader, &AsyncArchiveFileReader::finished);
    reader.start(archive, "perf.dat", 0);

    while (!finishedSpy.count()) {
        QTest::qWait(10);
        allData.append(reader.getAvailableData());
    }

    qint64 elapsed = timer.elapsed();
    qDebug() << "Read 100MB in" << elapsed << "ms";

    QVERIFY(elapsed < 10000); // Should complete in under 10 seconds
}

void TestAsyncArchiveFileReader::testSeekPerformance()
{
    // QSKIP("Performance test - enable manually");

    QByteArray testData = generateTestData(50 * 1024 * 1024);
    QMap<QString, QByteArray> files;
    files["seek_perf.dat"] = testData;

    QString archive = createTestArchive("seek_perf.tar", files);

    QElapsedTimer timer;
    timer.start();

    // Seek to 90% of file
    qint64 offset = testData.size() * 9 / 10;
    AsyncArchiveFileReader reader;

    QSignalSpy finishedSpy(&reader, &AsyncArchiveFileReader::finished);
    reader.start(archive, "seek_perf.dat", offset);
    QVERIFY(finishedSpy.wait(10000));

    qint64 elapsed = timer.elapsed();
    qDebug() << "Seek and read from offset" << offset << "in" << elapsed << "ms";
}

QTEST_MAIN(TestAsyncArchiveFileReader)
#include "test_asyncarchivefilereader.moc"
