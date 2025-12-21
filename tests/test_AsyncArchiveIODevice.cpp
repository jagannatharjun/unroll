// tst_asyncarchiveiodevice.cpp
#include <QBuffer>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
#include "../core/asyncarchiveiodevice.h"
#include <archive.h>
#include <archive_entry.h>

namespace {
static const auto dump = [](QString f, QByteArray d) {
    QFile fd(f);
    if (!fd.open(QIODevice::WriteOnly | QIODevice::Truncate))
        qWarning("failed to open dump file");

    fd.write(d);
};
} // namespace

class TestAsyncArchiveIODevice : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();

    // Comprehensive tests
    void test01_BasicSequentialRead();
    void test02_RandomAccessSeek();
    void test03_ReadAtEOF();
    void test04_MultipleSmallReads();
    void test05_ReadLargerThanBuffer();
    void test06_SeekAndReadPattern();
    void test07_BytesAvailableProgression();
    void test08_ReopenAfterClose();
    void test09_ConcurrentIODevices();
    void test10_QDataStreamIntegration();

private:
    QString createTestArchive(const QString &archiveName, const QMap<QString, QByteArray> &files);
    QByteArray generateTestData(qint64 size);
    QByteArray generatePatternData(qint64 size);

    QTemporaryDir m_tempDir;
};

void TestAsyncArchiveIODevice::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
}

void TestAsyncArchiveIODevice::cleanupTestCase() {}

QString TestAsyncArchiveIODevice::createTestArchive(const QString &archiveName,
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

QByteArray TestAsyncArchiveIODevice::generateTestData(qint64 size)
{
    QByteArray data;
    data.reserve(size);

    for (qint64 i = 0; i < size; ++i) {
        data.append(static_cast<char>('A' + (i % 26)));
        // data.append(i);
    }

    return data;
}

QByteArray TestAsyncArchiveIODevice::generatePatternData(qint64 size)
{
    QByteArray data;
    data.reserve(size);

    // Create a repeating 4-byte pattern for easy verification
    for (qint64 i = 0; i < size; i += 4) {
        quint32 value = static_cast<quint32>(i / 4);
        data.append(reinterpret_cast<const char *>(&value), qMin(4LL, size - i));
    }

    return data;
}

// Test 1: Basic sequential reading from start to end
void TestAsyncArchiveIODevice::test01_BasicSequentialRead()
{
    QByteArray testData = "The quick brown fox jumps over the lazy dog. "
                          "This is test data for sequential reading.";
    QMap<QString, QByteArray> files;
    files["sequential.txt"] = testData;

    QString archive = createTestArchive("test01.tar", files);

    AsyncArchiveIODevice device(archive, "sequential.txt", testData.size());
    QVERIFY(device.open(QIODevice::ReadOnly));

    QCOMPARE(device.size(), testData.size());
    QVERIFY(device.isSequential() == false);
    QCOMPARE(device.pos(), 0);

    // first read is greater than 0
    QVERIFY(device.bytesAvailable() > 0);

    // Read entire file
    QTRY_COMPARE(device.bytesAvailable(), testData.size());
    QByteArray result = device.readAll();
    QCOMPARE(result, testData);
    QCOMPARE(device.pos(), testData.size());
    QVERIFY(device.atEnd());

    device.close();
    QVERIFY(!device.isOpen());
}

// Test 2: Random access with multiple seeks and reads
void TestAsyncArchiveIODevice::test02_RandomAccessSeek()
{
    QByteArray testData = generatePatternData(10000);
    QMap<QString, QByteArray> files;
    files["random.dat"] = testData;

    QString archive = createTestArchive("test02.tar", files);

    AsyncArchiveIODevice device(archive, "random.dat", testData.size());
    QVERIFY(device.open(QIODevice::ReadOnly));

    // Test seeking to various positions
    struct TestSeek
    {
        qint64 pos;
        qint64 readSize;
    };

    QVector<TestSeek> seekTests = {
        {0, 100},    // Start
        {5000, 200}, // Middle
        {9900, 100}, // Near end
        {2500, 150}, // Back to earlier position
        {9000, 500}, // Read past end (should get partial)
    };

    for (const auto &test : seekTests) {
        QVERIFY(device.seek(test.pos));
        QCOMPARE(device.pos(), test.pos);

        qint64 expectedSize = qMin(test.readSize, testData.size() - test.pos);
        QTRY_VERIFY(device.bytesAvailable() >= expectedSize);

        QByteArray chunk = device.read(test.readSize);
        QCOMPARE(chunk.size(), expectedSize);
        QCOMPARE(chunk, testData.mid(test.pos, expectedSize));
    }

    device.close();
}

// Test 3: Reading at and past EOF
void TestAsyncArchiveIODevice::test03_ReadAtEOF()
{
    QByteArray testData = "Small file for EOF testing.";
    QMap<QString, QByteArray> files;
    files["eof.txt"] = testData;

    QString archive = createTestArchive("test03.tar", files);

    AsyncArchiveIODevice device(archive, "eof.txt", testData.size());
    QVERIFY(device.open(QIODevice::ReadOnly));

    QTRY_VERIFY(device.bytesAvailable() >= testData.size());
    // Read to EOF
    QByteArray all = device.readAll();
    QCOMPARE(all, testData);
    QVERIFY(device.atEnd());
    QCOMPARE(device.pos(), testData.size());

    // Try reading past EOF
    QByteArray empty = device.read(100);
    QVERIFY(empty.isEmpty());
    QVERIFY(device.atEnd());

    // Seek past EOF (should fail or adjust)
    bool seekResult = device.seek(testData.size() + 1000);
    QVERIFY(!seekResult || device.pos() == testData.size());

    // Seek back and verify we can read again
    QVERIFY(device.seek(0));
    QTRY_VERIFY(device.bytesAvailable() >= 10);

    QByteArray reread = device.read(10);
    QCOMPARE(reread, testData.left(10));

    device.close();
}

// Test 4: Multiple small consecutive reads
void TestAsyncArchiveIODevice::test04_MultipleSmallReads()
{
    QByteArray testData = generateTestData(5000);
    QMap<QString, QByteArray> files;
    files["small_reads.dat"] = testData;

    QString archive = createTestArchive("test04.tar", files);

    AsyncArchiveIODevice device(archive, "small_reads.dat", testData.size());
    QVERIFY(device.open(QIODevice::ReadOnly));

    QByteArray accumulated;
    const qint64 chunkSize = 250;

    while (!device.atEnd()) {
        QByteArray chunk = device.read(chunkSize);
        QVERIFY(!chunk.isEmpty() || device.atEnd());
        accumulated.append(chunk);

        // Verify position advances correctly
        QCOMPARE(device.pos(), accumulated.size());
    }

    QCOMPARE(accumulated, testData);
    QCOMPARE(accumulated.size(), testData.size());

    device.close();
}

// Test 5: Read larger chunks than internal buffer
void TestAsyncArchiveIODevice::test05_ReadLargerThanBuffer()
{
    // Create 5MB file
    QByteArray testData = generateTestData(5 * 1024 * 1024);
    QMap<QString, QByteArray> files;
    files["large.dat"] = testData;

    QString archive = createTestArchive("test05.tar", files);

    AsyncArchiveIODevice device(archive, "large.dat", testData.size());
    QVERIFY(device.open(QIODevice::ReadOnly));

    // Read in 500KB chunks (likely larger than internal buffer)
    QByteArray accumulated;
    const qint64 chunkSize = 500 * 1024;

    while (!device.atEnd()) {
        QByteArray chunk = device.read(chunkSize);
        accumulated.append(chunk);

        // Verify we're making progress
        QVERIFY(chunk.size() > 0 || device.atEnd());
    }

    QCOMPARE(accumulated.size(), testData.size());
    QCOMPARE(accumulated, testData);

    device.close();
}

// Test 6: Complex seek and read pattern
void TestAsyncArchiveIODevice::test06_SeekAndReadPattern()
{
    const QByteArray testData = generatePatternData(20000);
    QMap<QString, QByteArray> files;
    files["pattern.dat"] = testData;

    QString archive = createTestArchive("test06.tar", files);

    AsyncArchiveIODevice device(archive, "pattern.dat", testData.size());
    QVERIFY(device.open(QIODevice::ReadOnly));

    // Complex access pattern: forward, backward, skip, etc.
    struct Access
    {
        qint64 seekPos;
        qint64 readSize;
        QString description;
    };

    QVector<Access> pattern = {
        {0, 1000, "Read from start"},
        {10000, 500, "Jump to middle"},
        {5000, 1500, "Seek backward"},
        {19000, 2000, "Read past end"},
        {500, 100, "Small read near start"},
        {15000, 3000, "Read from middle to past end"},
        {0, 20000, "Read entire file from start"},
    };

    for (const auto &access : pattern) {
        qInfo() << "seek test" << access.description;
        QVERIFY2(device.seek(access.seekPos),
                 qPrintable(QString("Seek failed for: %1").arg(access.description)));

        qint64 expectedSize = qMin(access.readSize, testData.size() - access.seekPos);
        QTRY_VERIFY(device.bytesAvailable() >= expectedSize);
        QByteArray chunk = device.read(access.readSize); // pass more size then expected

        QCOMPARE(chunk.size(), expectedSize);
        auto expectedData = testData.mid(access.seekPos, expectedSize);
        QCOMPARE(chunk.size(), expectedData.size());
        dump("E:\\", chunk);
        dump("E:\\expectedData", expectedData);
        QCOMPARE(chunk, expectedData);
    }

    device.close();
}

// Test 7: bytesAvailable() as data becomes available
void TestAsyncArchiveIODevice::test07_BytesAvailableProgression()
{
    QByteArray testData = generateTestData(100000);
    QMap<QString, QByteArray> files;
    files["available.dat"] = testData;

    QString archive = createTestArchive("test07.tar", files);

    AsyncArchiveIODevice device(archive, "available.dat", testData.size());
    QVERIFY(device.open(QIODevice::ReadOnly));

    // Initially, bytesAvailable should be 0 or growing
    qint64 initialAvailable = device.bytesAvailable();
    QVERIFY(initialAvailable >= 0);

    // Wait a bit for data to buffer
    QTest::qWait(100);

    qint64 afterWait = device.bytesAvailable();
    QVERIFY(afterWait >= initialAvailable);

    // Read some data
    QByteArray chunk1 = device.read(1000);
    QCOMPARE(chunk1.size(), 1000);

    // bytesAvailable should account for position
    qint64 afterRead = device.bytesAvailable();
    QVERIFY(afterRead >= 0);

    // Read everything
    QByteArray rest = device.readAll();
    QCOMPARE(chunk1 + rest, testData);

    // At EOF, bytesAvailable should be 0
    QCOMPARE(device.bytesAvailable(), 0);
    QVERIFY(device.atEnd());

    device.close();
}

// Test 8: Reopen device after closing
void TestAsyncArchiveIODevice::test08_ReopenAfterClose()
{
    QByteArray testData = "Reopen test data with some content.";
    QMap<QString, QByteArray> files;
    files["reopen.txt"] = testData;

    QString archive = createTestArchive("test08.tar", files);

    AsyncArchiveIODevice device(archive, "reopen.txt", testData.size());

    // First open and read
    QVERIFY(device.open(QIODevice::ReadOnly));
    QByteArray firstRead = device.read(10);
    QCOMPARE(firstRead, testData.left(10));
    device.close();
    QVERIFY(!device.isOpen());

    // Reopen and read from start
    QVERIFY(device.open(QIODevice::ReadOnly));
    QCOMPARE(device.pos(), 0);
    QByteArray secondRead = device.readAll();
    QCOMPARE(secondRead, testData);
    device.close();

    // Third open, seek and read
    QVERIFY(device.open(QIODevice::ReadOnly));
    QVERIFY(device.seek(5));
    QByteArray thirdRead = device.read(10);
    QCOMPARE(thirdRead, testData.mid(5, 10));
    device.close();
}

// Test 9: Multiple IODevices on same archive
void TestAsyncArchiveIODevice::test09_ConcurrentIODevices()
{
    QByteArray data1 = generateTestData(50000);
    QByteArray data2 = generateTestData(50000);
    data2[0] = 'X'; // Make them different

    QMap<QString, QByteArray> files;
    files["file1.dat"] = data1;
    files["file2.dat"] = data2;

    QString archive = createTestArchive("test09.tar", files);

    // Create two devices on different files in same archive
    AsyncArchiveIODevice device1(archive, "file1.dat", data1.size());
    AsyncArchiveIODevice device2(archive, "file2.dat", data2.size());

    QVERIFY(device1.open(QIODevice::ReadOnly));
    QVERIFY(device2.open(QIODevice::ReadOnly));

    // Read from both alternately
    QByteArray result1, result2;
    const qint64 chunkSize = 5000;

    while (!device1.atEnd() || !device2.atEnd()) {
        if (!device1.atEnd()) {
            QTRY_VERIFY(device1.bytesAvailable() > 0);
            result1.append(device1.read(chunkSize));
        }
        if (!device2.atEnd()) {
            QTRY_VERIFY(device2.bytesAvailable() > 0);
            result2.append(device2.read(chunkSize));
        }
    }

    QCOMPARE(result1, data1);
    QCOMPARE(result2, data2);

    device1.close();
    device2.close();

    // Now test same file, different positions
    AsyncArchiveIODevice device3(archive, "file1.dat", data1.size());
    AsyncArchiveIODevice device4(archive, "file1.dat", data1.size());

    QVERIFY(device3.open(QIODevice::ReadOnly));
    QVERIFY(device4.open(QIODevice::ReadOnly));

    // Seek to different positions
    QVERIFY(device3.seek(10000));
    QVERIFY(device4.seek(30000));

    QTRY_VERIFY(device3.bytesAvailable() > 1000);
    QByteArray chunk3 = device3.read(1000);

    QTRY_VERIFY(device4.bytesAvailable() > 1000);
    QByteArray chunk4 = device4.read(1000);

    QCOMPARE(chunk3, data1.mid(10000, 1000));
    QCOMPARE(chunk4, data1.mid(30000, 1000));

    device3.close();
    device4.close();
}

// Test 10: Integration with QDataStream for structured reading
void TestAsyncArchiveIODevice::test10_QDataStreamIntegration()
{
    // Create binary data with structured content
    QByteArray testData;
    QDataStream writeStream(&testData, QIODevice::WriteOnly);

    // Write various data types
    writeStream << qint32(42);
    writeStream << qint64(123456789012345LL);
    writeStream << double(3.14159265359);
    writeStream << QString("Hello, Archive!");
    writeStream << QByteArray("Binary data here");

    QStringList list;
    list << "Item1" << "Item2" << "Item3";
    writeStream << list;

    QMap<QString, qint32> map;
    map["one"] = 1;
    map["two"] = 2;
    map["three"] = 3;
    writeStream << map;

    // Create archive
    QMap<QString, QByteArray> files;
    files["structured.dat"] = testData;

    QString archive = createTestArchive("test10.tar", files);

    // Read back using QDataStream
    AsyncArchiveIODevice device(archive, "structured.dat", testData.size());
    QVERIFY(device.open(QIODevice::ReadOnly));

    QDataStream readStream(&device);

    qint32 int32Val;
    qint64 int64Val;
    double doubleVal;
    QString stringVal;
    QByteArray byteArrayVal;
    QStringList listVal;
    QMap<QString, qint32> mapVal;

    readStream >> int32Val;
    readStream >> int64Val;
    readStream >> doubleVal;
    readStream >> stringVal;
    readStream >> byteArrayVal;
    readStream >> listVal;
    readStream >> mapVal;

    // Verify all values
    QCOMPARE(int32Val, 42);
    QCOMPARE(int64Val, 123456789012345LL);
    QCOMPARE(doubleVal, 3.14159265359);
    QCOMPARE(stringVal, QString("Hello, Archive!"));
    QCOMPARE(byteArrayVal, QByteArray("Binary data here"));
    QCOMPARE(listVal, list);
    QCOMPARE(mapVal, map);

    QVERIFY(device.atEnd());
    device.close();

    // Test partial reads with QDataStream
    QVERIFY(device.open(QIODevice::ReadOnly));
    QDataStream partialStream(&device);

    qint32 firstInt;
    partialStream >> firstInt;
    QCOMPARE(firstInt, 42);

    // Seek forward and read string
    qint64 stringPos = sizeof(qint32) + sizeof(qint64) + sizeof(double);
    QVERIFY(device.seek(stringPos));

    QDataStream seekStream(&device);
    QString seekString;
    seekStream >> seekString;
    QCOMPARE(seekString, QString("Hello, Archive!"));

    device.close();
}

QTEST_MAIN(TestAsyncArchiveIODevice)
#include "test_AsyncArchiveIODevice.moc"
