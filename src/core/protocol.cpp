#include "protocol.h"
#include <algorithm>

using namespace QSanProtocol;

unsigned int QSanProtocol::QSanGeneralPacket::_m_globalSerial = 0;
const QString QSanProtocol::Countdown::S_COUNTDOWN_MAGIC = "MG_COUNTDOWN";

bool QSanProtocol::Countdown::tryParse(QJsonValue valdata)
{
    QJsonArray val = valdata.toArray();
    if ((val.size() != 2 && val.size() != 3) || !val[0].isString() || val[0].toString() != S_COUNTDOWN_MAGIC)
		return false;
	if (val.size() == 3)
	{
		if (!Utils::isIntArray(val, 1, 2)) return false;
        m_current = (time_t)val[1].toDouble();
        m_max = (time_t)val[2].toDouble();
		m_type = S_COUNTDOWN_USE_SPECIFIED;
		return true;
	}
	else if (val.size() == 2)
	{
        CountdownType type = (CountdownType)val[1].toDouble();
		if (type != S_COUNTDOWN_NO_LIMIT && type != S_COUNTDOWN_USE_DEFAULT)
			return false;
		else m_type = type;
		return true;
	}
	else return false;            

}

bool QSanProtocol::Utils::isStringArray(const QJsonValue &jsonObjectData, unsigned int startIndex, unsigned int endIndex)
{
    QJsonArray jsonObject = jsonObjectData.toArray();
    if (jsonObject.size() <= endIndex)
	{
		return false;
	}
	for (unsigned int i = startIndex; i <= endIndex; i++)
	{
		if (!jsonObject[i].isString()){
			return false;
		}
	}
	return true;
}

bool QSanProtocol::Utils::isIntArray(const QJsonValue &jsonObjectData, unsigned int startIndex, unsigned int endIndex)
{
    QJsonArray jsonObject = jsonObjectData.toArray();
    if (jsonObject.size() <= endIndex)
	{
		return false;
	}
	for (unsigned int i = startIndex; i <= endIndex; i++)
	{
        if (!jsonObject[i].isDouble())
		{
			return false;
		}
	}
	return true;
}

bool QSanProtocol::QSanGeneralPacket::parse(const QByteArray &s)
{
    QJsonDocument doc = QJsonDocument::fromJson(s);
    if(doc.isNull() || !doc.isArray()){
        return false;
    }

    QJsonArray result = doc.array();
    if(!Utils::isIntArray(result, 0, 3) || result.size() > 5){
		return false;
    }

    m_globalSerial = (unsigned int)result[0].toDouble();
    m_localSerial = (unsigned int)result[1].toDouble();
    m_packetType = (PacketType)result[2].toDouble();
    m_command = (CommandType)result[3].toDouble();

	if (result.size() == 5)
		parseBody(result[4]);
	return true;
}

QString QSanProtocol::QSanGeneralPacket::toString() const
{
    QJsonArray result;
    result.append((double) m_globalSerial);
    result.append((double) m_localSerial);
    result.append(m_packetType);
    result.append(m_command);
	const QJsonValue &body = constructBody();
    if (!body.isNull())
        result.append(body);

    QJsonDocument doc(result);
    QString msg = doc.toJson(QJsonDocument::Compact);

	return msg;
}
