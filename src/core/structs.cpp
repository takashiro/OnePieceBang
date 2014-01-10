#include "structs.h"
#include "jsonutils.h"
#include "protocol.h"

using namespace QSanProtocol::Utils;

bool CardMoveStruct::tryParse(const QJsonValue &data){
	if(!data.isArray()){
		return false;
	}

	QJsonArray arg = data.toArray();
	if(arg.size() != 7){
		return false;
	}

	if (!arg[0].isDouble() || !isIntArray(arg, 1, 2) || !isStringArray(arg, 3, 6)) return false;
	card_id = arg[0].toDouble();
	from_place = (Player::Place)arg[1].toDouble();
	to_place = (Player::Place)arg[2].toDouble();
	from_player_name = arg[3].toString();
	to_player_name = arg[4].toString();
	from_pile_name = arg[5].toString();
	to_pile_name = arg[6].toString();
	return true;
}

QJsonValue CardMoveStruct::toJsonValue() const
{
	QJsonArray arg;
	if (open) arg.append(card_id);
	else arg.append(Card::S_UNKNOWN_CARD_ID);
	arg.append((int)from_place);
	arg.append((int)to_place);
	arg.append(from_player_name);
	arg.append(to_player_name);
	arg.append(from_pile_name);
	arg.append(to_pile_name);
	return arg;
}

bool CardsMoveStruct::tryParse(const QJsonValue &data){
	if(!data.isArray()){
		return false;
	}

	QJsonArray arg = data.toArray();
	if (arg.size() != 7) return false;
	if ((!arg[0].isDouble() && !arg[0].isArray()) ||
		!isIntArray(arg, 1, 2) || !isStringArray(arg, 3, 6)) return false;
	if (arg[0].isDouble())
	{
		int size = arg[0].toDouble();
		for (int i = 0; i < size; i++)        
			card_ids.append(Card::S_UNKNOWN_CARD_ID);        
	}
	else if (!QSanProtocol::Utils::tryParse(arg[0], card_ids))
		return false;
	from_place = (Player::Place)arg[1].toDouble();
	to_place = (Player::Place)arg[2].toDouble();
	from_player_name = arg[3].toString();
	to_player_name = arg[4].toString();
	from_pile_name = arg[5].toString();
	to_pile_name = arg[6].toString();
	return true;
}

QJsonValue CardsMoveStruct::toJsonValue() const{
	QJsonArray arg;
	if (open) arg.append(toJsonArray(card_ids));
	else arg.append(card_ids.size());
	arg.append((int)from_place);
	arg.append((int)to_place);
	arg.append(from_player_name);
	arg.append(to_player_name);
	arg.append(from_pile_name);
	arg.append(to_pile_name);
	return arg;
}

QList<CardMoveStruct> CardsMoveStruct::flatten() const{
	QList<CardMoveStruct> result;
	foreach (int card_id, card_ids)
	{
		CardMoveStruct move;
		move.from = from;
		move.to = to;
		move.from_pile_name = from_pile_name;
		move.to_pile_name = to_pile_name;
		move.from_place = from_place;
		move.to_place = to_place;
		move.card_id = card_id;
		move.from_player_name = from_player_name;
		move.to_player_name = to_player_name;
		move.open = open;
		result.append(move);
	}
	return result;
}
