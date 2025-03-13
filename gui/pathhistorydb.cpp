#include "pathhistorydb.hpp"

#include "../core/dbutil.hpp"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <qsqlerror.h>

#include <QDataStream>


namespace
{
template<typename T>
QDataStream &operator <<(QDataStream &s, const std::optional<T> &data)
{
    if (!data.has_value())
        return s << QVariant{};
    else
        return s << QVariant::fromValue(data.value());
}

template<typename T>
QDataStream &operator >>(QDataStream &s, std::optional<T> &data)
{
    QVariant v;
    s >> v;
    if (v.isValid() && v.canConvert<T>())
        data = v.value<T>();
    else
        data = std::nullopt;
    return s;
}
}

QDataStream &operator <<(QDataStream &s, const HistoryData &data)
{
    const int version = 0x1;

    s << version;
    s << data.row;
    s << data.col;
    s << data.sortcolumn;
    s << data.sortorder;
    s << data.randomsort;
    s << data.randomseed;
    s << data.random_row;
    s << data.random_col;
    s << data.random_sortcolumn;
    s << data.random_sortorder;

    return s;
}

QDataStream &operator>>(QDataStream &s, HistoryData &data)
{
    int version;

    s >> version;
    s >> data.row;
    s >> data.col;
    s >> data.sortcolumn;
    s >> data.sortorder;
    s >> data.randomsort;
    s >> data.randomseed;
    s >> data.random_row;
    s >> data.random_col;
    s >> data.random_sortcolumn;
    s >> data.random_sortorder;

    return s;
}
