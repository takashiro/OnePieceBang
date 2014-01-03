#include "jsonutils.h"

QJsonArray QSanProtocol::Utils::toJsonArray(const QString& s1, const QString& s2)
{
    QJsonArray val;
    val.append(s1);
    val.append(s2);
	return val;
}

QJsonArray QSanProtocol::Utils::toJsonArray(const QString& s1, const QString& s2, const QString& s3)
{
    QJsonArray val;
    val.append(s1);
    val.append(s2);
    val.append(s3);
	return val;
}

QJsonArray QSanProtocol::Utils::toJsonArray(const QString& s1, const QJsonValue& s2)
{
    QJsonArray val;
    val.append(s1);
    val.append(s2);
	return val;
}

QJsonArray QSanProtocol::Utils::toJsonArray(const QList<int>& arg)
{
    QJsonArray val;
	foreach(int i, arg)
		val.append(i);
	return val;
}

bool QSanProtocol::Utils::tryParse(const QJsonValue& argdata, QList<int>& result)
{
    if (!argdata.isArray()) return false;
    QJsonArray arg = argdata.toArray();
	for (unsigned int i = 0; i< arg.size(); i++)
	{
        if (!arg[i].isDouble()) return false;
	}
	for (unsigned int i = 0; i< arg.size(); i++)
	{
        result.append(arg[i].toDouble());
	}    
	return true;
}

QJsonArray QSanProtocol::Utils:: toJsonArray(const QList<QString>& arg)
{
    QJsonArray val;
	foreach(QString s, arg)
        val.append(s);
	return val;
}

QJsonArray QSanProtocol::Utils::toJsonArray(const QStringList& arg)
{
    QJsonArray val;
	foreach(QString s, arg)
        val.append(s);
	return val;
}

bool QSanProtocol::Utils::tryParse(const QJsonValue& argdata, QStringList& result)
{
    if (!argdata.isArray()) return false;
    QJsonArray arg = argdata.toArray();
	for (unsigned int i = 0; i< arg.size(); i++)
	{
		if (!arg[i].isString()) return false;        
	}
	for (unsigned int i = 0; i< arg.size(); i++)
	{
        result.append(arg[i].toString());
	}    
	return true;
}
