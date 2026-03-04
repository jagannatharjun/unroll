#include <QtTest>
#include <QBuffer>
#include <memory>
#include "../core/AsyncBufferedReader.h"

class VirtualDevice : public QIODevice {
public:
    VirtualDevice(qint64 size) : m_size(size) { open(ReadOnly); }
    bool isSequential() const override { return false; }
    qint64 size() const override { return m_size; }

protected:
    // Generates a repeating pattern 'ABC...'
    qint64 readData(char *data, qint64 maxlen) override {
        qint64 startpos = pos();
        qint64 remaining = m_size - startpos;
        qint64 len = std::min(maxlen, remaining);
        for (qint64 i = 0; i < len; ++i) {
            data[i] = static_cast<char>('A' + ((startpos + i) % 26));
        }
        return len;
    }
    qint64 writeData(const char*, qint64) override { return -1; }

private:
    qint64 m_size;
};

class AsyncBufferedReaderTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testBasicRead();
    void testSeekInsideBuffer();
    void testSeekOutsideBuffer();
    void testAbortMidRead();
    void testSourceOwnership();
    void testSeekAtCapacityBoundary();
    void testCircularBufferWrap();

    void testLargeVirtualReadBenchmark() {
        const qint64 fileSize = 4 * 1024ll * 1024 * 1024; // 1 GB
        const size_t bufferCapacity = 100 * 1024 * 1024; // 10 MB
        const qint64 chunkSize = 32 * 1024; // 64 KB reads

        // Setup the reader
        AsyncBufferedReader reader(bufferCapacity);

        // Run the benchmark
        QBENCHMARK {
            reader.openSource(std::make_unique<VirtualDevice>(fileSize));

            qint64 totalRead = 0;
            while (!reader.atEnd() || reader.bytesAvailable() > 0) {
                QByteArray chunk = reader.read(chunkSize);
                totalRead += chunk.size();
            }
            QCOMPARE(totalRead, fileSize);
        }
    }

    void testLargeVirtualReadSeekBenchmark() {
        const qint64 fileSize = 4ll * 1024 * 1024 * 1024;
        const size_t bufferCapacity = 10 * 1024 * 1024;
        const int numSeeks = 4000;

        AsyncBufferedReader reader(bufferCapacity);
        reader.openSource(std::make_unique<VirtualDevice>(fileSize));

        // Seed for reproducible "random" seeks
        srand(42);

        QBENCHMARK {
            for (int i = 0; i < numSeeks; ++i) {
                qint64 targetPos = (static_cast<qint64>(rand()) % fileSize);

                // Perform the seek
                bool success = reader.seek(targetPos);
                QVERIFY(success);

                // Read a small chunk to verify data integrity at the seek point
                QByteArray data = reader.read(32 * 1024);
                if (!data.isEmpty()) {
                    char expected = static_cast<char>('A' + (targetPos % 26));
                    QCOMPARE(data[0], expected);
                }
            }
        }
        reader.abort();
    }

    void testHistoryRetention() {
        const size_t capacity = 30;
        QByteArray data = "0123456789ABCDEFGHIJKLMNOPQRST"; // 30 bytes
        AsyncBufferedReader reader(capacity);
        reader.openSource(std::make_unique<QBuffer>(&data), 0, QIODevice::ReadOnly | QIODevice::Unbuffered);

        auto count = [&reader]() {
            return reader.m_count.load();
        };

        // Wait for full buffer
        QTRY_COMPARE(count(), capacity);

        QCOMPARE(reader.read(1), data.left(1));

        QMutexLocker locker(&reader.m_mutex);
        size_t history = reader.m_count - reader.m_readLeft;
        QCOMPARE(history, (size_t)1);

        // Internal Seek back to 0
        locker.unlock();
        QVERIFY(reader.seek(0));
        // Verify no source seek happened (if you have a way to track source calls)
        QCOMPARE(reader.read(5), QByteArray("01234"));
    }

    void testSeekBackwardAfterEOF() {
        QByteArray data = "SmallData";
        AsyncBufferedReader reader(100);
        reader.openSource(std::make_unique<QBuffer>(&data));

        QCOMPARE(reader.readAll(), data);
        QVERIFY(reader.atEnd());

        // Seek back to start
        QVERIFY(reader.seek(0));
        QTRY_VERIFY(reader.bytesAvailable() > 0);
        QCOMPARE(reader.read(5), QByteArray("Small"));
    }

    void testReadEmptySource() {
        QByteArray emptyData;
        AsyncBufferedReader reader(100);
        QVERIFY(reader.openSource(std::make_unique<QBuffer>(&emptyData)));

        QTRY_VERIFY(reader.atEnd());
        QCOMPARE(reader.bytesAvailable(), (qint64)0);
        QCOMPARE(reader.read(1).size(), 0);
    }

    /**
     * Test: Multiple full cycles of the ring buffer.
     * Ensures that head and tail pointers don't drift after wrapping many times.
     */
    void testLongSequentialRead() {
        const size_t capacity = 100;
        QByteArray inputData;
        for(int i = 0; i < 1000; ++i) inputData.append(char(i % 256));

        AsyncBufferedReader reader(capacity);
        reader.openSource(std::make_unique<QBuffer>(&inputData));

        QByteArray result;
        while (!reader.atEnd() || reader.bytesAvailable() > 0) {
            if (reader.bytesAvailable() > 0) {
                result.append(reader.read(rand() % 20 + 1)); // Random small reads
            } else {
                QTest::qWait(1);
            }
        }
        QCOMPARE(result, inputData);
    }

    /**
     * Test: Seeking to the very last byte of the source.
     */
    void testSeekToEnd() {
        QByteArray inputData = "EndIsNear";
        qint64 lastIndex = inputData.size() - 1;

        AsyncBufferedReader reader(1024);
        reader.openSource(std::make_unique<QBuffer>(&inputData));

        QVERIFY(reader.seek(lastIndex));
        QTRY_COMPARE(reader.bytesAvailable(), (qint64)1);
        QCOMPARE(reader.readAll(), QByteArray("r"));
    }

    /**
     * Test: Re-opening the reader.
     * Ensure that the state is fully reset when openSource is called a second time.
     */
    void testReopening() {
        AsyncBufferedReader reader(100);

        // First run
        QByteArray data1 = "FirstRun";
        reader.openSource(std::make_unique<QBuffer>(&data1));
        QCOMPARE(reader.readAll(), data1);
        reader.abort(); // Cleanup

        // Second run
        QByteArray data2 = "SecondRun";
        reader.openSource(std::make_unique<QBuffer>(&data2));
        QCOMPARE(reader.readAll(), data2);
    }

    /**
     * Test: Zero-byte Read.
     * Ensure reader.read(0) doesn't break pointers or cause hangs.
     */
    void testZeroByteRead() {
        QByteArray data = "LogicTest";
        AsyncBufferedReader reader(100);
        reader.openSource(std::make_unique<QBuffer>(&data));

        QTRY_VERIFY(reader.bytesAvailable() > 0);
        QByteArray empty = reader.read(0);
        QCOMPARE(empty.size(), 0);
        QCOMPARE(reader.readAll(), data);
    }
};

void AsyncBufferedReaderTest::initTestCase()
{
    // Setup global state if needed
}

/**
 * Verifies that data flows from a source device into the reader
 * and can be read via the standard QIODevice API.
 */
void AsyncBufferedReaderTest::testBasicRead()
{
    QByteArray inputData = "The quick brown fox jumps over the lazy dog";
    auto source = std::make_unique<QBuffer>(&inputData);

    AsyncBufferedReader reader;
    QVERIFY(reader.openSource(std::move(source)));

    // Wait for readyRead signal

    QByteArray outputData = reader.readAll();
    QCOMPARE(outputData, inputData);
}

/**
 * Tests seeking forward within the 2MB ring buffer.
 * This should be nearly instantaneous as it just moves the head pointer.
 */
void AsyncBufferedReaderTest::testSeekInsideBuffer()
{
    QByteArray inputData(1024 * 1024, 'A'); // 1MB of data
    inputData.append("TARGET");
    auto source = std::make_unique<QBuffer>(&inputData);

    AsyncBufferedReader reader(2 * 1024 * 1024);
    reader.openSource(std::move(source));

    // Wait until at least 1MB is buffered
    auto count = [&reader]() {
        return reader.m_count.load();
    };

    QTRY_VERIFY(count() == inputData.size());

    // Seek to the "TARGET" string
    bool seekSuccess = reader.seek(1024 * 1024);
    QVERIFY(seekSuccess);

    QByteArray result = reader.read(6);
    QCOMPARE(result, QByteArray("TARGET"));
}

/**
 * Tests seeking to a position not yet buffered.
 * This forces the background worker to reset the source device position.
 */
void AsyncBufferedReaderTest::testSeekOutsideBuffer()
{
    QByteArray inputData(5 * 1024 * 1024, '0'); // 5MB total
    inputData.replace(4 * 1024 * 1024, 4, "DATA");

    auto source = std::make_unique<QBuffer>(&inputData);
    AsyncBufferedReader reader;
    reader.openSource(std::move(source));

    // Seek way past the 2MB buffer capacity
    bool seekSuccess = reader.seek(4 * 1024 * 1024);
    QVERIFY(seekSuccess);

    // Wait for the worker to catch up and buffer new data
    QSignalSpy spy(&reader, &AsyncBufferedReader::readyRead);
    QVERIFY(spy.wait(2000));

    QCOMPARE(reader.read(4), QByteArray("DATA"));
}

/**
 * Verifies that the reader shuts down cleanly when aborted.
 */
void AsyncBufferedReaderTest::testAbortMidRead()
{
    QByteArray inputData(10 * 1024 * 1024, 'X');
    auto source = std::make_unique<QBuffer>(&inputData);

    AsyncBufferedReader reader;
    reader.openSource(std::move(source));

    QTest::qWait(100); // Let it start
    reader.abort();

    QTRY_VERIFY(!reader.m_workerRunning);
}

/**
 * Checks if the source device is properly deleted by the unique_ptr
 * when the reader finishes or is destroyed.
 */
void AsyncBufferedReaderTest::testSourceOwnership()
{
    bool destroyed = false;

    // A simple mock to track destruction
    class WatchdogBuffer : public QBuffer {
        bool* d;
    public:
        WatchdogBuffer(QByteArray* b, bool* destroyed) : QBuffer(b), d(destroyed) {}
        ~WatchdogBuffer() { *d = true; }
    };

    QByteArray data = "test";
    {
        AsyncBufferedReader reader;
        reader.openSource(std::make_unique<WatchdogBuffer>(&data, &destroyed));
        QTest::qWait(50);
    } // reader goes out of scope here

    QVERIFY(destroyed);
}

/**
 * Test: Buffer Wrap-around & Throttling
 * Setting capacity to 10 bytes and feeding 25 bytes.
 * This proves the tail wraps around to the start and the producer
 * waits for the consumer to free up space.
 */
void AsyncBufferedReaderTest::testCircularBufferWrap()
{
    const size_t smallCapacity = 10;
    QByteArray inputData = "abcdefghijklmnopqrstuvwxy"; // 25 bytes
    auto source = std::make_unique<QBuffer>(&inputData);

    AsyncBufferedReader reader(smallCapacity);
    QVERIFY(reader.openSource(std::move(source)));

    // Read in small chunks to force the producer to wait and wrap
    QByteArray result;

    // Chunk 1: Read 5 bytes.
    // Reader has 10 bytes capacity, so it should fill, wait, and we drain.
    result.append(reader.read(5));

    // Chunk 2: Read another 10 bytes.
    // This forces the 'tail' to wrap around the 10-byte boundary.
    result.append(reader.read(10));

    // Chunk 3: Read the rest
    result.append(reader.readAll());

    QCOMPARE(result, inputData);
    QCOMPARE(result.size(), 25);
}

/**
 * Test: Seek Exactly to Buffer Boundary
 * If capacity is 100, and we seek to 100, we verify the pointer logic
 * handles the 'end' of the physical buffer correctly.
 */
void AsyncBufferedReaderTest::testSeekAtCapacityBoundary()
{
    const size_t capacity = 100;
    QByteArray inputData(200, 'z');
    inputData.replace(100, 5, "BOUND");

    auto source = std::make_unique<QBuffer>(&inputData);
    AsyncBufferedReader reader(capacity);
    reader.openSource(std::move(source), QIODevice::ReadOnly | QIODevice::Unbuffered);

    // Wait for buffer to be full (producer should be throttled now)
    QTRY_COMPARE_GE(reader.bytesAvailable(), (qint64)capacity);

    // Seek to index 100 (The start of the second 'physical' pass)
    QVERIFY(reader.seek(100));

    QByteArray result = reader.read(5);
    QCOMPARE(result, QByteArray("BOUND"));
}


QTEST_MAIN(AsyncBufferedReaderTest)
#include "test_AsyncBufferedReader.moc"
