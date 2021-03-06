#ifndef ROOM_H
#define ROOM_H

class TriggerSkill;
class Scenario;
class RoomDriver3v3;
class RoomDriver1v1;
class TrickCard;

struct lua_State;
struct LogMessage;

#include "serverplayer.h"
#include "roomdriver.h"
#include "protocol.h"
#include <qmutex.h>

class Server;
class RoomController;

class Room : public QObject{
	Q_OBJECT

public:
	friend class RoomDriver;
	friend class RoomDriver3v3;
	friend class RoomDriver1v1;
	friend class RoomController;

	typedef void (Room::*Callback)(ServerPlayer *, const QString &);
	typedef void (Room::*CallBack)(ServerPlayer *, const QJsonValue &);
	typedef bool (Room::*ResponseVerifyFunction)(ServerPlayer*, const QJsonValue&, void*);

	explicit Room(Server *server, const QString &mode);
	~Room();
	ServerPlayer *addSocket(ClientSocket *socket);
	bool isFull() const;
	bool isFinished() const;
	int getLack() const;
	QString getMode() const;
	const Scenario *getScenario() const;
	Server *getServer() const;
	RoomDriver *getDriver() const;
	void playSkillEffect(const QString &skill_name, int index = -1) const;
	void broadcastSkillInvoked(ServerPlayer *player, const QString &skill_name) const;
	ServerPlayer *getCurrent() const;
	void setCurrent(ServerPlayer *current);
	int alivePlayerCount() const;
	QList<ServerPlayer *> getOtherPlayers(ServerPlayer *except) const;
	QList<ServerPlayer *> getPlayers() const;
	QList<ServerPlayer *> getAllPlayers() const;
	QList<ServerPlayer *> getAlivePlayers() const;
	void output(const QString &message);
	void outputEventStack();
	void enterDying(ServerPlayer *player, DamageStruct *reason);
	void killPlayer(ServerPlayer *victim, DamageStruct *reason = NULL);
	void revivePlayer(ServerPlayer *player);
	QStringList aliveRoles(ServerPlayer *except = NULL) const;
	void gameOver(const QString &winner);
	void slashEffect(const SlashEffectStruct &effect);
	void slashResult(const SlashEffectStruct &effect, const Card *jink);
	void attachSkillToPlayer(ServerPlayer *player, const QString &skill_name);
	void detachSkillFromPlayer(ServerPlayer *player, const QString &skill_name);
	bool obtainable(const Card *card, ServerPlayer *player);
	void setPlayerFlag(ServerPlayer *player, const QString &flag);
	void setPlayerProperty(ServerPlayer *player, const char *property_name, const QVariant &value);
	void setPlayerMark(ServerPlayer *player, const QString &mark, int value);
	void setPlayerCardLock(ServerPlayer *player, const QString &name);
	void clearPlayerCardLock(ServerPlayer *player);
	void setPlayerStatistics(ServerPlayer *player, const QString &property_name, const QVariant &value);
	void setCardFlag(const Card *card, const QString &flag, ServerPlayer *who = NULL);
	void setCardFlag(int card_id, const QString &flag, ServerPlayer *who = NULL);
	void clearCardFlag(const Card *card, ServerPlayer *who = NULL);
	void clearCardFlag(int card_id, ServerPlayer *who = NULL);
	void useCard(const CardUseStruct &card_use, bool add_history = true);
	void damage(DamageStruct data);
	void sendDamageLog(const DamageStruct &data);
	void loseHp(ServerPlayer *victim, int lose = 1);
	void loseMaxHp(ServerPlayer *victim, int lose = 1);
	void applyDamage(ServerPlayer *victim, const DamageStruct &damage);
	void recover(const RecoverStruct &recover, bool set_emotion = false);
	bool cardEffect(const Card *card, ServerPlayer *from, ServerPlayer *to);
	bool cardEffect(const CardEffectStruct &effect);
	void judge(JudgeStruct &judge_struct);
	void sendJudgeResult(const JudgeStar judge);
	QList<int> getNCards(int n, bool update_pile_number = true);
	ServerPlayer *getLord() const;
	void askForGuanxing(ServerPlayer *zhuge, const QList<int> &cards, bool up_only);
	void doGongxin(ServerPlayer *shenlvmeng, ServerPlayer *target);
	int drawCard();
	const Card *peek();
	void fillAG(const QList<int> &card_ids, ServerPlayer *who = NULL);
	void takeAG(ServerPlayer *player, int card_id);
	inline void clearAG(){broadcastNotification(BP::ClearAG);}
	void provide(const Card *card);
	QList<ServerPlayer *> getLieges(const QString &kingdom, ServerPlayer *lord) const;
	void showCard(ServerPlayer *player, int card_id, ServerPlayer *only_viewer = NULL);
	void showAllCards(ServerPlayer *player, ServerPlayer *to = NULL);

	void sendLog(const LogMessage &log);
	void sendLog(const QString type, ServerPlayer *from = NULL, const QString &arg = QString());
   
	// Ask a player to send a server request and returns the client response. Call is blocking until client 
	// replies or server times out, whichever is earlier.
	// @param player
	//        The server player to carry out the command.
	// @param command
	//        Command to be executed on client side.
	// @param arg
	//        Command args.
	// @param timeOut
	//        Maximum milliseconds that server should wait for client response before returning.        
	// @param wait
	//        If true, return immediately after sending the request without waiting for client reply.
	// @return True if the a valid response is returned from client.  
	// Usage note: when you need a round trip request-response vector with a SINGLE client, use this command
	// with wait = true and read the reponse from player->getClientReply(). If you want to initiate a poll 
	// where more than one clients can respond simultaneously, you have to do it in two steps:
	// 1. Use this command with wait = false once for each client involved in the poll (or you can call this
	//    command only once in all with broadcast = true if the poll is to everypody).
	// 2. Call getResult(player, timeout) on each player to retrieve the result. Read manual for getResults
	//    before you use.
	bool requestPlayer(ServerPlayer* player, BP::CommandType command, const QJsonValue &arg, time_t timeOut, bool wait);
	bool requestPlayer(ServerPlayer* player, BP::CommandType command, const QJsonValue &arg, bool wait);

	// Broadcast a request to a list of players and get the client responses. Call is blocking until all client
	// replies or server times out, whichever is earlier. Check each player's m_isClientResponseReady to see if a valid
	// result has been received. The client response can be accessed by calling each player's getClientReply() function. 
	// @param players
	//        The list of server players to carry out the command.
	// @param command
	//        Command to be executed on client side. Command arguments should be stored in players->m_commandArgs.
	// @param timeOut
	//        Maximum total milliseconds that server will wait for all clients to respond before returning. Any client 
	//        response after the timeOut will be rejected.
	// @return True if the a valid response is returned from client.  
	bool broadcastRequest(QList<ServerPlayer*> &players, BP::CommandType command, time_t timeOut);
	bool broadcastRequest(QList<ServerPlayer*> &players, BP::CommandType command);

	// Broadcast a request to a list of players and get the first valid client response. Call is blocking until the first
	// client response is received or server times out, whichever is earlier. Any client response is verified by the validation
	// function and argument passed in. When a response is verified to be invalid, the function will continue to wait for
	// the next client response.
	// @param validateFunc
	//        Validation function that verifies whether the reply is a valid one. The first parameter passed to the function
	//        is the response sender, the second parameter is the response content, the third parameter is funcArg passed in.
	// @return The player that first send a legal request to the server. NULL if no such request is received.
	ServerPlayer* broadcastRaceRequest(QList<ServerPlayer*> &players, BP::CommandType command,
		   time_t timeOut, ResponseVerifyFunction validateFunc = NULL, void* funcArg = NULL);
	
	// Notify a player of a event by sending S_SERVER_NOTIFICATION packets. No reply should be expected from
	// the client for S_SERVER_NOTIFICATION as it's a one way notice. Any message from the client in reply to this call
	// will be rejected.
	bool notifyPlayer(ServerPlayer *player, BP::CommandType command, const QJsonValue &arg) const;

	// Broadcast a event to a list of players by sending S_SERVER_NOTIFICATION packets. No replies should be expected from
	// the clients for S_SERVER_NOTIFICATION as it's a one way notice. Any message from the client in reply to this call
	// will be rejected.    
	bool broadcastNotification(BP::CommandType command, const QJsonValue &arg = QJsonValue(), ServerPlayer *except = NULL) const;

	// Ask a server player to wait for the client response. Call is blocking until client replies or server times out, 
	// whichever is earlier.
	// @param player
	//        The server player to retrieve the client response.
	// @param timeOut
	//        Maximum milliseconds that server should wait for client response before returning.
	// @return True if the a valid response is returned from client.
	
	// Usage note: this function is only supposed to be either internally used by doRequest (wait = true) or externally
	// used in pair with doRequest (wait = false). Any other usage could result in unexpected synchronization errors. 
	// When getResult returns true, it's guaranteed that the expected client response has been stored and can be accessed by
	// calling player->getClientReply(). If getResult returns false, the value stored in player->getClientReply() could be
	// corrupted or in response to an unrelevant server request. Therefore, if the return value is false, do not poke into
	// player->getClientReply(), use the default value directly. If the return value is true, the reply value should still be
	// examined as a malicious client can have tampered with the content of the package for cheating purposes.
	bool getResult(ServerPlayer* player, time_t timeOut);
	ServerPlayer* getRaceResult(QList<ServerPlayer*> &players, BP::CommandType command, time_t timeOut,
								ResponseVerifyFunction validateFunc = NULL, void* funcArg = NULL);

	// Verification functions
	bool verifyNullificationResponse(ServerPlayer*, const QJsonValue&, void*);

	// Notification functions
	bool notifyMoveFocus(ServerPlayer* player);
	bool notifyMoveFocus(ServerPlayer* player, BP::CommandType command);
	// Notify client side to move cards from one place to another place. A movement should always be completed by
	// calling notifyMoveCards in pairs, one with isLostPhase equaling true followed by one with isLostPhase
	// equaling false. The tow phase design is needed because the target player doesn't necessarily gets the 
	// cards that the source player lost. Any trigger during the movement can cause either the target player to
	// be dead or some of the cards to be moved to another place before the target player actually gets it. 
	// @param isLostPhase
	//        Specify whether this is a S_COMMAND_LOSE_CARD notification.
	// @param move
	//        Specify all movements need to be broadcasted.
	// @param forceVisible
	//        If true, all players will be able to see the face of card regardless of whether the movement is
	//        relevant or not.
	bool notifyMoveCards(bool is_lost_phase, QList<CardsMoveStruct> move, bool force_visible);

	void acquireSkill(ServerPlayer *player, const Skill *skill, bool open = true);
	void acquireSkill(ServerPlayer *player, const QString &skill_name, bool open = true);
	void adjustSeats();
	void swapPile();
	QList<int> getDiscardPile();
	QList<int> getDrawPile();
	int getCardFromPile(const QString &card_name);
	QList<ServerPlayer *> findPlayersBySkillName(const QString &skill_name, bool include_dead = false) const;
	ServerPlayer *findPlayer(const QString &general_name, bool include_dead = false) const;
	ServerPlayer *findPlayerBySkillName(const QString &skill_name, bool include_dead = false) const;
	void installEquip(ServerPlayer *player, const QString &equip_name);
	void resetAI(ServerPlayer *player);
	void transfigure(ServerPlayer *player, const QString &new_general, bool full_state, bool invoke_start = true, const QString &old_general = QString(""));
	void swapSeat(ServerPlayer *a, ServerPlayer *b);
	lua_State *getLuaState() const;
	void setFixedDistance(Player *from, const Player *to, int distance);
	void reverseFor3v3(const Card *card, ServerPlayer *player, QList<ServerPlayer *> &list);
	bool hasWelfare(const ServerPlayer *player) const;
	ServerPlayer *getFront(ServerPlayer *a, ServerPlayer *b) const;
	void signup(ServerPlayer *player, const QString &screen_name, const QString &avatar, bool is_robot);
	ServerPlayer *getOwner() const;
	void updateStateItem();

	void reconnect(ServerPlayer *player, ClientSocket *socket);
	void marshal(ServerPlayer *player);

	bool isVirtual();
	void setVirtual();
	void copyFrom(Room* rRoom);
	Room* duplicate();

	void setTag(const QString &key, const QVariant &value);
	QVariant getTag(const QString &key) const;
	void removeTag(const QString &key);

	void setEmotion(ServerPlayer *target, const QString &emotion);

	Player::Place getCardPlace(int card_id) const;
	ServerPlayer *getCardOwner(int card_id) const;
	void setCardMapping(int card_id, ServerPlayer *owner, Player::Place place);

	void drawCards(ServerPlayer *player, int n, const QString &reason = QString());
	void drawCards(QList<ServerPlayer*> players, int n, const QString &reason);
	void obtainCard(ServerPlayer *target, const Card *card, bool unhide = true);
	void obtainCard(ServerPlayer *target, int card_id, bool unhide = true);

	void throwCard(const Card *card, ServerPlayer *who = NULL);
	void throwCard(int card_id, ServerPlayer *who = NULL);

	void moveCardTo(const Card* card, ServerPlayer* dstPlayer, Player::Place dstPlace,
	bool forceMoveVisible = false, bool ignoreChanged = true);
	void moveCardsAtomic(QList<CardsMoveStruct> cards_move, bool force_visible);
	void moveCards(CardsMoveStruct cards_move, bool force_visible, bool enforce_origin = true);
	void moveCards(QList<CardsMoveStruct> cards_moves, bool force_visible, bool enforce_origin = true);
	void _moveCards(QList<CardsMoveStruct> cards_moves, bool force_visible, bool enforce_origin);
	QList<CardsMoveStruct> _breakDownCardMoves(QList<CardsMoveStruct> &cards_moves);

	// interactive methods
	void activate(ServerPlayer *player, CardUseStruct &card_use);
	Card::Suit askForSuit(ServerPlayer *player, const QString &reason);
	QString askForKingdom(ServerPlayer *player);
	bool askForSkillInvoke(ServerPlayer *player, const QString &skill_name, const QVariant &data = QVariant());
	QString askForChoice(ServerPlayer *player, const QString &skill_name, const QString &choices);
	bool askForDiscard(ServerPlayer *target, const QString &reason, int discard_num, int min_num, bool optional = false, bool include_equip = false);
	const Card *askForExchange(ServerPlayer *player, const QString &reason, int discard_num);
	bool askForNullification(const TrickCard *trick, ServerPlayer *from, ServerPlayer *to, bool positive);
	bool isCanceled(const CardEffectStruct &effect);
	int askForCardChosen(ServerPlayer *player, ServerPlayer *who, const QString &flags, const QString &reason);
	const Card *askForCard(ServerPlayer *player, const QString &pattern, const QString &prompt, const QVariant &data = QVariant(), TriggerEvent trigger_event = CardResponsed);
	bool askForUseCard(ServerPlayer *player, const QString &pattern, const QString &prompt);
	int askForAG(ServerPlayer *player, const QList<int> &card_ids, bool refusable, const QString &reason);
	const Card *askForCardShow(ServerPlayer *player, ServerPlayer *requestor, const QString &reason);
	bool askForYiji(ServerPlayer *guojia, QList<int> &cards);
	const Card *askForPindian(ServerPlayer *player, ServerPlayer *from, ServerPlayer *to, const QString &reason);
	ServerPlayer *askForPlayerChosen(ServerPlayer *player, const QList<ServerPlayer *> &targets, const QString &reason);
	QString askForGeneral(ServerPlayer *player, const QStringList &generals, QString default_choice = QString());    
	const Card *askForSingleWine(ServerPlayer *player, ServerPlayer *dying);
	
	void toggleReadyCommand(ServerPlayer *player, const QJsonValue &);
	void speakCommand(ServerPlayer *player, const QJsonValue &content);
	void trustCommand(ServerPlayer *player, const QJsonValue &arg);
	void kickCommand(ServerPlayer *player, const QJsonValue &arg);
	void processResponse(ServerPlayer *player, const BP::Packet* arg);
	void addRobotCommand(ServerPlayer *player, const QJsonValue &arg);
	void fillRobotsCommand(ServerPlayer *player, const QJsonValue &arg);
	void broadcastProperty(ServerPlayer *player, const char *property_name, const QString &value = QString());
	void startTest(const QString &to_test);
	void networkDelayTestCommand(ServerPlayer *player, const QJsonValue &);
	void start();

private:
	QThread thread;
	RoomController *controller;

	struct _MoveSourceClassifier{
		inline _MoveSourceClassifier(const CardsMoveStruct &move)
		{
			m_from = move.from; m_from_place = move.from_place; 
			m_from_pile_name = move.from_pile_name; m_from_player_name = move.from_player_name;
		}
		inline void copyTo(CardsMoveStruct & move)
		{
			move.from = m_from; move.from_place = m_from_place;
			move.from_pile_name = m_from_pile_name; move.from_player_name = m_from_player_name;
		}
		inline bool operator == (const _MoveSourceClassifier &other) const
		{
			return m_from == other.m_from && m_from_place == other.m_from_place &&
				m_from_pile_name == other.m_from_pile_name && m_from_player_name == other.m_from_player_name;
		}
		inline bool operator < (const _MoveSourceClassifier &other) const
		{
			return m_from < other.m_from || m_from_place < other.m_from_place ||
				m_from_pile_name < other.m_from_pile_name || m_from_player_name < other.m_from_player_name;
		}
		Player* m_from;
		Player::Place m_from_place;
		QString m_from_pile_name;
		QString m_from_player_name; 
	};
	int last_movement_id;
	void _fillMoveInfo(CardMoveStruct &move) const;
	void _fillMoveInfo(CardsMoveStruct &moves, int card_index) const;
	QString _chooseDefaultGeneral(ServerPlayer* player) const;
	bool _setPlayerGeneral(ServerPlayer* player, const QString& generalName, bool isFirst);
	QString mode;
	QList<ServerPlayer*> m_players, m_alivePlayers;
	int player_count;
	ServerPlayer *current;
	QList<int> pile1, pile2, pile3;
	QList<int> table_cards;
	QList<int> *draw_pile, *discard_pile, *handling_area;
	bool game_started;
	bool game_finished;
	lua_State *L;
	QList<AI *> ais;

	Server *server;
	RoomDriver *driver;
	RoomDriver3v3 *driver_3v3;
	RoomDriver1v1 *driver_1v1;
	QSemaphore *sem; // Legacy semaphore, expected to be reomved after new synchronization is fully deployed.
	QSemaphore sem_race_request; // When race starts, server waits on his semaphore for the first replier
	QSemaphore sem_room_mutex; // Provide per-room  (rather than per-player) level protection of any shared variables

	
	QHash<BP::CommandType, CallBack> callbacks; // Stores the callbacks for client request & notification
	QHash<BP::CommandType, BP::CommandType> request_response_pair; 
		// Stores the expected client response for each server request, any unmatched client response will be discarded.

	QTime _m_timeSinceLastSurrenderRequest; // Timer used to ensure that surrender polls are not initiated too frequently
	bool _m_isFirstSurrenderRequest; // We allow the first surrender poll to go through regardless of the timer.
	
	//helper variables for race request function
	bool race_started; 
	ServerPlayer* _m_raceWinner;

	QMap<int, Player::Place> place_map;
	QMap<int, ServerPlayer*> owner_map;

	const Card *provided;
	bool has_provided;

	QVariantMap tag;
	const Scenario *scenario;

	bool surrender_request_received;
	bool _virtual;

	static QString generatePlayerName();
	void prepareForStart();
	void assignGeneralsForPlayers(const QList<ServerPlayer *> &to_assign);
	void chooseGenerals();
	AI *cloneAI(ServerPlayer *player);
	void broadcast(const QString &message, ServerPlayer *except = NULL);
	void broadcast(const BP::AbstractPacket &packet, ServerPlayer *except = NULL);
	void arrangeCommand(ServerPlayer *player, const QJsonValue &arg);
	void takeGeneralCommand(ServerPlayer *player, const QJsonValue &arg);
	QString askForOrder(ServerPlayer *player);
	QString askForRole(ServerPlayer *player, const QStringList &roles, const QString &scheme);

	//process client requests
	void processRequestCheat(ServerPlayer *player, const QJsonValue &cheat);
	void processRequestSurrender(ServerPlayer *player, const QJsonValue &);

	bool makeSurrender(ServerPlayer* player);
	bool makeCheat(ServerPlayer* player);
	void makeDamage(const QString& source, const QString& target, BP::Cheat::Category nature, int point);
	void makeKilling(const QString& killer, const QString& victim);
	void makeReviving(const QString &name);
	void doScript(const QString &script);

	//helper functions and structs
	struct _NullificationAiHelper
	{
		const TrickCard* m_trick;
		ServerPlayer* m_from;
		ServerPlayer* m_to;
	};
	bool _askForNullification(const TrickCard *trick, ServerPlayer *from, ServerPlayer *to, bool positive, _NullificationAiHelper helper);
	void _setupChooseGeneralRequestArgs(ServerPlayer *player);    

private slots:
	void processClientPacket(const QByteArray &packet);
	void reportDisconnection();
	void assignRoles();
	void startGame();
	void abortGame();

signals:
	void room_message(const QString &msg);
	void game_start();
	void game_over(const QString &winner);
	void room_start();
	void ready_to_close();
};

typedef Room *RoomStar;
Q_DECLARE_METATYPE(RoomStar)

class RoomController: public QObject{
	Q_OBJECT
public:
	RoomController(Room *room);

public slots:
	void run();

private:
	Room *room;
};

#endif // ROOM_H
