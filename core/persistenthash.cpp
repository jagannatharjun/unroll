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

bool PersistentHashBase::storeData(const QList<QString> &key, const QList<QByteArray> &value)
{
    QSqlQuery query(*m_db);
    query.prepare("INSERT OR REPLACE INTO hash_data (key, value) VALUES (?, ?)");
    query.addBindValue(key);
    query.addBindValue(QVariant::fromValue(value));

    if (!query.execBatch()) {
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
