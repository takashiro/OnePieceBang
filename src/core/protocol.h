#ifndef _QSAN_PROTOCOL_H
#define _QSAN_PROTOCOL_H

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>

namespace BP{
	bool isStringArray(const QJsonValue &data, unsigned int start_index, unsigned int end_index);
	bool isIntArray(const QJsonValue &data, unsigned int start_index, unsigned int end_index);

	enum PacketType{
		UnknownPacket,
		ServerRequest,
		ServerReply,
		ServerNotification,
		ClientRequest,
		ClientReply,
		ClientNotification
	};

	enum ProcessInstanceType{
		ServerInstance,
		ClientInstance
	};

	enum CheatCode{
		GetOneCard,
		ChangeGeneral,
		KillPlayer,
		RevivePlayer,
		MakeDamage,
		RunScript
	};

	enum CheatCategory{
		FireDamage,
		ThunderDamage,
		NormalDamage,
		HpRecover,
		HpLose
	};

	enum CommandType{
		UnknownCommand,
		AskForCardChosen,
		Activate,
		AskForUseCard,
		AskForCard,
		AskForCardShow,
		ShowAllCards,
		AskForExchange,
		AskForDiscard,
		AskForSkillInvoke,
		MoveFocus,
		AskForGeneral,
		AskForKingdom,
		AskForSuit,
		AskForAssign,
		AskForRole3v3,
		AskForDirection,
		AskForPlayerChosen,
		AskForOrder,
		AskForSingleWine,
		ClaimGeneral,
		ClaimAddedSkill,
		ClaimRemovedSkill,
		AskForNullification,
		AskForChoice,
		AskForPindian,
		AskForAG,
		AskForYiji,
		AskForGuanxing,
		AskForGongxin,
		SetProperty,
		SetHp,
		SetMaxHp,
		Cheat,
		AskForSurrender,
		GameOver,
		MoveCard,
		GetCard,
		LoseCard,
		CheckVersion,
		Setup,
		Speak,
		AddPlayer,
		SetScreenName,
		SignUpR,
		SignUp,
		NetworkDelayTest
	};

	enum Game3v3ChooseOrderCommand
	{
		ChooseOrderTurn,
		ChooseOrderSelect
	};

	enum Game3v3Camp
	{
		WarmCamp = 0,
		CoolCamp = 1
	};

	class Countdown{
	public:
		enum CountdownType{            
			Unlimited,
			UseSpecified,
			UseDefault
		};
		CountdownType type;

		static const QString CountdownMagic;
		time_t current;
		time_t max;
		inline Countdown(CountdownType type = Unlimited, time_t current = 0, time_t max = 0)
			:type(type), current(current), max(max){

		}

		bool tryParse(QJsonValue val);

		inline QJsonValue toJsonValue(){
			if(type == Unlimited || type == UseDefault){
				QJsonArray val;
				val.append(CountdownMagic);
				val.append((int) type);
				return val;
			}else{
				QJsonArray val;
				val.append(CountdownMagic);
				val.append((int) current);
				val.append((int) max);
				return val;
			}
		}

		inline bool hasTimedOut(){
			if (type == Unlimited){
				return false;
			}else{
				return current >= max;
			}
		}
	};

	class AbstractPacket{
	public:
		virtual bool parse(const QByteArray &) = 0;
		virtual QByteArray toUtf8() const = 0;
		virtual PacketType getPacketType() const = 0;
		virtual CommandType getCommandType() const = 0;        
	};
		
	class Packet: public AbstractPacket{
	public:
		//format: [global_serial,local_serial,packet_type,command_name,command_body]
		unsigned int global_serial;
		unsigned int local_serial;

		inline Packet(PacketType packet_type = UnknownPacket, CommandType command = UnknownCommand)
			:local_serial(0), packet_type(packet_type), command(command){
			global_serial_sequence++;
			global_serial = global_serial_sequence;
		}

		inline void setMessageBody(const QJsonValue &value){
			message_body = value;
		}

		inline QJsonValue &getMessageBody(){
			return message_body;
		}

		inline const QJsonValue &getMessageBody() const{
			return message_body;
		}

		virtual bool parse(const QByteArray &);
		virtual QByteArray toUtf8() const;

		inline virtual PacketType getPacketType() const {
			return packet_type;
		}

		inline virtual CommandType getCommandType() const {
			return command;
		}

	protected:
		static unsigned int global_serial_sequence;
		CommandType command;
		PacketType packet_type;
		QJsonValue message_body;

		inline virtual bool parseBody(const QJsonValue &value){
			message_body = value;
			return true;
		}

		virtual const QJsonValue& constructBody() const{
			return message_body;
		}
	};
}

#endif
