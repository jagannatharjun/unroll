#ifndef PERSISTENTHASH_H
#define PERSISTENTHASH_H

#include <QString>
#include <QVariant>
#include <QStringList>
#include <QSqlDatabase>
#include <QDataStream>
#include <QByteArray>
#include <QIODevice>

// Forward declarations
class QSqlError;

// Non-template base class that can be implemented in a .cpp file
class PersistentHashBase
{
public:
    PersistentHashBase(const QString &dbName = "persistenthash.db");
    virtual ~PersistentHashBase();

    // Basic operations that don't depend on the template type
    bool contains(const QString &key) const;
    bool remove(const QString &key);
    bool clear();
    QStringList keys() const;
    int size() const;
    QString lastError() const;

protected:
    // Protected methods for derived template class
    bool storeData(const QString &key, const QByteArray &value);
    bool retrieveData(const QString &key, QByteArray &value) const;
    bool isOpen() const;

private:
    QSqlDatabase m_db;
    QString m_lastError;
    QString m_connectionName;

    // Helper to set the last error
    void setLastError(const QString &error);
    void setLastError(const QSqlError &sqlError);
    bool createTable();
};

// Template class that inherits from the base class
template <typename T>
class PersistentHash : public PersistentHashBase
{
public:
    // Constructor - uses the base class constructor
    PersistentHash(const QString &dbName = "persistenthash.db")
        : PersistentHashBase(dbName)
    {
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
        // Use the base class method to store the data
        return storeData(key, byteArray);
    }

    // Retrieve a value by key
    bool value(const QString &key, T &result) const
    {
        if (!isOpen()) {
            return false;
        }

        QByteArray byteArray;
        if (!retrieveData(key, byteArray)) {
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
};

#endif // PERSISTENTHASH_H
