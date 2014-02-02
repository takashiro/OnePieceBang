#ifndef SERVERPLAYER_H
#define SERVERPLAYER_H

class Room;
struct CardMoveStruct;
class AI;
class Recorder;

#include "player.h"
#include "socket.h"
#include "protocol.h"

#include <QSemaphore>
#include <QDateTime>

class ServerPlayer : public Player
{
	Q_OBJECT

	Q_PROPERTY(QString ip READ getIp)

public:
	explicit ServerPlayer(Room *room);

	void setSocket(ClientSocket *socket);
	void notify(BP::CommandType command, const QJsonValue &arg = QJsonValue()) const;
	QString reportHeader() const;
	void sendProperty(const char *property_name, const Player *player = NULL) const;
	void unicast(const QString &message) const;
	inline void invoke(const BP::AbstractPacket &packet) const{unicast(packet.toUtf8());}
	void drawCard(const Card *card);
	Room *getRoom() const;
	void playCardEffect(const Card *card) const;
	void playCardEffect(const QString &card_name) const;
	int getRandomHandCardId() const;
	const Card *getRandomHandCard() const;
	void obtainCard(const Card *card, bool unhide = true);
	void throwAllEquips();
	void throwAllHandCards();
	void throwAllCards();
	void bury();
	void throwAllMarks();
	void clearPrivatePiles();
	void drawCards(int n, bool set_emotion = true, const QString &reason = QString());
	bool askForSkillInvoke(const QString &skill_name, const QVariant &data = QVariant());
	QList<int> forceToDiscard(int discard_num, bool include_equip);
	QList<int> handCards() const;
	QList<const Card *> getHandcards() const;
	QList<const Card *> getCards(const QString &flags) const;
	DummyCard *wholeHandCards() const;
	bool hasNullification() const;
	void kick();
	bool pindian(ServerPlayer *target, const QString &reason, const Card *card1 = NULL);
	void turnOver();
	void play(QList<Player::Phase> set_phases = QList<Player::Phase>());

	QList<Player::Phase> &getPhases();
	void skip(Player::Phase phase);
	void skip();
	
	void gainMark(const QString &mark, int n = 1);
	void loseMark(const QString &mark, int n = 1);
	void loseAllMarks(const QString &mark_name);

	void setAI(AI *ai);
	AI *getAI() const;
	AI *getSmartAI() const;

	bool isOnline() const; // @todo: better rename this to be re
	inline bool isOffline() const { return getState() == "robot" || getState() == "offline"; }

	virtual int aliveCount() const;
	virtual int getHandcardNum() const;
	virtual void removeCard(const Card *card, Place place);
	virtual void addCard(const Card *card, Place place);
	virtual bool isLastHandCard(const Card *card) const;

	void addVictim(ServerPlayer *victim);
	QList<ServerPlayer *> getVictims() const;

	void startRecord();
	void saveRecord(const QString &filename);

	void setNext(ServerPlayer *next);
	ServerPlayer *getNext() const;
	ServerPlayer *getNextAlive() const;

	// 3v3 methods
	void addToSelected(const QString &general);
	QStringList getSelected() const;
	QString findReasonable(const QStringList &generals, bool no_unreasonable = false);
	void clearSelected();

	int getGeneralMaxHp() const;
	virtual QString getGameMode() const;

	QString getIp() const;
	void introduceTo(ServerPlayer *player);
	void marshal(ServerPlayer *player) const;

	void addToPile(const QString &pile_name, const Card *card, bool open = true);
	void addToPile(const QString &pile_name, int card_id, bool open = true);
	void addToPile(const QString &pile_name, QList<int> card_ids, bool open = true);
	void gainAnExtraTurn(ServerPlayer *clearflag = NULL);

	void copyFrom(ServerPlayer* sp);

	void startNetworkDelayTest();
	qint64 endNetworkDelayTest();

	//Synchronization helpers
	enum SemaphoreType {
		SEMA_MUTEX, // used to protect mutex access to member variables        
		SEMA_COMMAND_INTERACTIVE // used to wait for response from client        
	};
	inline QSemaphore* getSemaphore(SemaphoreType type){ return semas[type]; }
	inline void acquireLock(SemaphoreType type){ semas[type]->acquire(); }
	inline bool tryAcquireLock(SemaphoreType type, int timeout = 0){
		return semas[type]->tryAcquire(1, timeout); 
	}
	inline void releaseLock(SemaphoreType type){ semas[type]->release(); }
	inline void drainLock(SemaphoreType type){ while ((semas[type]->tryAcquire())) ; }
	inline void drainAllLocks(){
		for(int i=0; i< S_NUM_SEMAPHORES; i++){
			drainLock((SemaphoreType)i);
		}
	}
	inline QJsonValue getClientReply(){return client_response_data;}
	inline void setClientReply(const QJsonValue &val){client_response_data = val;}
	unsigned int expected_reply_serial; // Suggest the acceptable serial number of an expected response.
	bool is_client_response_ready; //Suggest whether a valid player's reponse has been received.
	bool is_waiting_for_reply; // Suggest if the server player is waiting for client's response.
	QJsonValue cheat_arguments; // Store the cheat code received from client.
	BP::CommandType expected_reply_command; // Store the command to be sent to the client.
	QJsonValue command_arguments; // Store the command args to be sent to the client.


protected:    
	//Synchronization helpers
	QSemaphore **semas;
	static const int S_NUM_SEMAPHORES;    

private:
	ClientSocket *socket;
	QList<const Card *> handcards;
	Room *room;
	AI *ai;
	AI *trust_ai;
	QList<ServerPlayer *> victims;
	Recorder *recorder;
	QList<Phase> phases;
	ServerPlayer *next;
	QStringList selected; // 3v3 mode use only
	QDateTime test_time;
	QJsonValue client_response_data;

private slots:
	void getMessage(QByteArray message);

signals:
	void disconnected();
	void request_got(const QByteArray &request);
	void packet_cast(const BP::Packet &packet) const;
	void message_cast(const QByteArray &raw_message) const;
};

#endif // SERVERPLAYER_H
