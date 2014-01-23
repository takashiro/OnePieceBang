#include "protocol.h"
#include <algorithm>

unsigned int BangProtocol::Packet::global_serial_sequence = 0;
const QString BangProtocol::Countdown::CountdownMagic = "MG_COUNTDOWN";

bool BangProtocol::Countdown::tryParse(QJsonValue valdata){
	QJsonArray val = valdata.toArray();
	if((val.size() != 2 && val.size() != 3) || !val[0].isString() || val[0].toString() != CountdownMagic)
		return false;

	if(val.size() == 3){
		if(!isIntArray(val, 1, 2)){
			return false;
		}

		current = (time_t) val[1].toDouble();
		max = (time_t) val[2].toDouble();
		type = UseSpecified;

		return true;

	}else if(val.size() == 2){
		CountdownType type = (CountdownType) val[1].toDouble();

		if(type != Unlimited && type != UseDefault){
			return false;
		}else{
			this->type = type;
		}

		return true;
	}
	else return false;            

}

bool BangProtocol::isStringArray(const QJsonValue &data, unsigned int start_index, unsigned int end_index)
{
	QJsonArray json_object = data.toArray();
	if(json_object.size() <= end_index){
		return false;
	}

	for(unsigned int i = start_index; i <= end_index; i++){
		if(!json_object[i].isString()){
			return false;
		}
	}
	return true;
}

bool BangProtocol::isIntArray(const QJsonValue &data, unsigned int start_index, unsigned int end_index){
	QJsonArray json_object = data.toArray();
	if(json_object.size() <= end_index){
		return false;
	}

	for(unsigned int i = start_index; i <= end_index; i++){
		if(!json_object[i].isDouble()){
			return false;
		}
	}
	return true;
}

bool BangProtocol::Packet::parse(const QByteArray &s){
	QJsonDocument doc = QJsonDocument::fromJson(s);
	if(doc.isNull() || !doc.isArray()){
		return false;
	}

	QJsonArray result = doc.array();
	if(!isIntArray(result, 0, 3) || result.size() > 5){
		return false;
	}

	global_serial = (unsigned int)result[0].toDouble();
	local_serial = (unsigned int)result[1].toDouble();
	packet_type = (PacketType)result[2].toDouble();
	command = (CommandType)result[3].toDouble();

	if (result.size() == 5)
		parseBody(result[4]);
	return true;
}

QString BangProtocol::Packet::toString() const{
	QJsonArray result;
	result.append((double) global_serial);
	result.append((double) local_serial);
	result.append(packet_type);
	result.append(command);

	const QJsonValue &body = constructBody();
	if (!body.isNull())
		result.append(body);

	QJsonDocument doc(result);
	QString msg = doc.toJson(QJsonDocument::Compact);

	return msg;
}
