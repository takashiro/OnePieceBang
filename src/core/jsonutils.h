#ifndef _QSAN_JSON_UTILS_H
#define _QSAN_JSON_UTILS_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QJsonArray>
#include <QJsonValue>

namespace BP{

QJsonArray toJsonArray(const QString& s1, const QString& s2);
QJsonArray toJsonArray(const QString& s1, const QJsonValue& s2);
QJsonArray toJsonArray(const QString& s1, const QString& s2, const QString& s3);
QJsonArray toJsonArray(const QList<int>&);
QJsonArray toJsonArray(const QList<QString>&);
QJsonArray toJsonArray(const QStringList&);
bool tryParse(const QJsonValue&, QList<int> &);
bool tryParse(const QJsonValue&, QStringList &);

}

#endif
