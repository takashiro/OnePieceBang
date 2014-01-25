#include "client.h"
#include "settings.h"
#include "engine.h"
#include "standard.h"
#include "choosegeneraldialog.h"
#include "nativesocket.h"
#include "recorder.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QMessageBox>
#include <QCheckBox>
#include <QCommandLinkButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QTextDocument>
#include <QTextCursor>

Client *ClientInstance = NULL;

Client::Client(QObject *parent, const QString &filename)
	:QObject(parent), is_discard_action_refusable(true),
	status(NotActive), alive_count(1), swap_pile(0)
{

	ClientInstance = this;
	is_game_over = false;

	callbacks[BP::CheckVersion] = &Client::checkVersion;

	/*oldcallbacks["roomBegin"] = &Client::roomBegin;
	oldcallbacks["room"] = &Client::room;
	oldcallbacks["roomEnd"] = &Client::roomEnd;
	oldcallbacks["roomCreated"] = &Client::roomCreated;
	oldcallbacks["roomError"] = &Client::roomError;
	oldcallbacks["hallEntered"] = &Client::hallEntered;*/

	callbacks[BP::Setup] = &Client::setup;
	callbacks[BP::NetworkDelayTest] = &Client::networkDelayTest;
	callbacks[BP::AddPlayer] = &Client::addPlayer;
	callbacks[BP::RemovePlayer] = &Client::removePlayer;
	callbacks[BP::StartInXSeconds] = &Client::startInXSeconds;
	callbacks[BP::ArrangeSeats] = &Client::arrangeSeats;
	callbacks[BP::Warn] = &Client::warn;

	callbacks[BP::StartGame] = &Client::startGame;
	callbacks[BP::GameOver] = &Client::gameOver;

	callbacks[BP::HpChange] = &Client::hpChange;
	callbacks[BP::KillPlayer] = &Client::killPlayer;
	callbacks[BP::RevivePlayer] = &Client::revivePlayer;
	callbacks[BP::AskForCardShow] = &Client::showCard;
	callbacks[BP::SetMark] = &Client::setMark;
	callbacks[BP::DoFilter] = &Client::doFilter;//deprecated?
	callbacks[BP::Log] = &Client::log;
	callbacks[BP::Speak] = &Client::speak;
	callbacks[BP::AcquireSkill] = &Client::acquireSkill;
	callbacks[BP::AttachSkill] = &Client::attachSkill;
	callbacks[BP::DetachSkill] = &Client::detachSkill;
	callbacks[BP::MoveFocus] = &Client::moveFocus;
	callbacks[BP::SetEmotion] = &Client::setEmotion;
	callbacks[BP::AskForSkillInvoke] = &Client::skillInvoked;
	callbacks[BP::ShowAllCards] = &Client::askForGongxin;
	callbacks[BP::AskForGongxin] = &Client::askForGongxin;
	callbacks[BP::AddHistory] = &Client::addHistory;
	callbacks[BP::Animate] = &Client::animate;
	callbacks[BP::JudgeResult] = &Client::judgeResult;
	callbacks[BP::SetScreenName] = &Client::setScreenName;
	callbacks[BP::SetFixedDistance] = &Client::setFixedDistance;
	callbacks[BP::Transfigure] = &Client::transfigure;
	//callbacks[BP::Jilei] = &Client::jilei;
	callbacks[BP::CardLock] = &Client::cardLock;
	callbacks[BP::Pile] = &Client::pile;

	callbacks[BP::UpdateStateItem] = &Client::updateStateItem;

	callbacks[BP::PlaySkillEffect] = &Client::playSkillEffect;
	callbacks[BP::PlayCardEffect] = &Client::playCardEffect;
	callbacks[BP::PlayAudio] = &Client::playAudio;

	callbacks[BP::GetCard] = &Client::getCards;
	callbacks[BP::LoseCard] = &Client::loseCards;
	callbacks[BP::ClearPile] = &Client::resetPiles;
	callbacks[BP::SetPileNumber] = &Client::setPileNumber;
	callbacks[BP::SetStatistics] = &Client::setStatistics;
	callbacks[BP::SetCardFlag] = &Client::setCardFlag;
	callbacks[BP::SetPlayerProperty] = &Client::setPlayerProperty;

	// interactive methods    
	interactions[BP::AskForGeneral] = &Client::askForGeneral;
	interactions[BP::AskForPlayerChosen] = &Client::askForPlayerChosen;
	interactions[BP::AskForAssign] = &Client::askForAssign;
	interactions[BP::AskForDirection] = &Client::askForDirection;
	interactions[BP::AskForExchange] = &Client::askForExchange;
	interactions[BP::AskForSingleWine] = &Client::askForSingleWine;
	interactions[BP::AskForGuanxing] = &Client::askForGuanxing;
	interactions[BP::AskForGongxin] = &Client::askForGongxin;
	interactions[BP::AskForYiji] = &Client::askForYiji;
	interactions[BP::Activate] = &Client::activate;
	interactions[BP::AskForDiscard] = &Client::askForDiscard;
	interactions[BP::AskForSuit] = &Client::askForSuit;
	interactions[BP::AskForKingdom] = &Client::askForKingdom;
	interactions[BP::AskForCard] = &Client::askForCard;
	interactions[BP::AskForUseCard] = &Client::askForUseCard;
	interactions[BP::AskForSkillInvoke] = &Client::askForSkillInvoke;
	interactions[BP::AskForChoice] = &Client::askForChoice;
	interactions[BP::AskForNullification] = &Client::askForNullification;
	interactions[BP::AskForCardShow] = &Client::askForCardShow;
	interactions[BP::AskForAG] = &Client::askForAG;
	interactions[BP::AskForPindian] = &Client::askForPindian;
	interactions[BP::AskForCardChosen] = &Client::askForCardChosen;
	interactions[BP::AskForOrder] = &Client::askForOrder;
	interactions[BP::AskForRole3v3] = &Client::askForRole3v3;
	interactions[BP::AskForSurrender] = &Client::askForSurrender;

	callbacks[BP::FillAG] = &Client::fillAG;
	callbacks[BP::TakeAG] = &Client::takeAG;
	callbacks[BP::ClearAG] = &Client::clearAG;

	// 3v3 mode & 1v1 mode
	callbacks[BP::FillGenerals] = &Client::fillGenerals;
	callbacks[BP::AskForGeneral3v3] = &Client::askForGeneral3v3;
	callbacks[BP::AskForGeneral1v1] = &Client::askForGeneral3v3;
	callbacks[BP::TakeGeneral] = &Client::takeGeneral;
	callbacks[BP::StartArrange] = &Client::startArrange;
	callbacks[BP::RecoverGeneral] = &Client::recoverGeneral;
	callbacks[BP::RevealGeneral] = &Client::revealGeneral;

	m_isUseCard = false;

	Self = new ClientPlayer(this);
	Self->setScreenName(Config.UserName);
	Self->setProperty("avatar", Config.UserAvatar);
	connect(Self, SIGNAL(phase_changed()), this, SLOT(clearTurnTag()));
	connect(Self, SIGNAL(role_changed(QString)), this, SLOT(notifyRoleChange(QString)));

	players << Self;

	if(!filename.isEmpty()){
		socket = NULL;
		recorder = NULL;

		replayer = new Replayer(this, filename);
		connect(replayer, SIGNAL(command_parsed(QString)), this, SLOT(processServerPacket(QString)));
	}else{
		socket = new NativeClientSocket;
		socket->setParent(this);

		recorder = new Recorder(this);

		connect(socket, SIGNAL(message_got(char*)), recorder, SLOT(record(char*)));
		connect(socket, SIGNAL(message_got(char*)), this, SLOT(processServerPacket(char*)));
		connect(socket, SIGNAL(error_message(QString)), this, SIGNAL(error_message(QString)));
		socket->connectToHost();

		replayer = NULL;
	}

	lines_doc = new QTextDocument(this);

	prompt_doc = new QTextDocument(this);
	prompt_doc->setTextWidth(350);
	prompt_doc->setDefaultFont(QFont("SimHei"));
}

void Client::signup(){
	if(replayer)
		replayer->start();
	else{
		BP::CommandType command = Config.value("EnableReconnection", false).toBool() ? BP::SignUpR : BP::SignUp;

		QJsonArray signup_arr;
		signup_arr.append(Config.UserName);
		signup_arr.append(Config.UserAvatar);
		QString password = Config.Password;
		if(!password.isEmpty()){
			password = QCryptographicHash::hash(password.toLatin1(), QCryptographicHash::Md5).toHex();
			signup_arr.append(password);
		}

		notifyServer(command, signup_arr);
	}
}

void Client::networkDelayTest(const QJsonValue &){
	notifyServer(BP::NetworkDelayTest);
}

void Client::replyToServer(BP::CommandType command, const QJsonValue &arg){
	if(socket){
		BP::Packet packet(BP::ClientReply, command);
		packet.local_serial = last_server_serial;
		packet.setMessageBody(arg);
		socket->send(packet);
	}
}

void Client::requestToServer(BP::CommandType command, const QJsonValue &arg){
	if(socket){
		BP::Packet packet(BP::ClientRequest, command);
		packet.setMessageBody(arg);
		socket->send(packet);
	}
}

void Client::notifyServer(BP::CommandType command, const QJsonValue &arg){
	if(socket){
		BP::Packet packet(BP::ClientNotification, command);
		packet.setMessageBody(arg);
		socket->send(packet);
	}
}

void Client::request(const QString &raw_message){
	if(socket)
		socket->send(raw_message.toUtf8());
}

void Client::checkVersion(const QJsonValue &server_version){
	QString version = server_version.toString();

	QString version_number, mod_name;
	if(version.contains(QChar(':'))){
		QStringList texts = version.split(QChar(':'));
		version_number = texts.value(0);
		mod_name = texts.value(1);
	}else{
		version_number = version;
		mod_name = "official";
	}

	emit version_checked(version_number, mod_name);
}

void Client::setup(const QJsonValue &setup){
	if(socket && !socket->isConnected())
		return;

	QString setup_str = setup.toString();
	if(ServerInfo.parse(setup_str)){
		emit server_connected();
		notifyServer(BP::ToggleReady);
	}else{
		QMessageBox::warning(NULL, tr("Warning"), tr("Setup string can not be parsed: %1").arg(setup_str));
	}
}

void Client::disconnectFromHost(){
	if(socket){
		socket->disconnectFromHost();
		socket = NULL;
	}
}

typedef char buffer_t[1024];

void Client::processServerPacket(const QString &cmd){
	processServerPacket(cmd.toUtf8().data());
}

void Client::processServerPacket(char *cmd){
	if(is_game_over) return;

	BP::Packet packet;
	if(packet.parse(cmd)){
		if(packet.getPacketType() == BP::ServerNotification){
			Callback callback = callbacks[packet.getCommandType()];
			if(callback){
				(this->*callback)(packet.getMessageBody());
			}else{
				QMessageBox::warning(NULL, tr("Warning"), tr("No such invokable method id \"%1\"").arg((int) packet.getCommandType()));
			}
		}else if(packet.getPacketType() == BP::ServerRequest){
			processServerRequest(packet);
		}
	}else{
		QMessageBox::warning(NULL, tr("Warning"), tr("Invalid packet received by client:%1").arg(cmd));
	}
}

bool Client::processServerRequest(const BP::Packet &packet){
	setStatus(Client::NotActive);

	last_server_serial = packet.global_serial;

	BP::CommandType command = packet.getCommandType();
	QJsonValue msg = packet.getMessageBody();

	BP::Countdown countdown;
	countdown.current = 0;

	bool use_default = !msg.isArray();
	if(!use_default){
		QJsonArray arr = msg.toArray();
		if(arr.size() <= 1 || !countdown.tryParse(arr.first())){
			use_default = true;
		}
	}
	if(use_default){
		countdown.type = BP::Countdown::UseDefault;
		countdown.max = ServerInfo.getCommandTimeout(command, BP::ClientInstance);
	}

	setCountdown(countdown);

	Callback callback = interactions[command];
	if(!callback){
		return false;
	}
	(this->*callback)(msg);

	return true;
}

void Client::setPlayerProperty(const QJsonValue &cmd){
	QJsonArray texts = cmd.toArray();

	bool self = texts.at(0).toBool();

	if(self){
		//client it Self
		if(Self){
			QString property = texts.at(1).toString();
			QVariant value = texts.at(2).toVariant();
			Self->setProperty(property.toLatin1().data(), value);
		}
	}else{
		//others
		QString object_name = texts.at(1).toString();
		QString property = texts.at(2).toString();
		QVariant value = texts.at(3).toVariant();
		ClientPlayer *player = getPlayer(object_name);
		if(player){
			player->setProperty(property.toLatin1().data(), value);
		}else{
			QMessageBox::warning(NULL, tr("Warning"), tr("There is no player named %1").arg(object_name));
		}
	}
}

void Client::addPlayer(const QJsonValue &player_info){
	QJsonArray texts = player_info.toArray();
	QString name = texts.at(0).toString();
	QString screen_name = texts.at(1).toString();
	QString avatar = texts.at(2).toString();

	ClientPlayer *player = new ClientPlayer(this);
	player->setObjectName(name);
	player->setScreenName(screen_name);
	player->setProperty("avatar", avatar);

	players << player;

	alive_count++;

	emit player_added(player);
}

void Client::removePlayer(const QJsonValue &player_name){
	QString name = player_name.toString();
	ClientPlayer *player = findChild<ClientPlayer*>(name);
	if(player){
		player->setParent(NULL);

		alive_count--;

		emit player_removed(name);
	}
}

bool Client::_loseSingleCard(int card_id, CardsMoveStruct move){
	const Card *card = Bang->getCard(card_id);
	if(move.from)
		move.from->removeCard(card, move.from_place);
	else
	{
		// @todo: synchronize discard pile when "marshal"
		if(move.from_place == Player::DiscardPile)
			discarded_list.removeOne(card);
		else if(move.from_place == Player::DrawPile && !Self->hasFlag("marshalling"))
				pile_num--;        
	}
	updatePileNum();
	return true;
}

bool Client::_getSingleCard(int card_id, CardsMoveStruct move){
	const Card *card = Bang->getCard(card_id);
	if(move.to)
		move.to->addCard(card, move.to_place);
	else
	{
		if(move.to_place == Player::DrawPile)
			pile_num++;
		// @todo: synchronize discard pile when "marshal"
		else if(move.to_place == Player::DiscardPile)
			discarded_list.prepend(card);        
	}

	updatePileNum();
	return true;
}

void Client::getCards(const QJsonValue& data){
	Q_ASSERT(data.isArray());
	QJsonArray arg = data.toArray();
	Q_ASSERT(arg.size() >= 1);
	int moveId = arg[0].toDouble();
	QList<CardsMoveStruct> moves;
	for (unsigned int i = 1; i < arg.size(); i++){
		CardsMoveStruct move;
		if (!move.tryParse(arg[i])) return;
		move.from = getPlayer(move.from_player_name);
		move.to = getPlayer(move.to_player_name);
		Player::Place dstPlace = move.to_place;        
	
		if (dstPlace == Player::SpecialArea)
			((ClientPlayer*)move.to)->changePile(move.to_pile_name, true, move.card_ids);
		else{
			foreach (int card_id, move.card_ids)
				_getSingleCard(card_id, move); // DDHEJ->DDHEJ, DDH/EJ->EJ
		}
		moves.append(move);
	}
	emit move_cards_got(moveId, moves);
}

void Client::loseCards(const QJsonValue& data){
	Q_ASSERT(data.isArray());
	QJsonArray arg = data.toArray();
	Q_ASSERT(arg.size() >= 1);
	int moveId = arg[0].toDouble();
	QList<CardsMoveStruct> moves;
	for (unsigned int i = 1; i < arg.size(); i++)
	{
		CardsMoveStruct move;
		if (!move.tryParse(arg[i])) return;
		move.from = getPlayer(move.from_player_name);
		move.to = getPlayer(move.to_player_name);
		Player::Place srcPlace = move.from_place;   
		
		if (srcPlace == Player::SpecialArea)        
			((ClientPlayer*)move.from)->changePile(move.from_pile_name, false, move.card_ids);
		else {
			foreach (int card_id, move.card_ids)
				_loseSingleCard(card_id, move); // DDHEJ->DDHEJ, DDH/EJ->EJ
		}
		moves.append(move);
	}
	emit move_cards_lost(moveId, moves);    
}

void Client::onPlayerChooseGeneral(const QString &item_name){
	setStatus(Client::NotActive);
	if(!item_name.isEmpty()){
		replyToServer(BP::AskForGeneral, item_name);
		Bang->playAudio("choose-item");
	}

}

void Client::requestCheatRunScript(const QString &script){
	QJsonArray cheatReq;
	cheatReq.append((int) BP::Cheat::RunScript);
	cheatReq.append(script);
	requestToServer(BP::RequestCheat, cheatReq);
}

void Client::requestCheatRevive(const QString& name)
{
	QJsonArray cheatReq;
	cheatReq.append((int) BP::Cheat::RevivePlayer);
	cheatReq.append(name);
	requestToServer(BP::RequestCheat, cheatReq);
}

void Client::requestCheatDamage(const QString& source, const QString& target, DamageStruct::Nature nature, int points)
{
	QJsonArray cheatReq, cheatArg;
	cheatArg.append(source);
	cheatArg.append(target);
	cheatArg.append((int)nature);
	cheatArg.append(points);

	cheatReq.append((int) BP::Cheat::Damage);
	cheatReq.append(cheatArg);
	requestToServer(BP::RequestCheat, cheatReq);
}

void Client::requestCheatKill(const QString& killer, const QString& victim)
{
	QJsonArray cheatArg;
	cheatArg.append((int) BP::Cheat::KillPlayer);
	cheatArg.append(BP::toJsonArray(killer, victim));
	requestToServer(BP::RequestCheat, cheatArg);
}

void Client::requestCheatGetOneCard(int card_id){
	QJsonArray cheatArg;
	cheatArg.append((int) BP::Cheat::GetOneCard);
	cheatArg.append(card_id);
	requestToServer(BP::RequestCheat, cheatArg);
}

void Client::requestCheatChangeGeneral(QString name){
	QJsonArray cheatArg;
	cheatArg.append((int) BP::Cheat::ChangeGeneral);
	cheatArg.append(name);
	requestToServer(BP::RequestCheat, cheatArg);
}

void Client::addRobot(){
	notifyServer(BP::AddRobot);
}

void Client::fillRobots(){
	notifyServer(BP::FillRobots);
}

void Client::arrange(const QStringList &order){
	Q_ASSERT(order.length() == 3);
	notifyServer(BP::Arrange, BP::toJsonArray(order));
}

void Client::onPlayerUseCard(const Card *card, const QList<const Player *> &targets){

	if(card == NULL){
		replyToServer(BP::AskForUseCard, QJsonValue());
	}else{
		QJsonArray targetNames;
		foreach(const Player *target, targets)
		targetNames.append(target->objectName());

		replyToServer(BP::AskForUseCard, BP::toJsonArray(card->toString(), targetNames));

		if(status == Responsing)
			card_pattern.clear();
	}

	setStatus(NotActive);
}

void Client::startInXSeconds(const QJsonValue &left_seconds){
	int seconds = left_seconds.toDouble();
	if (seconds > 0)
		lines_doc->setHtml(tr("<p align = \"center\">Game will start in <b>%1</b> seconds...</p>").arg(seconds));
	else
		lines_doc->setHtml(QString());

	emit start_in_xs();
	if(seconds == 0 && Bang->getScenario(ServerInfo.GameMode) == NULL){
		emit avatars_hiden();
	}
}

void Client::arrangeSeats(const QJsonValue &seats_data){
	QJsonArray player_names = seats_data.toArray();
	players.clear();
	
	for (int i = 0; i < player_names.size(); i++){
		ClientPlayer *player = findChild<ClientPlayer*>(player_names.at(i).toString());

		Q_ASSERT(player != NULL);

		player->setSeat(i + 1);
		players << player;
	}

	QList<const ClientPlayer*> seats;
	int self_index = players.indexOf(Self);

	Q_ASSERT(self_index != -1);

	for (int i = self_index+1; i < players.length(); i++)
		seats.append(players.at(i));
	for(int i = 0; i < self_index; i++)
		seats.append(players.at(i));

	Q_ASSERT(seats.length() == players.length() - 1);

	emit seats_arranged(seats);
}

void Client::notifyRoleChange(const QString &new_role){
	if(!new_role.isEmpty()){
		QString prompt_str = tr("Your role is %1").arg(Bang->translate(new_role));
		if(new_role != "lord")
			prompt_str += tr("\n wait for the lord player choosing general, please");
		lines_doc->setHtml(prompt_str);
	}
}

void Client::activate(const QJsonValue& playerId){    
	if(playerId.toString() == Self->objectName())
		setStatus(Playing);
	else
		setStatus(NotActive);
}

void Client::startGame(const QJsonValue &){
	QList<ClientPlayer *> players = findChildren<ClientPlayer *>();
	alive_count = players.count();

	emit game_started();
}

void Client::hpChange(const QJsonValue &change_str){
	QJsonArray texts = change_str.toArray();
	if(texts.size() < 2 || texts.size() > 3){
		return;
	}

	QString who = texts.at(0).toString();
	int delta = (int) texts.at(1).toDouble();
	QString nature_str = texts.at(2).toString();

	DamageStruct::Nature nature;
	if(nature_str == "F")
		nature = DamageStruct::Fire;
	else if(nature_str == "T")
		nature = DamageStruct::Thunder;
	else
		nature = DamageStruct::Normal;

	emit hp_changed(who, delta, nature, nature_str == "L");
}

void Client::setStatus(Status status){    
	Status old_status = this->status;
	this->status = status;
	emit status_changed(old_status, status);
}

Client::Status Client::getStatus() const{
	return status;
}

void Client::jilei(const QJsonValue &jilei_str){
	Self->jilei(jilei_str.toString());
}

void Client::cardLock(const QJsonValue &card_str){
	Self->setCardLocked(card_str.toString());
}

void Client::judgeResult(const QJsonValue &result_str){
	QJsonArray texts = result_str.toArray();
	QString who = texts.at(0).toString();
	QString result = texts.at(1).toString();

	emit judge_result(who, result);
}

QString Client::getSkillLine() const{
	return skill_line;
}

Replayer *Client::getReplayer() const{
	return replayer;
}

QString Client::getPlayerName(const QString &str){
	QRegExp rx("sgs\\d+");
	QString general_name;
	if(rx.exactMatch(str)){
		ClientPlayer *player = getPlayer(str);
		general_name = player->getGeneralName();
		general_name = Bang->translate(general_name);
		if(ServerInfo.EnableSame || player->getGeneralName() == "anjiang")
			general_name = QString("%1[%2]").arg(general_name).arg(player->getSeat());
		return general_name;

	}else
		return Bang->translate(str);
}

QString Client::getPattern() const{
	return card_pattern;
}

QString Client::getSkillNameToInvoke() const{
	return skill_to_invoke;
}

void Client::onPlayerInvokeSkill(bool invoke){    
	if (skill_name == "surrender")
		replyToServer(BP::AskForSurrender, invoke);
	else
		replyToServer(BP::AskForSkillInvoke, invoke);
	setStatus(NotActive);
}

void Client::setPromptList(const QStringList &texts){
	QString prompt = Bang->translate(texts.at(0));
	if(texts.length() >= 2)
		prompt.replace("%src", getPlayerName(texts.at(1)));

	if(texts.length() >= 3)
		prompt.replace("%dest", getPlayerName(texts.at(2)));

	if(texts.length() >= 4){
		QString arg = Bang->translate(texts.at(3));
		prompt.replace("%arg", arg);
	}

	if(texts.length() >= 5){
		QString arg2 = Bang->translate(texts.at(4));
		prompt.replace("%2arg", arg2);
	}

	prompt_doc->setHtml(prompt);
}

void Client::commandFormatWarning(const QString &str, const QRegExp &rx, const char *command){
	QString text = tr("The argument (%1) of command %2 does not conform the format %3")
				   .arg(str).arg(command).arg(rx.pattern());
	QMessageBox::warning(NULL, tr("Command format warning"), text);
}

void Client::_askForCardOrUseCard(const QJsonValue &cardUsageData){
	Q_ASSERT(cardUsageData.isArray());
	QJsonArray cardUsage = cardUsageData.toArray();
	Q_ASSERT(isStringArray(cardUsage, 0, 1));
	card_pattern = cardUsage[0].toString();
	QStringList texts = cardUsage[1].toString().split(":");

	if(texts.isEmpty()){
		return;
	}else
		setPromptList(texts);

	if(card_pattern.endsWith("!"))
		is_discard_action_refusable = false;
	else
		is_discard_action_refusable = true;

	QRegExp rx("^@@?(\\w+)(-card)?$");
	if(rx.exactMatch(card_pattern)){
		QString skill_name = rx.capturedTexts().at(1);
		const Skill *skill = Bang->getSkill(skill_name);
		if(skill){
			QString text = prompt_doc->toHtml();
			text.append(tr("<br/> <b>Notice</b>: %1<br/>").arg(skill->getDescription()));
			prompt_doc->setHtml(text);
		}
	}

	setStatus(Responsing);
}

void Client::askForCard(const QJsonValue &data){
	if(!data.isArray()) return;

	QJsonArray req = data.toArray();
	if (req.size() == 0) return;
	m_isUseCard = req[0].toString().startsWith("@@");
	_askForCardOrUseCard(req);
}

void Client::askForUseCard(const QJsonValue &req){
	m_isUseCard = true;
	_askForCardOrUseCard(req);
}

void Client::askForSkillInvoke(const QJsonValue &argdata){
	if(!argdata.isArray()) return;
	QJsonArray arg = argdata.toArray();
	if (!BP::isStringArray(arg, 0, 1)) return;
	QString skill_name = arg[0].toString();
	QString data = arg[1].toString();

	skill_to_invoke = skill_name;
		
	QString text;
	if(data.isEmpty())
		text = tr("Do you want to invoke skill [%1] ?").arg(Bang->translate(skill_name));
	else
		text = Bang->translate(QString("%1:%2").arg(skill_name).arg(data));

	const Skill *skill = Bang->getSkill(skill_name);
	if(skill){
		text.append(tr("<br/> <b>Notice</b>: %1<br/>").arg(skill->getDescription()));
	}

	prompt_doc->setHtml(text);
	setStatus(AskForSkillInvoke);
}

void Client::onPlayerMakeChoice(){
	QString option = sender()->objectName();
	replyToServer(BP::AskForChoice, option);
	setStatus(NotActive);
}

void Client::askForSurrender(const QJsonValue &initiator){
	
	if (!initiator.isString()) return;    

	QString text = tr("%1 initiated a vote for disadvataged side to claim "
						"capitulation. Click \"OK\" to surrender or \"Cancel\" to resist.")
		.arg(Bang->translate(initiator.toString()));
	text.append(tr("<br/> <b>Noitce</b>: if all people on your side decides to surrender. "
				   "You'll lose this game."));
	skill_name = "surrender";

	prompt_doc->setHtml(text);
	setStatus(AskForSkillInvoke);
}


void Client::playSkillEffect(const QJsonValue &play_str){
	QJsonArray words = play_str.toArray();
	if(words.size() != 2){
		return;
	}

	QString skill_name = words.at(0).toString();
	int index = words.at(1).toDouble();

	Bang->playSkillEffect(skill_name, index);
}

void Client::askForNullification(const QJsonValue &argdata){
	if(!argdata.isArray()) return;
	QJsonArray arg = argdata.toArray();
	if (arg.size() != 3 || !arg[0].isString()
		|| !(arg[1].isNull() ||arg[1].isString())
		|| !arg[2].isString()) return;
	
	QString trick_name = arg[0].toString();
	QJsonValue source_name = arg[1].toString();
	ClientPlayer* target_player = getPlayer(arg[2].toString());

	if (!target_player || !target_player->getGeneral()) return;

	const Card *trick_card = Bang->findChild<const Card *>(trick_name);
	ClientPlayer *source = NULL;
	if(!source_name.isNull())
		source = getPlayer(source_name.toString());

	if(Config.NeverNullifyMyTrick && source == Self){
		if(trick_card->inherits("SingleTargetTrick") || trick_card->objectName() == "tama_dragon"){
			onPlayerResponseCard(NULL);
			return;
		}
	}

	QString trick_path = trick_card->getPixmapPath();
	QString to = target_player->getGeneral()->getPixmapPath("big");
	if(source == NULL){
		prompt_doc->setHtml(QString("<img src='%1' /> ==&gt; <img src='%2' />").arg(trick_path).arg(to));
	}else{
		QString from = source->getGeneral()->getPixmapPath("big");
		prompt_doc->setHtml(QString("<img src='%1' /> <img src='%2'/> ==&gt; <img src='%3' />").arg(trick_path).arg(from).arg(to));
	}

	card_pattern = "nullification";
	is_discard_action_refusable = true;
	m_isUseCard = false;

	setStatus(Responsing);
}

void Client::playAudio(const QJsonValue &name){
	Bang->playAudio(name.toString());
}

void Client::playCardEffect(const QJsonValue &play_str){
	QJsonArray texts = play_str.toArray();

	if(texts.size() == 2){
		QString card_name = texts.at(0).toString();
		bool is_male = texts.at(1).toBool();

		Bang->playCardEffect(card_name, is_male);
	}else if(texts.size() == 3){// old version
		QString card_name = texts.at(0).toString();
		bool is_male = texts.at(3).toBool();

		Bang->playCardEffect("@" + card_name, is_male);
	}
}

void Client::onPlayerChooseCard(int card_id){
	QJsonValue reply;
	if(card_id != -2){   
		reply = card_id;
	}
	replyToServer(BP::AskForCardChosen, reply);
	setStatus(NotActive);
}

void Client::onPlayerChoosePlayer(const Player *player){
	if(player == NULL)
		player = findChild<const Player *>(players_to_choose.first());

	if (player == NULL) return;
	replyToServer(BP::AskForPlayerChosen, player->objectName());
	setStatus(NotActive);
}

void Client::trust(){
	notifyServer(BP::Trust);

	if(Self->getState() == "trust")
		Bang->playAudio("untrust");
	else
		Bang->playAudio("trust");

	setStatus(NotActive);
}

void Client::requestSurrender(){
	requestToServer(BP::AskForSurrender);
	setStatus(NotActive);
}

void Client::speakToServer(const QString &text){
	if(text.isEmpty())
		return;

	notifyServer(BP::Speak, text);
}

void Client::addHistory(const QJsonValue &add_str){
	QString history_str = add_str.toString();
	if(history_str == "pushPile"){
		emit card_used();
		return;
	}

	QRegExp rx("(.+)(#\\d+)?");
	if(rx.exactMatch(history_str)){
		QStringList texts = rx.capturedTexts();
		QString card_name = texts.at(1);
		QString times_str = texts.at(2);

		int times = 1;
		if(!times_str.isEmpty()){
			times_str.remove(QChar('#'));
			times = times_str.toInt();
		}

		Self->addHistory(card_name, times);
	}
}

int Client::alivePlayerCount() const{
	return alive_count;
}

void Client::onPlayerResponseCard(const Card *card){
	if(card)
		replyToServer(BP::AskForCard, card->toString());
		//request(QString("responseCard %1").arg(card->toString()));
	else
		replyToServer(BP::AskForCard, QJsonValue());
		//request("responseCard .");

	card_pattern.clear();
	setStatus(NotActive);
}

bool Client::hasNoTargetResponsing() const{
	return status == Responsing && !m_isUseCard;
}

ClientPlayer *Client::getPlayer(const QString &name){
	if (name == Self->objectName()) return Self;
	else return findChild<ClientPlayer *>(name);
}

void Client::kick(const QString &to_kick){
	notifyServer(BP::Kick, to_kick);
}

bool Client::save(const QString &filename) const{
	if(recorder)
		return recorder->save(filename);
	else
		return false;
}

void Client::setLines(const QString &filename){
	QRegExp rx(".+/(\\w+\\d?).ogg");
	if(rx.exactMatch(filename)){
		QString skill_name = rx.capturedTexts().at(1);
		skill_line = Bang->translate("$" + skill_name);

		QChar last_char = skill_name[skill_name.length()-1];
		if(last_char.isDigit())
			skill_name.chop(1);

		skill_title = Bang->translate(skill_name);

		updatePileNum();
	}
}

QTextDocument *Client::getLinesDoc() const{
	return lines_doc;
}

QTextDocument *Client::getPromptDoc() const{
	return prompt_doc;
}

void Client::resetPiles(const QJsonValue &){
	discarded_list.clear();
	swap_pile++;
	updatePileNum();
	emit pile_reset();
}

void Client::setPileNumber(const QJsonValue &pile_str){
	pile_num = pile_str.toDouble();

	updatePileNum();
}

void Client::setStatistics(const QJsonValue &property_str){
	QJsonArray texts = property_str.toArray();
	if(texts.size() != 2){
		return;
	}

	QString property_name = texts.at(0).toString();
	const QJsonValue &value_str = texts.at(1);

	StatisticsStruct *statistics = Self->getStatistics();
	if(value_str.isDouble())
		statistics->setStatistics(property_name, value_str.toDouble());
	else
		statistics->setStatistics(property_name, value_str.toString());

	Self->setStatistics(statistics);
}

void Client::setCardFlag(const QJsonValue &pattern){
	QJsonArray texts = pattern.toArray();

	int card_id = texts.at(0).toDouble();
	QString object = texts.at(1).toString();

	Bang->getCard(card_id)->setFlags(object);
}

void Client::updatePileNum(){
	QString pile_str = tr("Draw pile: <b>%1</b>, discard pile: <b>%2</b>, swap times: <b>%3</b>")
					   .arg(pile_num).arg(discarded_list.length()).arg(swap_pile);
	lines_doc->setHtml("<p align = \"center\">" + pile_str + "</p>");
}

void Client::askForDiscard(const QJsonValue &reqdata){
	if(!reqdata.isArray()) return;
	QJsonArray req = reqdata.toArray();
	if (!req[0].isDouble() || !req[2].isBool() || !req[3].isBool() || !req[1].isDouble())
		return;

	discard_num = req[0].toDouble();
	is_discard_action_refusable = req[2].toBool();
	can_discard_equip = req[3].toBool();
	min_num = req[1].toDouble();

	QString prompt;
	if(can_discard_equip)
		prompt = tr("Please discard %1 card(s), include equip").arg(discard_num);
	else
		prompt = tr("Please discard %1 card(s), only hand cards is allowed").arg(discard_num);

	prompt_doc->setHtml(prompt);

	setStatus(Discarding);
}

void Client::askForExchange(const QJsonValue &exchange_str){
	if (!exchange_str.isDouble())
	{
		QMessageBox::warning(NULL, tr("Warning"), tr("Exchange string is not well formatted!"));
		return;
	}

	discard_num = exchange_str.toDouble();
	is_discard_action_refusable = false;
	can_discard_equip = false;

	prompt_doc->setHtml(tr("Please give %1 cards to exchange").arg(discard_num));

	setStatus(Discarding);
}

void Client::gameOver(const QJsonValue &argdata){
	if(!argdata.isArray()) return;
	QJsonArray arg = argdata.toArray();
	is_game_over = true;
	setStatus(Client::NotActive);
	QString winner = arg[0].toString();
	QStringList roles;
	BP::tryParse(arg[1], roles);

	Q_ASSERT(roles.length() == players.length());

	for(int i = 0; i < roles.length(); i++){
		QString name = players.at(i)->objectName();
		getPlayer(name)->setRole(roles.at(i));
	}

	if(winner == "."){
		emit standoff();
		return;
	}

	QSet<QString> winners = winner.split("+").toSet();
	foreach(const ClientPlayer *player, players){
		QString role = player->getRole();
		bool win = winners.contains(player->objectName()) || winners.contains(role);

		ClientPlayer *p = const_cast<ClientPlayer *>(player);
		p->setProperty("win", win);
	}

	emit game_over();
}

void Client::killPlayer(const QJsonValue &player_name){
	alive_count--;

	QString victim_name = player_name.toString();
	ClientPlayer *player = getPlayer(victim_name);
	if(player == Self){
		foreach(const Skill *skill, Self->getVisibleSkills())
			emit skill_detached(skill->objectName());
	}

	player->loseAllSkills();

	if(!Self->hasFlag("marshalling")){
		QString general_name = player->getGeneralName();
		QString last_word = Bang->translate(QString("~%1").arg(general_name));
		if(last_word.startsWith("~")){
			QStringList origin_generals = general_name.split("_");
			if(origin_generals.length()>1)
				last_word = Bang->translate(("~") +  origin_generals.at(1));
		}

		if(last_word.startsWith("~") && general_name.endsWith("f")){
			QString origin_general = general_name;
			origin_general.chop(1);
			if(Bang->getGeneral(origin_general))
				last_word = Bang->translate(("~") + origin_general);
		}
		skill_title = tr("%1[dead]").arg(Bang->translate(general_name));
		skill_line = last_word;

		updatePileNum();
	}

	emit player_killed(victim_name);
}

void Client::revivePlayer(const QJsonValue &player_name){
	alive_count++;

	emit player_revived(player_name.toString());
}


void Client::warn(const QJsonValue &reason){
	QString msg = reason.toString();
	if(msg == "GAME_OVER")
		msg = tr("Game is over now");
	else if(msg == "REQUIRE_PASSWORD")
		msg = tr("The server require password to signup");
	else if(msg == "WRONG_PASSWORD")
		msg = tr("Your password is wrong");
	else if(msg == "INVALID_FORMAT")
		msg = tr("Invalid signup string");
	else if(msg == "LEVEL_LIMITATION")
		msg = tr("Your level is not enough");
	else
		msg = tr("Unknown warning: %1").arg(msg);

	disconnectFromHost();
	QMessageBox::warning(NULL, tr("Warning"), msg);
}

void Client::askForGeneral(const QJsonValue &arg){
	QStringList generals;
	if (!BP::tryParse(arg, generals)) return;
	emit generals_got(generals);
	setStatus(ExecDialog);
}


void Client::askForSuit(const QJsonValue &){
	QStringList suits;
	suits << "spade" << "club" << "heart" << "diamond";
	emit suits_got(suits);
	setStatus(ExecDialog);
}

void Client::askForKingdom(const QJsonValue&){
	QStringList kingdoms = Bang->getKingdoms();
	kingdoms.removeOne("god"); // god kingdom does not really exist
	emit kingdoms_got(kingdoms);
	setStatus(ExecDialog);
}

void Client::askForChoice(const QJsonValue &ask_str_raw){
	if(!ask_str_raw.isArray()) return;
	QJsonArray ask_str = ask_str_raw.toArray();
	if (!BP::isStringArray(ask_str, 0, 1)) return;
	QString skill_name = ask_str[0].toString();
	QStringList options = ask_str[1].toString().split("+");
	emit options_got(skill_name, options);
	setStatus(ExecDialog);
}

void Client::askForCardChosen(const QJsonValue &ask_str_raw){
	if(!ask_str_raw.isArray()) return;
	QJsonArray ask_str = ask_str_raw.toArray();
	if (!BP::isStringArray(ask_str, 0, 2)) return;
	QString player_name = ask_str[0].toString();
	QString flags = ask_str[1].toString();
	QString reason = ask_str[2].toString();
	ClientPlayer *player = getPlayer(player_name);
	if (player == NULL) return;
	emit cards_got(player, flags, reason);
	setStatus(ExecDialog);
}


void Client::askForOrder(const QJsonValue &arg){
	if (!arg.isDouble()) return;
	BP::Game3v3ChooseOrderCommand reason = (BP::Game3v3ChooseOrderCommand) arg.toDouble();
	emit orders_got(reason);
	setStatus(ExecDialog);
}

void Client::askForRole3v3(const QJsonValue &argdata){
	if(!argdata.isArray()) return;
	QJsonArray arg = argdata.toArray();
	if (arg.size() != 2 || !arg[0].isString() || !arg[1].isArray()) return;
	QStringList roles;
	if (!BP::tryParse(arg[1], roles)) return;
	QString scheme = arg[0].toString();
	emit roles_got(scheme, roles);
	setStatus(ExecDialog);
}

void Client::askForDirection(const QJsonValue &){
	emit directions_got();
	setStatus(ExecDialog);
}


void Client::setMark(const QJsonValue &mark_str){
	QJsonArray texts = mark_str.toArray();
	QString who = texts.at(0).toString();
	QString mark = texts.at(1).toString();
	int value = texts.at(2).toDouble();

	ClientPlayer *player = getPlayer(who);
	player->setMark(mark, value);
}

void Client::doFilter(const QJsonValue &){
	emit do_filter();
}

void Client::onPlayerChooseSuit(){
	replyToServer(BP::AskForSuit, sender()->objectName());
	setStatus(NotActive);
}

void Client::onPlayerChooseKingdom(){
	replyToServer(BP::AskForKingdom, sender()->objectName());
	setStatus(NotActive);
}

void Client::onPlayerDiscardCards(const Card *cards){
	if(!cards){
		replyToServer(BP::AskForDiscard, QJsonValue());
	}else{
		QJsonArray val;
		foreach(int card_id, cards->getSubcards()){
			val.append(card_id);
		}
		replyToServer(BP::AskForDiscard, val);
	}

	setStatus(NotActive);
}

void Client::fillAG(const QJsonValue &cards_str){
	QJsonArray cards = cards_str.toArray();

	QList<int> card_ids;
	foreach(const QJsonValue &card, cards){
		card_ids << (int) card.toDouble();
	}

	emit ag_filled(card_ids);
}

void Client::takeAG(const QJsonValue &take_str){
	QJsonArray words = take_str.toArray();
	if(words.size() != 2){
		return;
	}

	QString taker_name = words.at(0).toString();
	int card_id = words.at(1).toDouble();

	const Card *card = Bang->getCard(card_id);
	if(taker_name != "."){
		ClientPlayer *taker = getPlayer(taker_name);
		taker->addCard(card, Player::HandArea);
		emit ag_taken(taker, card_id);
	}else{
		discarded_list.prepend(card);
		emit ag_taken(NULL, card_id);
	}
}

void Client::clearAG(const QJsonValue &){
	emit ag_cleared();
}

void Client::askForSingleWine(const QJsonValue &argdata){
	if(!argdata.isArray()) return;
	QJsonArray arg = argdata.toArray();
	if(arg.size() != 2 || !arg[0].isString() || !arg[1].isDouble()) return;

	ClientPlayer *dying = getPlayer(arg[0].toString());
	int wine_num = arg[1].toDouble();

	if(dying == Self){
		prompt_doc->setHtml(tr("You are dying, please provide %1 wine(es) to save yourself").arg(wine_num));
		card_pattern = "wine";
	}else{
		QString dying_general = Bang->translate(dying->getGeneralName());
		prompt_doc->setHtml(tr("%1 is dying, please provide %2 wine(es) to save him").arg(dying_general).arg(wine_num));
		card_pattern = "wine";
	}

	is_discard_action_refusable = true;
	m_isUseCard = false;
	setStatus(Responsing);
}

void Client::askForCardShow(const QJsonValue &requestor){
	if (!requestor.isString()) return;
	QString name = Bang->translate(requestor.toString());
	prompt_doc->setHtml(tr("%1 request you to show one hand card").arg(name));

	card_pattern = ".";
	is_discard_action_refusable = false;
	m_isUseCard = false;
	setStatus(Responsing);
}

void Client::askForAG(const QJsonValue &arg){
	if (!arg.isBool()) return;
	is_discard_action_refusable = arg.toBool();
	setStatus(AskForAG);
}

void Client::onPlayerChooseAG(int card_id){
	replyToServer(BP::AskForAG, card_id);
	setStatus(NotActive);
}

QList<const ClientPlayer*> Client::getPlayers() const{
	return players;
}

void Client::clearTurnTag(){
	switch(Self->getPhase()){
	case Player::Start:{
			Bang->playAudio("your-turn");
			QApplication::alert(QApplication::focusWidget());
			break;
	}

	case Player::Play:{
			Self->clearHistory();
			break;
		}

	case Player::NotActive:{
			Self->clearFlags();
			break;
		}

	default:
		break;
	}
}

void Client::showCard(const QJsonValue &data){
	if(!data.isArray()){
		return;
	}

	QJsonArray show_str = data.toArray();
	if(show_str.size() != 2 || !show_str[0].isString() || !show_str[1].isDouble())
		return;

	QString player_name = show_str[0].toString();
	int card_id = show_str[1].toDouble();

	ClientPlayer *player = getPlayer(player_name);
	if(player != Self)
		player->addKnownHandCard(Bang->getCard(card_id));

	emit card_shown(player_name, card_id);
}

void Client::attachSkill(const QJsonValue &skill_name){
	QString name = skill_name.toString();
	Self->acquireSkill(name);
	emit skill_attached(name, true);
}

void Client::detachSkill(const QJsonValue &detach_str){
	QJsonArray texts = detach_str.toArray();
	ClientPlayer *player = NULL;
	QString skill_name;
	if(texts.size() == 1){
		player = Self;
		skill_name = texts.at(0).toString();
	}else if(texts.size() == 2){
		player = getPlayer(texts.at(0).toString());
		skill_name = texts.at(1).toString();
	}

	player->loseSkill(skill_name);

	if(player == Self)
		emit skill_detached(skill_name);
}

void Client::askForAssign(const QJsonValue &){
	emit assign_asked();
}

void Client::onPlayerAssignRole(const QList<QString> &names, const QList<QString> &roles)
{
	Q_ASSERT(names.size() == roles.size());
	QJsonArray reply;
	reply.append(BP::toJsonArray(names));
	reply.append(BP::toJsonArray(roles));
	replyToServer(BP::AskForAssign, reply);
}

void Client::askForGuanxing(const QJsonValue &argdata){
	QJsonArray arg = argdata.toArray();
	QJsonValue deck = arg[0];
	bool up_only = arg[1].toBool();
	QList<int> card_ids;
	BP::tryParse(deck, card_ids);
	
	emit guanxing(card_ids, up_only);
	setStatus(AskForGuanxing);
}

void Client::askForGongxin(const QJsonValue &data){
	if(!data.isArray()){
		return;
	}

	QJsonArray arg = data.toArray();
	if(arg.size() != 3 || !arg[0].isString() || ! arg[1].isBool()){
		return;
	}
	ClientPlayer *who = getPlayer(arg[0].toString());
	bool enable_heart = arg[1].toBool();
	QList<int> card_ids;
	if (!BP::tryParse(arg[2], card_ids)) return;

	who->setCards(card_ids);

	emit gongxin(card_ids, enable_heart);
	setStatus(AskForGongxin);
}

void Client::onPlayerReplyGongxin(int card_id){
	QJsonValue reply;
	if(card_id != -1)
		reply = card_id;    
	replyToServer(BP::AskForGongxin, reply);
	setStatus(NotActive);
}

void Client::askForPindian(const QJsonValue &data){
	if(!data.isArray()){
		return;
	}

	QJsonArray ask_str = data.toArray();
	if (!BP::isStringArray(ask_str, 0, 1)){
		return;
	}
	QString from = ask_str[0].toString();
	if(from == Self->objectName())
		prompt_doc->setHtml(tr("Please play a card for pindian"));
	else{
		QString requestor = getPlayerName(from);
		prompt_doc->setHtml(tr("%1 ask for you to play a card to pindian").arg(requestor));
	}
	m_isUseCard = false;
	card_pattern = ".";
	is_discard_action_refusable = false;
	setStatus(Responsing);
}

void Client::askForYiji(const QJsonValue &card_list_data){
	if (!card_list_data.isArray()) return;
	QJsonArray card_list = card_list_data.toArray();
	int count = card_list.size();
	prompt_doc->setHtml(tr("Please distribute %1 cards as you wish").arg(count));
	//@todo: use cards directly rather than the QString
	QStringList card_str;
	for (unsigned int i = 0; i < card_list.size(); i++)
		card_str << QString::number((int) card_list[i].toDouble());
	card_pattern = card_str.join("+");
	setStatus(AskForYiji);
}

void Client::askForPlayerChosen(const QJsonValue &playerdata){
	if(!playerdata.isArray()) return;
	QJsonArray players = playerdata.toArray();
	if (players.size() != 2) return;
	if (!players[1].isString() || !players[0].isArray()) return;
	QJsonArray players0 = players[0].toArray();
	if (players0.size() == 0) return;
	skill_name = players[1].toString();
	players_to_choose.clear();
	for (unsigned int i = 0; i < players0.size(); i++)
		players_to_choose.push_back(players0[i].toString());

	setStatus(AskForPlayerChoose);
}

void Client::onPlayerReplyYiji(const Card *card, const Player *to){
	QJsonValue req;
	if(card){
		QJsonArray arr;
		arr.append(BP::toJsonArray(card->getSubcards()));
		arr.append(to->objectName());
		req = arr;
	}
	replyToServer(BP::AskForYiji, req);

	setStatus(NotActive);
}

void Client::onPlayerReplyGuanxing(const QList<int> &up_cards, const QList<int> &down_cards){
	QJsonArray decks;
	decks.append(BP::toJsonArray(up_cards));
	decks.append(BP::toJsonArray(down_cards));

	replyToServer(BP::AskForGuanxing, decks);
	//request(QString("replyGuanxing %1:%2").arg(up_items.join("+")).arg(down_items.join("+")));

	setStatus(NotActive);
}

void Client::log(const QJsonValue &log_str){
	emit log_received(log_str.toString());
}

void Client::speak(const QJsonValue &speak_data){
	QJsonArray words = speak_data.toArray();
	if(words.size() != 2){
		return;
	}

	QString who = words.at(0).toString();
	QString text = words.at(1).toString();
	emit text_spoken(text);

	if(who == "."){
		QString line = tr("<font color='red'>System: %1</font>").arg(text);
		emit line_spoken(QString("<p style=\"margin:3px 2px;\">%1</p>").arg(line));
		return;
	}

	const ClientPlayer *from = getPlayer(who);

	QString title;
	if(from){
		title = from->getGeneralName();
		title = Bang->translate(title);
		title.append(QString("(%1)").arg(from->screenName()));
	}

	title = QString("<b>%1</b>").arg(title);

	QString line = tr("<font color='%1'>[%2] said: %3 </font>")
				   .arg(Config.TextEditColor.name()).arg(title).arg(text);

	emit line_spoken(QString("<p style=\"margin:3px 2px;\">%1</p>").arg(line));
}

void Client::moveFocus(const QJsonValue &focus){
	QString who;
	BP::Countdown countdown;
	if (focus.isString())
	{
		who = focus.toString();
		countdown.type = BP::Countdown::Unlimited;
	}
	else
	{
		Q_ASSERT(focus.isArray());
		QJsonArray focusarr = focus.toArray();
		Q_ASSERT(focusarr.size() == 2);
		who = focusarr[0].toString();
	
		bool success = countdown.tryParse(focusarr[1]);
		if (!success){
			Q_ASSERT(focusarr[1].isDouble());
			BP::CommandType command = (BP::CommandType) focusarr[1].toDouble();
			countdown.max = ServerInfo.getCommandTimeout(command, BP::ClientInstance);
			countdown.type = BP::Countdown::UseDefault;
		}
	}
	emit focus_moved(who, countdown);
}

void Client::setEmotion(const QJsonValue &set_str){
	QJsonArray words = set_str.toArray();
	QString target_name = words.at(0).toString();
	QString emotion = words.at(1).toString();

	emit emotion_set(target_name, emotion);
}

void Client::skillInvoked(const QJsonValue &argdata){
	if(!argdata.isArray()) return;
	QJsonArray arg = argdata.toArray();
	if (!BP::isStringArray(arg,0,1)) return;
	emit skill_invoked(arg[1].toString(), arg[0].toString());
}

void Client::acquireSkill(const QJsonValue &acquire_arr){
	QJsonArray texts = acquire_arr.toArray();
	ClientPlayer *who = getPlayer(texts.at(0).toString());
	QString skill_name = texts.at(1).toString();

	who->acquireSkill(skill_name);

	emit skill_acquired(who, skill_name);
}

void Client::animate(const QJsonValue &animate_str){
	QJsonArray args = animate_str.toArray();

	QStringList arglist;
	for(int i = 1; i < args.size(); i++){
		arglist << args.at(i).toString();
	}

	QString name = args.at(0).toString();

	emit animated(name, arglist);
}

void Client::setScreenName(const QJsonValue &set_str){
	QJsonArray words = set_str.toArray();
	ClientPlayer *player = getPlayer(words.at(0).toString());
	QString screen_name = words.at(1).toString();
	player->setScreenName(screen_name);
}

void Client::setFixedDistance(const QJsonValue &set_str){
	QJsonArray texts = set_str.toArray();
	ClientPlayer *from = getPlayer(texts.at(0).toString());
	ClientPlayer *to = getPlayer(texts.at(1).toString());
	int distance = texts.at(2).toDouble();

	if(from && to)
		from->setFixedDistance(to, distance);
}

void Client::pile(const QJsonValue &pile_str){
	QJsonArray texts = pile_str.toArray();
	if(texts.size() != 4){
		return;
	}

	ClientPlayer *player = getPlayer(texts.at(0).toString());
	QString name = texts.at(1).toString();
	bool add = texts.at(2).toString() == "+";
	int card_id = texts.at(3).toDouble();

	QList<int> card_ids;
	card_ids.append(card_id);

	if(player)
		player->changePile(name, add, card_ids);
}

void Client::transfigure(const QJsonValue &transfigure_tr){
	QJsonArray generals = transfigure_tr.toArray();

	if(generals.size() >= 2){
		const General *from = Bang->getGeneral(generals.at(0).toString());
		const General *to = Bang->getGeneral(generals.at(1).toString());

		if(from) foreach(const Skill *skill, from->getVisibleSkills()){
			emit skill_detached(skill->objectName());
		}

		if(to) foreach(const Skill *skill, to->getVisibleSkills()){
			emit skill_attached(skill->objectName(), false);
		}
	}
}

void Client::fillGenerals(const QJsonValue &generals){
	QStringList generallist;
	foreach(const QJsonValue &general, generals.toArray()){
		generallist << general.toString();
	}

	emit generals_filled(generallist);
}

void Client::askForGeneral3v3(const QJsonValue &){
	emit general_asked();
}

void Client::takeGeneral(const QJsonValue &take_str){
	QJsonArray texts = take_str.toArray();
	if(texts.size() != 2){
		return;
	}

	QString who = texts.at(0).toString();
	QString name = texts.at(1).toString();

	emit general_taken(who, name);
}

void Client::startArrange(const QJsonValue &){
	emit arrange_started();
}

void Client::onPlayerChooseRole3v3(){
	replyToServer(BP::AskForRole3v3, sender()->objectName());
	setStatus(NotActive);
}

void Client::recoverGeneral(const QJsonValue &recover_str){
	QJsonArray texts = recover_str.toArray();
	if(texts.size() != 2){
		return;
	}

	int index = texts.at(0).toDouble();
	QString name = texts.at(1).toString();

	emit general_recovered(index, name);
}

void Client::revealGeneral(const QJsonValue &reveal_str){
	QJsonArray texts = reveal_str.toArray();
	if(texts.size() != 2){
		return;
	}

	bool self = texts.at(0).toString() == Self->objectName();
	QString general = texts.at(1).toString();

	emit general_revealed(self, general);
}

void Client::onPlayerChooseOrder(){
	OptionButton *button = qobject_cast<OptionButton *>(sender());
	QString order;
	if(button){
		order = button->objectName();
	}else{
		if(qrand() % 2 == 0)
			order = "warm";
		else
			order = "cool";
	}
	int req;
	if (order == "warm") req = (int) BP::WarmCamp;
	else req = (int) BP::CoolCamp;
	replyToServer(BP::AskForOrder, req);
	setStatus(NotActive);
}

void Client::updateStateItem(const QJsonValue &state_str){
	emit role_state_changed(state_str.toString());
}
