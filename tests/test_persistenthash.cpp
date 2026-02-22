#include <QObject>
#include <qtest.h>
#include "../core/persistenthash.hpp"
#include <QTemporaryDir>
#include <QFile>
#include <QString>

// Test structure for serialization
struct TestData
{
    int row = 0;
    int col = 0;

    bool operator==(const TestData &other) const
    {
        return row == other.row && col == other.col;
    }

    friend QDataStream &operator<<(QDataStream &stream, const TestData &data)
    {
        stream << data.row << data.col;
        return stream;
    }

    friend QDataStream &operator>>(QDataStream &stream, TestData &data)
    {
        stream >> data.row >> data.col;
        return stream;
    }
};

class TestPersistentHash : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // Ensure clean state for tests
        m_tempDir = std::make_unique<QTemporaryDir>();
        QVERIFY(m_tempDir->isValid());
    }

    void cleanupTestCase()
    {
        m_tempDir.reset();
    }

    void testConstructorWithDefaultName()
    {
        // Test default database name
        PersistentHash<TestData> hash;
        QVERIFY(hash.lastError().isEmpty() || hash.lastError().startsWith("Key not found"));
    }

    void testConstructorWithCustomName()
    {
        // Test custom database name
        QString dbPath = m_tempDir->path() + "/test.db";
        PersistentHash<TestData> hash(dbPath);
        QVERIFY(hash.lastError().isEmpty() || hash.lastError().startsWith("Key not found"));
    }

    void testInsertAndRetrieve()
    {
        QString dbPath = m_tempDir->path() + "/insert_retrieve.db";
        PersistentHash<TestData> hash(dbPath);

        // Insert a value
        TestData insertData{42, 100};
        bool result = hash.insert("key1", insertData);
        QVERIFY(result);

        // Retrieve the value
        TestData retrieveData;
        result = hash.value("key1", retrieveData);
        QVERIFY(result);
        QCOMPARE(retrieveData.row, 42);
        QCOMPARE(retrieveData.col, 100);
    }

    void testInsertMultipleValues()
    {
        QString dbPath = m_tempDir->path() + "/multiple_values.db";
        PersistentHash<TestData> hash(dbPath);

        // Insert multiple values
        TestData data1{1, 10};
        TestData data2{2, 20};
        TestData data3{3, 30};

        QVERIFY(hash.insert("key1", data1));
        QVERIFY(hash.insert("key2", data2));
        QVERIFY(hash.insert("key3", data3));

        // Retrieve and verify all values
        TestData result1, result2, result3;
        QVERIFY(hash.value("key1", result1));
        QVERIFY(hash.value("key2", result2));
        QVERIFY(hash.value("key3", result3));

        QCOMPARE(result1, data1);
        QCOMPARE(result2, data2);
        QCOMPARE(result3, data3);
    }

    void testUpdateExistingKey()
    {
        QString dbPath = m_tempDir->path() + "/update_key.db";
        PersistentHash<TestData> hash(dbPath);

        // Insert initial value
        TestData initialData{1, 1};
        QVERIFY(hash.insert("key", initialData));

        // Retrieve to confirm
        TestData result;
        QVERIFY(hash.value("key", result));
        QCOMPARE(result, initialData);

        // Update with new value
        TestData updatedData{2, 2};
        QVERIFY(hash.insert("key", updatedData));

        // Retrieve and verify update
        QVERIFY(hash.value("key", result));
        QCOMPARE(result, updatedData);
    }

    void testRetrieveMissingKey()
    {
        QString dbPath = m_tempDir->path() + "/missing_key.db";
        PersistentHash<TestData> hash(dbPath);

        TestData result;
        bool success = hash.value("nonexistent", result);
        QVERIFY(!success);
        QVERIFY(hash.lastError().contains("Key not found") || !hash.lastError().isEmpty());
    }

    void testValueWithDefaultReturn()
    {
        QString dbPath = m_tempDir->path() + "/default_return.db";
        PersistentHash<TestData> hash(dbPath);

        TestData insertData{99, 88};
        QVERIFY(hash.insert("test", insertData));

        // Test the overloaded value() method that returns TestData directly
        TestData result = hash.value("test");
        QCOMPARE(result.row, 99);
        QCOMPARE(result.col, 88);

        // Test with missing key - should return default constructed TestData
        TestData missing = hash.value("missing");
        QCOMPARE(missing.row, 0);
        QCOMPARE(missing.col, 0);
    }

    void testCacheBehavior()
    {
        QString dbPath = m_tempDir->path() + "/cache_test.db";
        {
            PersistentHash<TestData> hash(dbPath);

            // Insert values (cached in memory)
            TestData data1{5, 15};
            TestData data2{6, 16};
            QVERIFY(hash.insert("key1", data1));
            QVERIFY(hash.insert("key2", data2));

            // Retrieve from cache
            TestData result;
            QVERIFY(hash.value("key1", result));
            QCOMPARE(result, data1);
        }
        // Destructor should persist data

        // Create new instance and verify data was persisted
        PersistentHash<TestData> hash2(dbPath);
        TestData result;
        QVERIFY(hash2.value("key1", result));
        QCOMPARE(result.row, 5);
        QCOMPARE(result.col, 15);
    }

    void testDatabasePersistence()
    {
        QString dbPath = m_tempDir->path() + "/persistence.db";

        // First instance: insert data
        {
            PersistentHash<TestData> hash(dbPath);
            TestData data{123, 456};
            QVERIFY(hash.insert("persistent_key", data));
        }

        // Second instance: retrieve persisted data
        {
            PersistentHash<TestData> hash(dbPath);
            TestData result;
            QVERIFY(hash.value("persistent_key", result));
            QCOMPARE(result.row, 123);
            QCOMPARE(result.col, 456);
        }
    }

    void testEmptyStringKey()
    {
        QString dbPath = m_tempDir->path() + "/empty_key.db";
        PersistentHash<TestData> hash(dbPath);

        // Insert with empty string key
        TestData data{77, 88};
        bool result = hash.insert("", data);
        QVERIFY(result);

        // Retrieve with empty string key
        TestData retrieved;
        QVERIFY(hash.value("", retrieved));
        QCOMPARE(retrieved, data);
    }

    void testSpecialCharactersInKey()
    {
        QString dbPath = m_tempDir->path() + "/special_chars.db";
        PersistentHash<TestData> hash(dbPath);

        // Test various special characters in keys
        TestData data{1, 2};
        QString specialKey = "key!@#$%^&*()_+-=[]{}|;:',.<>?/\\";

        QVERIFY(hash.insert(specialKey, data));

        TestData result;
        QVERIFY(hash.value(specialKey, result));
        QCOMPARE(result, data);
    }

    void testUnicodeKey()
    {
        QString dbPath = m_tempDir->path() + "/unicode_key.db";
        PersistentHash<TestData> hash(dbPath);

        // Test Unicode characters in key
        TestData data{10, 20};
        QString unicodeKey = "键值_clé_chiave_🔑";

        QVERIFY(hash.insert(unicodeKey, data));

        TestData result;
        QVERIFY(hash.value(unicodeKey, result));
        QCOMPARE(result, data);
    }

    void testLargeValues()
    {
        QString dbPath = m_tempDir->path() + "/large_values.db";

        {
            PersistentHash<TestData> hash(dbPath);
            // Create large range of values
            for (int i = 0; i < 1000; ++i) {
                TestData data{i, i * 2};
                QString key = QString("key_%1").arg(i);
                QVERIFY(hash.insert(key, data));
            }

            // Verify some values
            TestData result;
            QVERIFY(hash.value("key_500", result));
            QCOMPARE(result.row, 500);
            QCOMPARE(result.col, 1000);

            QVERIFY(hash.value("key_999", result));
            QCOMPARE(result.row, 999);
            QCOMPARE(result.col, 1998);
        }

        PersistentHash<TestData> hash(dbPath);
        TestData result;
        QVERIFY(hash.value("key_500", result));
        QCOMPARE(result.row, 500);
        QCOMPARE(result.col, 1000);

        QVERIFY(hash.value("key_999", result));
        QCOMPARE(result.row, 999);
        QCOMPARE(result.col, 1998);
    }

    void testZeroValues()
    {
        QString dbPath = m_tempDir->path() + "/zero_values.db";
        PersistentHash<TestData> hash(dbPath);

        // Insert and retrieve zero values
        TestData zeroData{0, 0};
        QVERIFY(hash.insert("zero_key", zeroData));

        TestData result;
        QVERIFY(hash.value("zero_key", result));
        QCOMPARE(result.row, 0);
        QCOMPARE(result.col, 0);
    }

    void testNegativeValues()
    {
        QString dbPath = m_tempDir->path() + "/negative_values.db";
        PersistentHash<TestData> hash(dbPath);

        // Insert and retrieve negative values
        TestData negativeData{-42, -100};
        QVERIFY(hash.insert("negative_key", negativeData));

        TestData result;
        QVERIFY(hash.value("negative_key", result));
        QCOMPARE(result.row, -42);
        QCOMPARE(result.col, -100);
    }

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
};

QTEST_MAIN(TestPersistentHash)
#include "test_persistenthash.moc"
