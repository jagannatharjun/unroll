#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include "../core/ArchiveIODevice.h"
#include <archive.h>
#include <archive_entry.h>

class TestArchiveIODevice : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // 10 comprehensive test cases
    void test1_BasicOpenReadClose();
    void test2_InvalidOperations();
    void test3_SimpleSeekAndRead();
    void test4_ExtensiveRandomSeeking();
    void test5_BackwardForwardSeekPatterns();
    void test6_BoundaryConditions();
    void test7_ChunkedReadingWithSeeks();
    void test8_ContentVerificationAfterSeeks();
    void test9_StressTestMultipleSeeks();
    void test10_EdgeCasesAndErrorHandling();

private:
    void createTestArchive(const QString &archivePath, const QMap<QString, QByteArray> &files);
    QByteArray generatePatternedData(int size, int blockSize = 256);
    void verifyDataAtPosition(ArchiveIODevice &device,
                              qint64 pos,
                              const QByteArray &expected,
                              int length);

    QTemporaryDir *m_tempDir;
    QString m_testArchive;
    QByteArray m_patternedData; // For content verification
    static constexpr int LARGE_FILE_SIZE = 100000;
};

void TestArchiveIODevice::initTestCase()
{
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());

    // Generate patterned data for verification
    m_patternedData = generatePatternedData(LARGE_FILE_SIZE, 256);

    // Create test archive with multiple files
    m_testArchive = m_tempDir->filePath("test.rar");
    QMap<QString, QByteArray> testFiles;
    testFiles["small.txt"] = "Hello, World!";
    testFiles["empty.txt"] = "";
    testFiles["binary.bin"] = QByteArray("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09", 10);
    testFiles["large.dat"] = m_patternedData;
    testFiles["subdir/nested.txt"] = "Nested content here";
    testFiles["unicode_тест_文件.txt"] = "Unicode filename content";
    createTestArchive(m_testArchive, testFiles);
}

void TestArchiveIODevice::cleanupTestCase()
{
    delete m_tempDir;
}

void TestArchiveIODevice::test1_BasicOpenReadClose()
{
    // Test basic functionality: constructor, open, read, close, properties
    ArchiveIODevice device(m_testArchive, "small.txt");

    // Before opening
    QVERIFY(!device.isOpen());
    QCOMPARE(device.isSequential(), false);
    QCOMPARE(device.size(), 0);
    QCOMPARE(device.pos(), 0);

    // Open and verify
    QVERIFY(device.open(QIODevice::ReadOnly));
    QVERIFY(device.isOpen());
    QCOMPARE(device.size(), 13);
    QCOMPARE(device.pos(), 0);

    // Read and verify content
    QByteArray data = device.readAll();
    QCOMPARE(data, QByteArray("Hello, World!"));
    QCOMPARE(device.pos(), 13);
    QVERIFY(device.atEnd());

    // Close and verify cleanup
    device.close();
    QVERIFY(!device.isOpen());
    QCOMPARE(device.pos(), 0);

    // Test multiple open modes
    QVERIFY(!device.open(QIODevice::WriteOnly));
    QVERIFY(!device.open(QIODevice::ReadWrite));

    // Test write always fails
    device.open(QIODevice::ReadOnly);
    QCOMPARE(device.write("test"), -1);
    device.close();
}

void TestArchiveIODevice::test2_InvalidOperations()
{
    // Test error conditions and edge cases

    // Non-existent archive
    ArchiveIODevice device1("/nonexistent/path/archive.zip", "file.txt");
    QVERIFY(!device1.open(QIODevice::ReadOnly));
    QVERIFY(!device1.isOpen());

    // Non-existent entry in valid archive
    ArchiveIODevice device2(m_testArchive, "nonexistent.txt");
    QVERIFY(!device2.open(QIODevice::ReadOnly));

    // Operations on closed device
    ArchiveIODevice device3(m_testArchive, "small.txt");
    QVERIFY(device3.read(100).isEmpty());
    QVERIFY(!device3.seek(5));

    // Double open
    device3.open(QIODevice::ReadOnly);
    QVERIFY(!device3.open(QIODevice::ReadOnly)); // Should fail
    device3.close();

    // Empty file
    ArchiveIODevice device4(m_testArchive, "empty.txt");
    QVERIFY(device4.open(QIODevice::ReadOnly));
    QCOMPARE(device4.size(), 0);
    QVERIFY(device4.readAll().isEmpty());
    // QVERIFY(device4.seek(0));
    QVERIFY(!device4.seek(1)); // Beyond empty file
    device4.close();

    // Unicode and nested paths
    ArchiveIODevice device5(m_testArchive, "unicode_тест_文件.txt");
    QVERIFY(device5.open(QIODevice::ReadOnly));
    QCOMPARE(device5.readAll(), QByteArray("Unicode filename content"));
    device5.close();

    ArchiveIODevice device6(m_testArchive, "subdir/nested.txt");
    QVERIFY(device6.open(QIODevice::ReadOnly));
    QCOMPARE(device6.readAll(), QByteArray("Nested content here"));
    device6.close();
}

void TestArchiveIODevice::test3_SimpleSeekAndRead()
{
    // Test basic seeking operations with content verification
    ArchiveIODevice device(m_testArchive, "large.dat");
    QVERIFY(device.open(QIODevice::ReadOnly));
    QCOMPARE(device.size(), (qint64) LARGE_FILE_SIZE);

    // Seek to beginning
    QVERIFY(device.seek(0));
    QCOMPARE(device.pos(), 0);
    QByteArray data1 = device.read(256);
    QCOMPARE(data1, m_patternedData.mid(0, 256));

    // Seek to middle
    QVERIFY(device.seek(50000));
    QCOMPARE(device.pos(), 50000);
    QByteArray data2 = device.read(256);
    QCOMPARE(data2, m_patternedData.mid(50000, 256));

    // Seek to near end
    QVERIFY(device.seek(LARGE_FILE_SIZE - 256));
    QCOMPARE(device.pos(), LARGE_FILE_SIZE - 256);
    QByteArray data3 = device.read(256);
    QCOMPARE(data3, m_patternedData.mid(LARGE_FILE_SIZE - 256, 256));

    // Seek to exact end
    QVERIFY(device.seek(LARGE_FILE_SIZE));
    QCOMPARE(device.pos(), (qint64) LARGE_FILE_SIZE);
    QVERIFY(device.read(1).isEmpty());

    // Invalid seeks
    QVERIFY(!device.seek(-1));
    QVERIFY(!device.seek(LARGE_FILE_SIZE + 1));

    // Seek back to start
    QVERIFY(device.seek(0));
    QCOMPARE(device.pos(), 0);

    device.close();
}

void TestArchiveIODevice::test4_ExtensiveRandomSeeking()
{
    // Test many random seeks with full content verification
    ArchiveIODevice device(m_testArchive, "large.dat");
    QVERIFY(device.open(QIODevice::ReadOnly));

    // Perform 200 random seeks and verify content
    QRandomGenerator rng(42); // Fixed seed for reproducibility
    for (int i = 0; i < 200; i++) {
        qint64 pos = rng.bounded(LARGE_FILE_SIZE - 100);
        int readLen = rng.bounded(10, 100);

        QVERIFY(device.seek(pos));
        QCOMPARE(device.pos(), pos);

        QByteArray data = device.read(readLen);
        QCOMPARE(data.size(), readLen);
        QCOMPARE(data, m_patternedData.mid(pos, readLen));
        QCOMPARE(device.pos(), pos + readLen);
    }

    device.close();
}

void TestArchiveIODevice::test5_BackwardForwardSeekPatterns()
{
    // Test various seek patterns: backward, forward, zigzag
    ArchiveIODevice device(m_testArchive, "large.dat");
    QVERIFY(device.open(QIODevice::ReadOnly));

    // Forward sequential seeks
    for (int i = 0; i < 10; i++) {
        qint64 pos = i * 10000;
        QVERIFY(device.seek(pos));
        verifyDataAtPosition(device, pos, m_patternedData, 100);
    }

    // Backward sequential seeks
    for (int i = 9; i >= 0; i--) {
        qint64 pos = i * 10000;
        QVERIFY(device.seek(pos));
        verifyDataAtPosition(device, pos, m_patternedData, 100);
    }

    // Zigzag pattern
    QList<qint64> positions = {10000, 5000, 20000, 15000, 30000, 25000, 40000, 35000};
    for (qint64 pos : positions) {
        QVERIFY(device.seek(pos));
        verifyDataAtPosition(device, pos, m_patternedData, 100);
    }

    // Large jumps
    QList<qint64> jumpPositions = {0, 99000, 1000, 98000, 50000, 75000, 25000};
    for (qint64 pos : jumpPositions) {
        QVERIFY(device.seek(pos));
        QByteArray data = device.read(50);
        QCOMPARE(data, m_patternedData.mid(pos, 50));
    }

    device.close();
}

void TestArchiveIODevice::test6_BoundaryConditions()
{
    // Test edge cases at file boundaries
    ArchiveIODevice device(m_testArchive, "large.dat");
    QVERIFY(device.open(QIODevice::ReadOnly));

    // Read at position 0
    QVERIFY(device.seek(0));
    QByteArray firstByte = device.read(1);
    QCOMPARE(firstByte.size(), 1);
    QCOMPARE(firstByte, m_patternedData.mid(0, 1));

    // Read at last position
    QVERIFY(device.seek(LARGE_FILE_SIZE - 1));
    QByteArray lastByte = device.read(1);
    QCOMPARE(lastByte.size(), 1);
    QCOMPARE(lastByte, m_patternedData.mid(LARGE_FILE_SIZE - 1, 1));

    // Read at EOF
    QVERIFY(device.seek(LARGE_FILE_SIZE));
    QByteArray atEOF = device.read(1);
    QVERIFY(atEOF.isEmpty());

    // Read spanning to EOF
    QVERIFY(device.seek(LARGE_FILE_SIZE - 50));
    QByteArray spanning = device.read(100);
    QCOMPARE(spanning.size(), 50);
    QCOMPARE(spanning, m_patternedData.mid(LARGE_FILE_SIZE - 50, 50));

    // Multiple reads at same boundary
    for (int i = 0; i < 5; i++) {
        QVERIFY(device.seek(LARGE_FILE_SIZE - 10));
        QByteArray boundary = device.read(10);
        QCOMPARE(boundary.size(), 10);
        QCOMPARE(boundary, m_patternedData.mid(LARGE_FILE_SIZE - 10, 10));
    }

    // Seek to 0 multiple times
    for (int i = 0; i < 5; i++) {
        QVERIFY(device.seek(0));
        QCOMPARE(device.pos(), 0);
        QByteArray start = device.read(10);
        QCOMPARE(start, m_patternedData.mid(0, 10));
    }

    device.close();
}

void TestArchiveIODevice::test7_ChunkedReadingWithSeeks()
{
    // Test reading file in chunks with seeks between chunks
    ArchiveIODevice device(m_testArchive, "large.dat");
    QVERIFY(device.open(QIODevice::ReadOnly));

    // Read every 1000 bytes with 500-byte chunks
    for (qint64 offset = 0; offset < LARGE_FILE_SIZE; offset += 1000) {
        QVERIFY(device.seek(offset));

        qint64 chunkSize = qMin(500LL, LARGE_FILE_SIZE - offset);
        QByteArray chunk = device.read(chunkSize);

        QCOMPARE(chunk.size(), chunkSize);
        QCOMPARE(chunk, m_patternedData.mid(offset, chunkSize));
        QCOMPARE(device.pos(), offset + chunkSize);
    }

    // Read overlapping chunks
    for (qint64 offset = 0; offset < LARGE_FILE_SIZE - 2000; offset += 1500) {
        QVERIFY(device.seek(offset));
        QByteArray chunk = device.read(2000);
        QCOMPARE(chunk.size(), 2000);
        QCOMPARE(chunk, m_patternedData.mid(offset, 2000));
    }

    // Read specific sections in random order
    QList<qint64> sections = {80000, 20000, 60000, 40000, 0, 90000};
    for (qint64 section : sections) {
        QVERIFY(device.seek(section));
        QByteArray chunk = device.read(1000);
        qint64 expectedSize = qMin(1000LL, LARGE_FILE_SIZE - section);
        QCOMPARE(chunk.size(), expectedSize);
        QCOMPARE(chunk, m_patternedData.mid(section, expectedSize));
    }

    device.close();
}

void TestArchiveIODevice::test8_ContentVerificationAfterSeeks()
{
    // Intensive content verification with complex seek patterns
    ArchiveIODevice device(m_testArchive, "large.dat");
    QVERIFY(device.open(QIODevice::ReadOnly));

    // Pattern 1: Read forward with varying step sizes
    qint64 pos = 0;
    while (pos < LARGE_FILE_SIZE - 1000) {
        QVERIFY(device.seek(pos));

        int readSize = ((pos / 1000) % 5 + 1) * 100; // Varying sizes
        QByteArray data = device.read(readSize);

        QCOMPARE(data.size(), readSize);
        QCOMPARE(data, m_patternedData.mid(pos, readSize));

        pos += ((pos / 1000) % 10 + 1) * 500; // Varying steps
    }

    // Pattern 2: Read specific pattern positions
    for (int block = 0; block < LARGE_FILE_SIZE / 256; block += 13) {
        qint64 blockPos = block * 256;
        QVERIFY(device.seek(blockPos));

        QByteArray blockData = device.read(256);
        int expectedSize = qMin(256, LARGE_FILE_SIZE - (int) blockPos);
        QCOMPARE(blockData.size(), expectedSize);
        QCOMPARE(blockData, m_patternedData.mid(blockPos, expectedSize));
    }

    // Pattern 3: Verify specific known patterns in the data
    for (int i = 0; i < 50; i++) {
        qint64 patternPos = i * 2000;
        if (patternPos >= LARGE_FILE_SIZE)
            break;

        QVERIFY(device.seek(patternPos));
        QByteArray pattern = device.read(16);

        // Verify against known pattern structure
        QCOMPARE(pattern, m_patternedData.mid(patternPos, 16));
    }

    // Pattern 4: Read and verify non-aligned positions
    for (int i = 0; i < 100; i++) {
        qint64 oddPos = i * 997; // Prime number for odd positions
        if (oddPos >= LARGE_FILE_SIZE - 100)
            break;

        QVERIFY(device.seek(oddPos));
        QByteArray data = device.read(73); // Odd size
        QCOMPARE(data.size(), 73);
        QCOMPARE(data, m_patternedData.mid(oddPos, 73));
    }

    device.close();
}

void TestArchiveIODevice::test9_StressTestMultipleSeeks()
{
    // Stress test with many operations
    ArchiveIODevice device(m_testArchive, "large.dat");
    QVERIFY(device.open(QIODevice::ReadOnly));

    QElapsedTimer timer;
    timer.start();

    // Perform 1000 seek-read operations
    QRandomGenerator rng(12345);
    int successCount = 0;

    for (int i = 0; i < 1000; i++) {
        qint64 pos = rng.bounded(LARGE_FILE_SIZE - 50);
        int readLen = rng.bounded(10, 50);

        if (device.seek(pos)) {
            QByteArray data = device.read(readLen);
            if (data == m_patternedData.mid(pos, readLen)) {
                successCount++;
            }
        }
    }

    qint64 elapsed = timer.elapsed();
    qDebug() << "1000 seek-read operations took:" << elapsed << "ms";
    qDebug() << "Success rate:" << successCount << "/ 1000";

    QCOMPARE(successCount, 1000);
    QVERIFY(elapsed < 30000); // Should complete in reasonable time

    // Test repeated seeks to same positions
    for (int i = 0; i < 100; i++) {
        qint64 pos = 50000;
        QVERIFY(device.seek(pos));
        QByteArray data = device.read(100);
        QCOMPARE(data, m_patternedData.mid(pos, 100));
    }

    // Test rapid position changes
    for (int i = 0; i < 50; i++) {
        QVERIFY(device.seek(i * 2000));
        device.read(10);
        QVERIFY(device.seek((49 - i) * 2000));
        device.read(10);
    }

    device.close();
}

void TestArchiveIODevice::test10_EdgeCasesAndErrorHandling()
{
    // Test error conditions and recovery
    ArchiveIODevice device(m_testArchive, "large.dat");
    QVERIFY(device.open(QIODevice::ReadOnly));

    // Test seeking after reading to EOF
    device.readAll();
    QCOMPARE(device.pos(), (qint64) LARGE_FILE_SIZE);
    QVERIFY(device.seek(0));
    QByteArray afterEOF = device.read(100);
    QCOMPARE(afterEOF, m_patternedData.mid(0, 100));

    // Test zero-length reads
    QVERIFY(device.seek(5000));
    QByteArray zero = device.read(0);
    QVERIFY(zero.isEmpty());
    QCOMPARE(device.pos(), 5000); // Position shouldn't change

    // Test negative read lengths (Qt handles this)
    QVERIFY(device.seek(1000));
    QByteArray negative = device.read(-10);
    QVERIFY(negative.isEmpty());

    // Test sequential reads without seeking
    QVERIFY(device.seek(10000));
    QByteArray seq1 = device.read(100);
    QByteArray seq2 = device.read(100);
    QByteArray seq3 = device.read(100);
    QCOMPARE(seq1, m_patternedData.mid(10000, 100));
    QCOMPARE(seq2, m_patternedData.mid(10100, 100));
    QCOMPARE(seq3, m_patternedData.mid(10200, 100));

    // Test that position is maintained correctly
    qint64 currentPos = device.pos();
    QCOMPARE(currentPos, 10300);

    // Test reading after failed seek
    QVERIFY(!device.seek(-100));
    qint64 posAfterFail = device.pos();
    QByteArray afterFail = device.read(50);
    QVERIFY(!afterFail.isEmpty()); // Should still be able to read

    // Test multiple failed operations
    for (int i = 0; i < 10; i++) {
        QVERIFY(!device.seek(LARGE_FILE_SIZE + 1000));
    }

    // Verify device still works after failed operations
    QVERIFY(device.seek(0));
    QByteArray recovery = device.read(100);
    QCOMPARE(recovery, m_patternedData.mid(0, 100));

    device.close();

    // Test operations on closed device
    QVERIFY(device.read(100).isEmpty());
    QVERIFY(!device.seek(0));
    QVERIFY(!device.isOpen());

    // Test binary data integrity
    ArchiveIODevice binDevice(m_testArchive, "binary.bin");
    QVERIFY(binDevice.open(QIODevice::ReadOnly));
    QByteArray binData = binDevice.readAll();
    QCOMPARE(binData, QByteArray("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09", 10));
    binDevice.close();
}

QByteArray TestArchiveIODevice::generatePatternedData(int size, int blockSize)
{
    QByteArray data;
    data.reserve(size);

    for (int i = 0; i < size; i += blockSize) {
        int currentBlockSize = qMin(blockSize, size - i);

        for (int j = 0; j < currentBlockSize; j++) {
            // Create a pattern based on position
            int blockNum = i / blockSize;
            char value = (blockNum * 17 + j * 3) % 256;
            data.append(value);
        }
    }

    return data;
}

void TestArchiveIODevice::verifyDataAtPosition(ArchiveIODevice &device,
                                               qint64 pos,
                                               const QByteArray &expected,
                                               int length)
{
    QByteArray data = device.read(length);
    QCOMPARE(data.size(), length);
    QCOMPARE(data, expected.mid(pos, length));
    QCOMPARE(device.pos(), pos + length);
}

void TestArchiveIODevice::createTestArchive(const QString &archivePath,
                                            const QMap<QString, QByteArray> &files)
{
    // Initialize the archive object
    struct archive *a = archive_write_new();
    if (!a) {
        // Handle memory allocation failure
        return;
    }

    // Set the format (Required: libarchive needs a format and/or filter)
    if (archive_write_set_format_zip(a) != ARCHIVE_OK) {
        qDebug() << "Failed to set format:" << archive_error_string(a);
        archive_write_free(a);
        return;
    }

    // Open the file
    if (archive_write_open_filename(a, archivePath.toLocal8Bit().constData()) != ARCHIVE_OK) {
        qDebug() << "Failed to open archive for writing:" << archive_error_string(a);
        archive_write_free(a);
        return;
    }

    for (auto it = files.constBegin(); it != files.constEnd(); ++it) {
        struct archive_entry *entry = archive_entry_new();
        if (!entry)
            continue;

        archive_entry_set_pathname(entry, it.key().toUtf8().constData());
        archive_entry_set_size(entry, it.value().size());
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0644);

        // Write the header
        int r = archive_write_header(a, entry);
        if (r < ARCHIVE_OK) {
            qDebug() << "Header error:" << archive_error_string(a);
        }

        if (r >= ARCHIVE_WARN) {
            // Write the data
            if (archive_write_data(a, it.value().constData(), it.value().size()) < 0) {
                qDebug() << "Data write error:" << archive_error_string(a);
            }
        }

        archive_entry_free(entry);

        if (r == ARCHIVE_FATAL)
            break;
    }

    // Finalize and cleanup
    archive_write_close(a);
    archive_write_free(a);
}

QTEST_MAIN(TestArchiveIODevice)
#include "test_ArchiveIODevice.moc"
