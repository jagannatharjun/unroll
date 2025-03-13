#ifndef PATHHISTORYDB_HPP
#define PATHHISTORYDB_HPP

#include <QObject>
#include <QHash>

#include "../core/persistenthash.hpp"

class QDataStream;

struct HistoryData
{
    std::optional<int> row, col;
    std::optional<int> sortcolumn, sortorder;

    // if in history, random sort was true
    std::optional<bool> randomsort;

    // maybe split
    std::optional<int> randomseed;
    std::optional<int> random_row, random_col;
    std::optional<int> random_sortcolumn, random_sortorder;
};

QDataStream &operator <<(QDataStream &s, const HistoryData &data);


QDataStream &operator>>(QDataStream &s, HistoryData &data);

class PathHistoryDB : public PersistentHash<HistoryData>
{
public:
    PathHistoryDB(const QString &dbPath) : PersistentHash (dbPath) {}
};

#endif // PATHHISTORYDB_HPP
