#ifndef CLIENT_H
#define CLIENT_H

#include "clientplayer.h"
#include "card.h"
#include "skill.h"
#include "socket.h"
#include "clientstruct.h"
#include "protocol.h"

class NullificationDialog;
class Recorder;
class Replayer;
class QTextDocument;

class Client : public QObject
{
	Q_OBJECT
	Q_PROPERTY(Client::Status status READ getStatus WRITE setStatus)

	Q_ENUMS(Status)

public:
	enum Status{
		NotActive,
		Responsing,
		Playing,
		Discarding,
		ExecDialog,
		AskForSkillInvoke,
		AskForAG,
		AskForPlayerChoose,
		AskForYiji,
		AskForGuanxing,
		AskForGongxin
	};

	explicit Client(QObject *parent, const QString &filename = QString());

	void roomBegin(const QString &begin_str);
	void room(const QString &room_str);
	void roomEnd(const QString &);
	void roomCreated(const QString &idstr);
	void roomError(const QString &errorStr);
	void hallEntered(const QString &);

	// cheat functions
	void requestCheatGetOneCard(int card_id);
	void requestCheatChangeGeneral(QString name);
	void requestCheatKill(const QString& killer, const QString& victim);
	void requestCheatDamage(const QString& source, const QString& target, DamageStruct::Nature nature, int points);
	void requestCheatRevive(const QString& name);
	void requestCheatRunScript(const QString& script);

	// other client requests
	void requestSurrender();

	void disconnectFromHost();
	void replyToServer(BP::CommandType command, const QJsonValue &arg = QJsonValue());
	void requestToServer(BP::CommandType command, const QJsonValue &arg = QJsonValue());
	void notifyServer(BP::CommandType command, const QJsonValue &arg = QJsonValue());
	void request(const QString &raw_message);
	void onPlayerUseCard(const Card *card, const QList<const Player *> &targets = QList<const Player *>());
	void setStatus(Status status);
	Status getStatus() const;
	int alivePlayerCount() const;    
	bool hasNoTargetResponsing() const;
	void onPlayerResponseCard(const Card *card);
	void onPlayerInvokeSkill(bool invoke);
	void onPlayerDiscardCards(const Card *card);
	void onPlayerReplyYiji(const Card *card, const Player *to);
	void onPlayerReplyGuanxing(const QList<int> &up_cards, const QList<int> &down_cards);
	void onPlayerAssignRole(const QList<QString> &names, const QList<QString> &roles);
	QList<const ClientPlayer *> getPlayers() const;
	void speakToServer(const QString &text);
	ClientPlayer *getPlayer(const QString &name);
	void kick(const QString &to_kick);
	bool save(const QString &filename) const;
	void setLines(const QString &skill_name);
	QString getSkillLine() const;
	Replayer *getReplayer() const;
	QString getPlayerName(const QString &str);
	QString getPattern() const;
	QString getSkillNameToInvoke() const;    

	QTextDocument *getLinesDoc() const;
	QTextDocument *getPromptDoc() const;

	typedef void (Client::*Callback)(const QString &);
	typedef void (Client::*CallBack)(const QJsonValue &);

	void checkVersion(const QJsonValue &server_version);
	void setup(const QJsonValue &setup);
	void networkDelayTest(const QJsonValue &);
	void addPlayer(const QJsonValue &player_info);
	void removePlayer(const QJsonValue &player_name);
	void drawCards(const QString &cards_str);
	void drawNCards(const QString &draw_str);    
	void startInXSeconds(const QJsonValue &);
	void arrangeSeats(const QJsonValue &seats_data);
	void activate(const QJsonValue &playerId);
	void startGame(const QJsonValue &);
	void hpChange(const QJsonValue &change_str);
	void playSkillEffect(const QString &play_str);
	void playCardEffect(const QString &play_str);
	void playAudio(const QString &name);
	void resetPiles(const QString &);
	void setPileNumber(const QString &pile_num);
	void gameOver(const QJsonValue &);
	void loseCards(const QJsonValue &data);
	void getCards(const QJsonValue &data);
	void killPlayer(const QJsonValue &player_name);
	void revivePlayer(const QJsonValue &player_name);
	void warn(const QJsonValue &);
	void setMark(const QJsonValue &mark_str);
	void doFilter(const QJsonValue &);
	void showCard(const QJsonValue &data);
	void log(const QJsonValue &log_str);
	void speak(const QJsonValue &speak_data);
	void addHistory(const QString &card);
	void moveFocus(const QJsonValue &focus);
	void setEmotion(const QJsonValue &set_str);
	void skillInvoked(const QJsonValue &invoke_str);
	void acquireSkill(const QJsonValue &acquire_arr);
	void animate(const QString &animate_str);
	void jilei(const QString &jilei_str);
	void cardLock(const QString &card_str);
	void judgeResult(const QString &result_str);
	void setScreenName(const QJsonValue &set_str);
	void setFixedDistance(const QString &set_str);
	void pile(const QString &pile_str);
	void transfigure(const QString &transfigure_tr);
	void updateStateItem(const QString &state_str);
	void setStatistics(const QString &property_str);
	void setCardFlag(const QString &pattern_str);

	void fillAG(const QString &cards_str);    
	void takeAG(const QString &take_str);
	void clearAG(const QString &);

	//interactive server callbacks
	void askForCard(const QJsonValue &);
	void askForUseCard(const QJsonValue &);
	void askForAG(const QJsonValue&);
	void askForSingleWine(const QJsonValue &);
	void askForCardShow(const QJsonValue &);
	void askForSkillInvoke(const QJsonValue &);
	void askForChoice(const QJsonValue &);
	void askForDiscard(const QJsonValue &);
	void askForExchange(const QJsonValue &);
	void askForSuit(const QJsonValue &);
	void askForKingdom(const QJsonValue &);
	void askForNullification(const QJsonValue &);
	void askForPindian(const QJsonValue &data);
	void askForCardChosen(const QJsonValue &);
	void askForPlayerChosen(const QJsonValue &);
	void askForGeneral(const QJsonValue &);
	void askForYiji(const QJsonValue &data);
	void askForGuanxing(const QJsonValue &data);
	void askForGongxin(const QJsonValue &data);
	void askForAssign(const QJsonValue &); // Assign roles at the beginning of game
	void askForSurrender(const QJsonValue &);
	//3v3 & 1v1
	void askForOrder(const QJsonValue &);
	void askForRole3v3(const QJsonValue &);
	void askForDirection(const QJsonValue &);    

	// 3v3 & 1v1 methods
	void fillGenerals(const QString &generals);
	void askForGeneral3v3(const QString &);
	void takeGeneral(const QString &take_str);
	void startArrange(const QString &);    
	
	void recoverGeneral(const QString &);
	void revealGeneral(const QString &);

	void attachSkill(const QJsonValue &skill_name);
	void detachSkill(const QJsonValue &skill_name);
	
	inline void setCountdown(BP::Countdown countdown) {
		countdown_mutex.lock();
		this->countdown = countdown;
		countdown_mutex.unlock();
	}

	inline BP::Countdown getCountdown(){
		countdown_mutex.lock();
		BP::Countdown countdown = countdown;
		countdown_mutex.unlock();
		return countdown;
	}

	// public fields
	bool is_discard_action_refusable;
	bool can_discard_equip;
	int discard_num;
	int min_num;
	QString skill_name;
	QList<const Card*> discarded_list;
	QStringList players_to_choose;    

public slots:
	void signup();
	void onPlayerChooseGeneral(const QString &_name);
	void onPlayerMakeChoice();
	void onPlayerChooseCard(int card_id = -2);
	void onPlayerChooseAG(int card_id);
	void onPlayerChoosePlayer(const Player *player);
	void trust();
	void addRobot();
	void fillRobots();
	void arrange(const QStringList &order);
	
	void onPlayerReplyGongxin(int card_id = -1);

protected:
	// operation countdown
	BP::Countdown countdown;
	// sync objects    
	QMutex countdown_mutex;

private:
	ClientSocket *socket;
	bool m_isGameOver;
	Status status;
	int alive_count;
	QHash<QString, Callback> oldcallbacks;
	QHash<BP::CommandType, CallBack> interactions;
	QHash<BP::CommandType, CallBack> callbacks;
	QList<const ClientPlayer *> players;
	bool m_isUseCard;
	QStringList ban_packages;
	Recorder *recorder;
	Replayer *replayer;
	QTextDocument *lines_doc, *prompt_doc;
	int pile_num;
	QString skill_title, skill_line;
	QString card_pattern;
	QString skill_to_invoke;
	int swap_pile;

	unsigned int last_server_serial;

	void updatePileNum();
	void setPromptList(const QStringList &text);
	void commandFormatWarning(const QString &str, const QRegExp &rx, const char *command);

	void _askForCardOrUseCard(const QJsonValue&);
	bool _loseSingleCard(int card_id, CardsMoveStruct move);
	bool _getSingleCard(int card_id, CardsMoveStruct move);

private slots:
	void processServerPacket(const QString &cmd);
	void processServerPacket(char *cmd);
	bool processServerRequest(const BP::Packet &packet);
	void processReply(char *reply);
	void notifyRoleChange(const QString &new_role);
	void onPlayerChooseSuit();
	void onPlayerChooseKingdom();
	void clearTurnTag();
	void onPlayerChooseOrder();
	void onPlayerChooseRole3v3();

signals:
	void version_checked(const QString &version_number, const QString &mod_name);
	void server_connected();
	void error_message(const QString &msg);
	void player_added(ClientPlayer *new_player);
	void player_removed(const QString &player_name);
	// choice signal
	void generals_got(const QStringList &generals);
	void kingdoms_got(const QStringList &kingdoms);
	void suits_got(const QStringList &suits);
	void options_got(const QString &skillName, const QStringList &options);
	void cards_got(const ClientPlayer *player, const QString &flags, const QString &reason);
	void roles_got(const QString &scheme, const QStringList &roles);
	void directions_got();    
	void orders_got(BP::Game3v3ChooseOrderCommand reason);
	
	void seats_arranged(const QList<const ClientPlayer*> &seats);
	void hp_changed(const QString &who, int delta, DamageStruct::Nature nature, bool losthp);
	void status_changed(Client::Status oldStatus, Client::Status newStatus);
	void avatars_hiden();
	void pile_reset();
	void player_killed(const QString &who);
	void player_revived(const QString &who);
	void card_shown(const QString &player_name, int card_id);
	void log_received(const QString &log_str);
	void guanxing(const QList<int> &card_ids, bool up_only);
	void gongxin(const QList<int> &card_ids, bool enable_heart);
	void focus_moved(const QString &focus, BP::Countdown countdown);
	void emotion_set(const QString &target, const QString &emotion);
	void skill_invoked(const QString &who, const QString &skill_name);
	void skill_acquired(const ClientPlayer *player, const QString &skill_name);
	void animated(const QString &name, const QStringList &args);
	void text_spoken(const QString &text);
	void line_spoken(const QString &line);
	void judge_result(const QString &who, const QString &result);
	void card_used();

	void game_started();
	void game_over();
	void standoff();
		
	void move_cards_lost(int moveId, QList<CardsMoveStruct> moves);
	void move_cards_got(int moveId, QList<CardsMoveStruct> moves);

	void skill_attached(const QString &skill_name, bool from_left);
	void skill_detached(const QString &skill_name);
	void do_filter();

	void ag_filled(const QList<int> &card_ids);
	void ag_taken(ClientPlayer *taker, int card_id);
	void ag_cleared();

	void generals_filled(const QStringList &general_names);
	void general_taken(const QString &who, const QString &name);
	void general_asked();
	void arrange_started();
	void general_recovered(int index, const QString &name);
	void general_revealed(bool self, const QString &general);

	void role_state_changed(const QString & state_str);

	void assign_asked();
	void start_in_xs();
};

extern Client *ClientInstance;

#endif // CLIENT_H
