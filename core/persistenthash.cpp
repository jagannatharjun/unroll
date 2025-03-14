#include "persistenthash.hpp"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDir>

// Static counter for unique connection names
static int connectionCounter = 0;

PersistentHashBase::PersistentHashBase(const QString &dbName)
{
    // Create a unique connection name for this instance
    m_connectionName = QString("PersistentHashConnection%1").arg(connectionCounter++);

    m_db = std::make_unique<QSqlDatabase>(QSqlDatabase::addDatabase("QSQLITE", m_connectionName));
    m_db->setDatabaseName(dbName);

    if (!m_db->open()) {
        setLastError(m_db->lastError());
        qCritical() << "Failed to open database:" << m_lastError;
        return;
    }

    // Create table if it doesn't exist
    if (!createTable()) {
        qCritical() << "Failed to create table:" << m_lastError;
    }
}

PersistentHashBase::~PersistentHashBase()
{
    if (m_db->isOpen()) {
        m_db->close();
    }

    m_db.reset();

    // Remove the database connection
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool PersistentHashBase::createTable()
{
    QSqlQuery query(*m_db);
    if (!query.exec("CREATE TABLE IF NOT EXISTS hash_data "
                    "(key TEXT PRIMARY KEY, value BLOB)")) {
        setLastError(query.lastError());
        return false;
    }
    return true;
}

bool PersistentHashBase::isOpen() const
{
    return m_db->isOpen();
}

bool PersistentHashBase::storeData(const QString &key, const QByteArray &value)
{
    QSqlQuery query(*m_db);
    query.prepare("INSERT OR REPLACE INTO hash_data (key, value) VALUES (?, ?)");
    query.addBindValue(key);
    query.addBindValue(value);

    if (!query.exec()) {
        setLastError(query.lastError());
        qCritical() << "Failed to insert data:" << m_lastError;
        return false;
    }

    return true;
}

bool PersistentHashBase::retrieveData(const QString &key, QByteArray &value) const
{
    QSqlQuery query(*m_db);
    query.prepare("SELECT value FROM hash_data WHERE key = ?");
    query.addBindValue(key);

    if (!query.exec()) {
        const_cast<PersistentHashBase*>(this)->setLastError(query.lastError());
        return false;
    }

    if (!query.next()) {
        const_cast<PersistentHashBase*>(this)->setLastError("Key not found");
        return false;
    }

    value = query.value(0).toByteArray();
    return true;
}

bool PersistentHashBase::contains(const QString &key) const
{
    if (!m_db->isOpen()) {
        return false;
    }

    QSqlQuery query(*m_db);
    query.prepare("SELECT 1 FROM hash_data WHERE key = ?");
    query.addBindValue(key);

    if (!query.exec()) {
        const_cast<PersistentHashBase*>(this)->setLastError(query.lastError());
        return false;
    }

    return query.next();
}

bool PersistentHashBase::remove(const QString &key)
{
    if (!m_db->isOpen()) {
        return false;
    }

    QSqlQuery query(*m_db);
    query.prepare("DELETE FROM hash_data WHERE key = ?");
    query.addBindValue(key);

    if (!query.exec()) {
        setLastError(query.lastError());
        qCritical() << "Failed to remove data:" << m_lastError;
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool PersistentHashBase::clear()
{
    if (!m_db->isOpen()) {
        return false;
    }

    QSqlQuery query(*m_db);
    if (!query.exec("DELETE FROM hash_data")) {
        setLastError(query.lastError());
        qCritical() << "Failed to clear data:" << m_lastError;
        return false;
    }

    return true;
}

QStringList PersistentHashBase::keys() const
{
    QStringList keyList;

    if (!m_db->isOpen()) {
        return keyList;
    }

    QSqlQuery query(*m_db);
    if (query.exec("SELECT key FROM hash_data")) {
        while (query.next()) {
            keyList << query.value(0).toString();
        }
    } else {
        const_cast<PersistentHashBase*>(this)->setLastError(query.lastError());
    }

    return keyList;
}

int PersistentHashBase::size() const
{
    if (!m_db->isOpen()) {
        return 0;
    }

    QSqlQuery query(*m_db);
    if (query.exec("SELECT COUNT(*) FROM hash_data") && query.next()) {
        return query.value(0).toInt();
    } else {
        const_cast<PersistentHashBase*>(this)->setLastError(query.lastError());
    }

    return 0;
}

QString PersistentHashBase::lastError() const
{
    return m_lastError;
}

void PersistentHashBase::setLastError(const QString &error)
{
    m_lastError = error;
}

void PersistentHashBase::setLastError(const QSqlError &sqlError)
{
    m_lastError = sqlError.text();
}
