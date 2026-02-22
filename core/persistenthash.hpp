#ifndef PERSISTENTHASH_H
#define PERSISTENTHASH_H

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <QString>
#include <QStringList>
#include <QVariant>

// Forward declarations
class QSqlError;
class QSqlDatabase;

// Non-template base class that can be implemented in a .cpp file
class PersistentHashBase
{
public:
    PersistentHashBase(const QString &dbName = "persistenthash.db");
    virtual ~PersistentHashBase();

    QString lastError() const;

protected:
    // Protected methods for derived template class
    bool storeData(const QList<QString> &key, const QList<QByteArray> &value);
    bool retrieveData(const QString &key, QByteArray &value) const;
    bool isOpen() const;

private:
    std::unique_ptr<QSqlDatabase> m_db;
    QString m_lastError;
    QString m_connectionName;

    // Helper to set the last error
    void setLastError(const QString &error);
    void setLastError(const QSqlError &sqlError);
    bool createTable();
};

// Template class that inherits from the base class
template<typename T>
class PersistentHash : public PersistentHashBase
{
public:
    // Constructor - uses the base class constructor
    PersistentHash(const QString &dbName = "persistenthash.db")
        : PersistentHashBase(dbName)
    {}

    ~PersistentHash()
    {
        storeData(m_cache.keys(), m_cache.values());
    }

    // Insert or update a key-value pair
    bool insert(const QString &key, const T &value)
    {
        if (!isOpen()) {
            return false;
        }

        // Serialize the value to a QByteArray
        QByteArray byteArray;
        {
            QDataStream stream(&byteArray, QIODevice::WriteOnly);
            stream << value;
        }

        qDebug() << "insert" << key << value.row << value.col;
        m_cache[key] = byteArray;
        return true;
    }

    // Retrieve a value by key
    bool value(const QString &key, T &result) const
    {
        if (!isOpen()) {
            return false;
        }

        QByteArray byteArray;
        if (m_cache.contains(key)) {
            byteArray = m_cache[key];
        } else if (!retrieveData(key, byteArray)) {
            return false;
        }

        // Deserialize the value from QByteArray
        QDataStream stream(&byteArray, QIODevice::ReadOnly);
        stream >> result;

        qDebug() << "value" << key << result.row << result.col;
        return stream.status() == QDataStream::Ok;
    }

    T value(const QString &key) const
    {
        T result;
        value(key, result);
        return result;
    }

public:
    mutable QHash<QString, QByteArray> m_cache;
};

#endif // PERSISTENTHASH_H
