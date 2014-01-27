#include "serverplayer.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "ai.h"
#include "settings.h"
#include "recorder.h"
#include "banpair.h"
#include "lua-wrapper.h"

const int ServerPlayer::S_NUM_SEMAPHORES = 6;

ServerPlayer::ServerPlayer(Room *room)
	: Player(room), m_isClientResponseReady(false), m_isWaitingReply(false),
	socket(NULL), room(room),
	ai(NULL), trust_ai(new TrustAI(this)), recorder(NULL), next(NULL), _m_clientResponse()
{
	semas = new QSemaphore*[S_NUM_SEMAPHORES];
	for(int i=0; i< S_NUM_SEMAPHORES; i++){
		semas[i] = new QSemaphore(0);
	}
}

void ServerPlayer::drawCard(const Card *card){
	handcards << card;
}

Room *ServerPlayer::getRoom() const{
	return room;
}

void ServerPlayer::playCardEffect(const QString &card_name) const{
	QJsonArray arg;
	arg.append(card_name);
	arg.append(getGender() == General::Male);
	room->doBroadcastNotify(BP::PlayCardEffect, arg);
}

void ServerPlayer::playCardEffect(const Card *card) const{
	if(card->isMute())
		return;

	if(!card->isVirtualCard())
		playCardEffect(card->objectName());

	QString skill_name = card->getSkillName();
	const Skill *skill = Bang->getSkill(skill_name);
	if(skill == NULL)
		return;

	int index = skill->getEffectIndex(this, card);
	if(index == 0)
		return;

	if(index == -1 && skill->getSources().isEmpty())
		playCardEffect(card->objectName());
	else
		room->playSkillEffect(skill_name, index);
}

int ServerPlayer::getRandomHandCardId() const{
	return getRandomHandCard()->getEffectiveId();
}

const Card *ServerPlayer::getRandomHandCard() const{
	int index = qrand() % handcards.length();
	return handcards.at(index);
}

void ServerPlayer::obtainCard(const Card *card, bool unhide){
	room->obtainCard(this, card, unhide);
}

void ServerPlayer::throwAllEquips(){
	QList<const Card *> equips = getEquips();

	if(equips.isEmpty())
		return;

	DummyCard *card = new DummyCard;
	foreach(const Card *equip, equips)
		card->addSubcard(equip);
	room->throwCard(card, this);
	card->deleteLater();
}

void ServerPlayer::throwAllHandCards(){
	DummyCard *card = wholeHandCards();
	if(card == NULL)
		return;

	room->throwCard(card, this);
	card->deleteLater();
}

void ServerPlayer::throwAllMarks(){
	// throw all marks
	foreach(QString mark_name, marks.keys()){
		if(!mark_name.startsWith("@"))
			continue;

		int n = marks.value(mark_name, 0);
		if(n != 0){
			room->setPlayerMark(this, mark_name, 0);
		}
	}

	marks.clear();
}

void ServerPlayer::clearPrivatePiles(){
	// throw private piles
	foreach(QString pile_name, piles.keys()){
		QList<int> &pile = piles[pile_name];

		foreach(int card_id, pile){
			room->throwCard(card_id);
			QJsonArray arg;
			arg.append(objectName());
			arg.append(pile_name);
			arg.append(card_id);
			room->doBroadcastNotify(BP::Pile, arg);
		}
	}
	piles.clear();
}

void ServerPlayer::bury(){
	clearFlags();
	clearHistory();
	throwAllCards();
	throwAllMarks();
	clearPrivatePiles();

	room->clearPlayerCardLock(this);
}

void ServerPlayer::throwAllCards(){
	throwAllEquips();
	throwAllHandCards();

	QList<const Card *> tricks = getJudgingArea();
	foreach(const Card *trick, tricks)
		room->throwCard(trick);
}

void ServerPlayer::drawCards(int n, bool set_emotion, const QString &reason){
	room->drawCards(this, n, reason);

	if(set_emotion)
		room->setEmotion(this, "draw-card");
}

// a convenient way to ask player
bool ServerPlayer::askForSkillInvoke(const QString &skill_name, const QVariant &data){
	return room->askForSkillInvoke(this, skill_name, data);
}

QList<int> ServerPlayer::forceToDiscard(int discard_num, bool include_equip){
	QList<int> to_discard;

	QString flags = "h";
	if(include_equip)
		flags.append("e");

	QList<const Card *> all_cards = getCards(flags);
	qShuffle(all_cards);

	int i;
	for(i=0; i<discard_num; i++)
		to_discard << all_cards.at(i)->getId();

	return to_discard;
}

int ServerPlayer::aliveCount() const{
	return room->alivePlayerCount();
}

int ServerPlayer::getHandcardNum() const{
	return handcards.length();
}

void ServerPlayer::setSocket(ClientSocket *socket){
	if(socket){
		connect(socket, SIGNAL(disconnected()), this, SIGNAL(disconnected()));
		connect(socket, SIGNAL(message_got(char*)), this, SLOT(getMessage(char*)));

		connect(this, SIGNAL(message_cast(QString)), this, SLOT(castMessage(QString)));
	}else{
		if(this->socket){
			this->disconnect(this->socket);
			this->socket->disconnect(this);
			this->socket->disconnectFromHost();
			this->socket->deleteLater();
		}

		disconnect(this, SLOT(castMessage(QString)));
	}

	this->socket = socket;
}

void ServerPlayer::getMessage(char *message){
	QString request = message;
	if(request.endsWith("\n"))
		request.chop(1);

	emit request_got(request);
}

void ServerPlayer::unicast(const QString &message) const{
	emit message_cast(message);

	if(recorder)
		recorder->recordLine(message);
}

void ServerPlayer::startNetworkDelayTest(){
	test_time = QDateTime::currentDateTime();
	notify(BP::NetworkDelayTest);
}

qint64 ServerPlayer::endNetworkDelayTest(){
	return test_time.msecsTo(QDateTime::currentDateTime());
}

void ServerPlayer::startRecord(){
	recorder = new Recorder(this);
}

void ServerPlayer::saveRecord(const QString &filename){
	if(recorder)
		recorder->save(filename);
}

void ServerPlayer::addToSelected(const QString &general){
	selected.append(general);
}

QStringList ServerPlayer::getSelected() const{
	return selected;
}

QString ServerPlayer::findReasonable(const QStringList &generals, bool no_unreasonable){

	foreach(QString name, generals){
		if(Config.Enable2ndGeneral){
			if(getGeneral()){
				if(BanPair::isBanned(getGeneralName(), name))
					continue;
			}else{
				if(BanPair::isBanned(name))
					continue;
			}

			if(Config.EnableHegemony){
				if(getGeneral())
					if((getGeneral()->getKingdom()
							!= Bang->getGeneral(name)->getKingdom()))
						continue;
			}
		}
		if(Config.EnableBasara){
			QStringList ban_list = Config.value("Banlist/Basara").toStringList();

			if(ban_list.contains(name))continue;
		}
		if(Config.GameMode == "zombie_mode"){
			QStringList ban_list = Config.value("Banlist/Zombie").toStringList();

			if(ban_list.contains(name))continue;
		}
		if((Config.GameMode.endsWith("p") ||
			Config.GameMode.endsWith("pd")))
		{
			QStringList ban_list = Config.value("Banlist/Roles").toStringList();

			if(ban_list.contains(name))continue;
		}

		return name;
	}


	if(no_unreasonable)
		return NULL;

	return generals.first();
}

void ServerPlayer::clearSelected(){
	selected.clear();
}

void ServerPlayer::castMessage(const QString &message){
	if(socket){
		socket->send(message.toUtf8());

#ifndef QT_NO_DEBUG
		qDebug("%s: %s", qPrintable(objectName()), qPrintable(message));
#endif
	}
}

void ServerPlayer::notify(BP::CommandType command, const QJsonValue &arg) const{
	BP::Packet packet(BP::ServerNotification, command);
	packet.setMessageBody(arg);
	invoke(packet);
}

QString ServerPlayer::reportHeader() const{
	QString name = objectName();
	return QString("%1 ").arg(name.isEmpty() ? tr("Anonymous") : name);
}

void ServerPlayer::sendProperty(const char *property_name, const Player *player) const{
	if(player == NULL)
		player = this;

	QJsonArray arg;
	if(player == this){
		arg.append(true);
	}else{
		arg.append(false);
		arg.append(player->objectName());
	}
	arg.append(QString(property_name));
	arg.append(player->property(property_name).toString());
	notify(BP::SetPlayerProperty, arg);
}

void ServerPlayer::removeCard(const Card *card, Place place){
	switch(place){
	case HandArea: {
			handcards.removeOne(card);
			break;
		}
	case HandlingArea: {
			m_takenOffCards.removeOne(card);
			break;
		}

	case EquipArea: {
			const EquipCard *equip = qobject_cast<const EquipCard *>(card);
			removeEquip(equip);

			LogMessage log;
			log.type = "$Uninstall";
			log.card_str = card->toString();
			log.from = this;
			room->sendLog(log);

			equip->onUninstall(this);
			break;
		}

	case JudgingArea:{
			removeDelayedTrick(card);
			break;
		}

	case SpecialArea:{
			int card_id = card->getEffectiveId();
			QString pile_name = getPileName(card_id);
			
			//@todo: sanity check required
			if(!pile_name.isEmpty())
				piles[pile_name].removeOne(card_id);

			break;
		}

	default:
		// FIXME
		;
	}
}

void ServerPlayer::addCard(const Card *card, Place place){
	switch(place){
	case HandArea: {
			handcards << card;
			break;
		}
	case HandlingArea: {
			m_takenOffCards << card;
			break;
		}
	case EquipArea: {
			const EquipCard *equip = qobject_cast<const EquipCard *>(card);
			setEquip(equip);
			equip->onInstall(this);
			break;
		}

	case JudgingArea:{
			addDelayedTrick(card);
			break;
		}

	default:
		// FIXME
		;
	}
}

bool ServerPlayer::isLastHandCard(const Card *card) const{
	if(handcards.length() != 1)
		return false;

	return card->getEffectiveId() == handcards.first()->getEffectiveId();
}

QList<int> ServerPlayer::handCards() const{
	QList<int> card_ids;
	foreach(const Card *card, handcards)
		card_ids << card->getId();

	return card_ids;
}

QList<const Card *> ServerPlayer::getHandcards() const{
	return handcards;
}

QList<const Card *> ServerPlayer::getCards(const QString &flags) const{
	QList<const Card *> cards;
	if(flags.contains("h"))
		cards << handcards;

	if(flags.contains("e"))
		cards << getEquips();

	if(flags.contains("j"))
		cards << getJudgingArea();

	return cards;
}

DummyCard *ServerPlayer::wholeHandCards() const{
	if(isKongcheng())
		return NULL;

	DummyCard *dummy_card = new DummyCard;
	foreach(const Card *card, handcards)
		dummy_card->addSubcard(card->getId());

	return dummy_card;
}

bool ServerPlayer::hasNullification() const{
	if(hasSkill("kanpo")){
		foreach(const Card *card, handcards){
			if(card->isBlack() || card->objectName() == "nullification")
				return true;
		}
	}
	if(hasSkill("wushen")){
		foreach(const Card *card, handcards){
			if(card->objectName() == "nullification" && card->getSuit() != Card::Heart)
				return true;
		}
	}
	if(hasSkill("guhuo")){
		return !isKongcheng();
	}
	if(hasFlag("lexue")){
		int card_id = getMark("lexue");
		const Card *card = Bang->getCard(card_id);
		if(card->objectName() == "nullification"){
			foreach(const Card *c, handcards + getEquips()){
				if(c->objectName() == "nullification" || c->getSuit() == card->getSuit())
					return true;
			}
		}
	}
	if(hasSkill("longhun")){
		int n = qMax(1, getHp());
		int count = 0;
		foreach(const Card *card, handcards + getEquips()){
			if(card->objectName() == "nullification")
				return true;

			if(card->getSuit() == Card::Spade)
				count ++;
		}

		return count >= n;
	}
	if(hasSkill("yanzheng")){
		foreach(const Card *card, handcards){
			if(card->objectName() == "nullification")
				return true;
		}

		return getHandcardNum() > getHp() && !getEquips().isEmpty();
	}

	foreach(const Card *card, handcards){
		if(card->objectName() == "nullification")
			return true;
	}

	foreach(const Skill* skill, getVisibleSkillList()){
		if(skill->inherits("LuaViewAsSkill")){
			const LuaViewAsSkill* luaskill = qobject_cast<const LuaViewAsSkill*>(skill);
			if(luaskill->isEnabledAtNullification(this)) return true;
		}else if(skill->inherits("TriggerSkill")){
			const TriggerSkill* trigger_skill = qobject_cast<const TriggerSkill*>(skill);
			if(trigger_skill && trigger_skill->getViewAsSkill()
					&& trigger_skill->getViewAsSkill()->inherits("LuaViewAsSkill")){
				const LuaViewAsSkill* luaskill = qobject_cast<const LuaViewAsSkill*>(trigger_skill->getViewAsSkill());
				if(luaskill && luaskill->isEnabledAtNullification(this)) return true;
			}
		}
	}

	return false;
}

void ServerPlayer::kick(){
	if(socket){
		socket->disconnectFromHost();
	}
}

bool ServerPlayer::pindian(ServerPlayer *target, const QString &reason, const Card *card1){
	LogMessage log;
	log.type = "#Pindian";
	log.from = this;
	log.to << target;
	room->sendLog(log);

	if(card1 == NULL)
		card1 = room->askForPindian(this, this, target, reason);
	else if(card1->isVirtualCard()){
		int card_id = card1->getEffectiveId();
		card1 = Bang->getCard(card_id);
	}

	const Card *card2 = room->askForPindian(target, this, target, reason);

	PindianStruct pindian_struct;
	pindian_struct.from = this;
	pindian_struct.to = target;
	pindian_struct.from_card = card1;
	pindian_struct.to_card = card2;
	pindian_struct.reason = reason;

	PindianStar pindian_star = &pindian_struct;
	QVariant data = QVariant::fromValue(pindian_star);
	room->getThread()->trigger(Pindian, this, data);

	bool success = pindian_star->from_card->getNumber() > pindian_star->to_card->getNumber();
	log.type = success ? "#PindianSuccess" : "#PindianFailure";
	log.from = this;
	log.to.clear();
	log.to << target;
	log.card_str.clear();
	room->sendLog(log);

	if(success)
		room->setEmotion(this, "success");
	else
		room->setEmotion(this, "no-success");

	return success;
}

void ServerPlayer::turnOver(){
	setFaceUp(!faceUp());
	room->broadcastProperty(this, "faceup");

	LogMessage log;
	log.type = "#TurnOver";
	log.from = this;
	log.arg = faceUp() ? "face_up" : "face_down";
	room->sendLog(log);

	room->getThread()->trigger(TurnedOver, this);
}

void ServerPlayer::play(QList<Player::Phase> set_phases){
	if(!set_phases.isEmpty()){
		if(!set_phases.contains(NotActive))
			set_phases << NotActive;
	}
	else
		set_phases << RoundStart << Start << Judge << Draw << Play
				<< Discard << Finish << NotActive;

	phases = set_phases;
	while(!phases.isEmpty()){
		PhaseChangeStruct phase_change;

		Phase phase = phases.takeFirst();
		phase_change.from = this->getPhase();
		phase_change.to = phase;

		setPhase(phase);
		room->broadcastProperty(this, "phase");

		QVariant data = QVariant::fromValue(phase_change);
		room->getThread()->trigger(PhaseChange, this, data);

		if(isDead() && phase != NotActive){
			phases.clear();
			phases << NotActive;
		}
	}
}

QList<Player::Phase> &ServerPlayer::getPhases(){
	return phases;
}

void ServerPlayer::skip(Player::Phase phase){
	phases.removeOne(phase);

	static QStringList phase_strings;
	if(phase_strings.isEmpty()){
		phase_strings << "round_start" << "start" << "judge" << "draw"
				<< "play" << "discard" << "finish" << "not_active";
	}

	int index = static_cast<int>(phase);

	LogMessage log;
	log.type = "#SkipPhase";
	log.from = this;
	log.arg = phase_strings.at(index);
	room->sendLog(log);
}

void ServerPlayer::skip(){
	phases.clear();

	LogMessage log;
	log.type = "#SkipAllPhase";
	log.from = this;
	room->sendLog(log);
}

void ServerPlayer::gainMark(const QString &mark, int n){
	int value = getMark(mark) + n;

	if(mark.at(1) == '#'){
		LogMessage log;
		log.type = "#GetMark";
		log.from = this;
		log.arg = mark;
		log.arg2 = QString::number(n);

		room->sendLog(log);
	}

	room->setPlayerMark(this, mark, value);
}

void ServerPlayer::loseMark(const QString &mark, int n){
	int value = getMark(mark) - n;

	if(mark.at(1) != '#'){
		LogMessage log;
		log.type = "#LoseMark";
		log.from = this;
		log.arg = mark;
		log.arg2 = QString::number(n);

		room->sendLog(log);
	}

	room->setPlayerMark(this, mark, value);
}

void ServerPlayer::loseAllMarks(const QString &mark_name){
	int n = getMark(mark_name);
	if(n > 0){
		loseMark(mark_name, n);
	}
}

bool ServerPlayer::isOnline() const {
	return getState() == "online";
}

void ServerPlayer::setAI(AI *ai) {
	this->ai = ai;
}

AI *ServerPlayer::getAI() const{
	if(getState() == "online"){
		return NULL;
	}
	else if(getState() == "trust" && !Config.FreeChoose)
		return trust_ai;
	else
		return ai;
}

AI *ServerPlayer::getSmartAI() const{
	return ai;
}

void ServerPlayer::addVictim(ServerPlayer *victim){
	victims.append(victim);
}

QList<ServerPlayer *> ServerPlayer::getVictims() const{
	return victims;
}

void ServerPlayer::setNext(ServerPlayer *next){
	this->next = next;
}

ServerPlayer *ServerPlayer::getNext() const{
	return next;
}

ServerPlayer *ServerPlayer::getNextAlive() const{
	ServerPlayer *next = this->next;
	while(next->isDead())
		next = next->getNext();

	return next;
}

int ServerPlayer::getGeneralMaxHp() const{
	int max_hp = 0;

	if(getGeneral2() == NULL){
		max_hp = getGeneral() != NULL ? getGeneral()->getMaxHp() : 4;
	}else{
		int first = getGeneral()->getMaxHp();
		int second = getGeneral2()->getMaxHp();

		int plan = Config.MaxHpScheme;
		if(Config.GameMode.contains("_mini_"))plan = 1;

		switch(plan){
		case 2: max_hp = (first + second)/2; break;
		case 1: max_hp = qMin(first, second); break;
		case 0:
		default:
			max_hp = first + second - 3; break;
		}

		max_hp = qMin(max_hp, 8);
	}

	if(room->hasWelfare(this))
		max_hp++;

	return max_hp;
}

QString ServerPlayer::getGameMode() const{
	return room->getMode();
}

QString ServerPlayer::getIp() const{
	if(socket)
		return socket->peerAddress();
	else
		return QString();
}

void ServerPlayer::introduceTo(ServerPlayer *player){
	QString screen_name = Config.ContestMode ? tr("Contestant") : screenName();
	QString avatar = property("avatar").toString();

	QJsonArray intro;
	intro.append(objectName());
	intro.append(screen_name);
	intro.append(avatar);

	BP::Packet intro_packet(BP::ServerNotification, BP::AddPlayer);
	intro_packet.setMessageBody(intro);

	if(player){
		player->invoke(intro_packet);
	}else{
		room->broadcastInvoke(intro_packet, this);
	}

	if(isReady())
		room->broadcastProperty(this, "ready");
}

void ServerPlayer::marshal(ServerPlayer *player) const{
	player->sendProperty("maxhp", this);
	player->sendProperty("hp", this);

	if(getKingdom() != getGeneral()->getKingdom())
		player->sendProperty("kingdom", this);

	if(isAlive()){
		player->sendProperty("seat", this);
		if(getPhase() != Player::NotActive)
			player->sendProperty("phase", this);
	}else{
		player->sendProperty("alive", this);
		player->sendProperty("role", this);
		player->notify(BP::KillPlayer, objectName());
	}

	if(!faceUp())
		player->sendProperty("faceup", this);

	if(isChained())
		player->sendProperty("chained", this);

	if(!isKongcheng()){
		if(player != this){
			player->invoke("drawNCards",
						   QString("%1:%2")
						   .arg(objectName())
						   .arg(getHandcardNum()));
		}else{
			QStringList card_str;
			foreach(const Card *card, handcards){
				card_str << QString::number(card->getId());
			}

			player->invoke("drawCards", card_str.join("+"));
		}
	}


	foreach(const Card *equip, getEquips()){
		player->invoke("moveCard",
					   QString("%1:_@=->%2@equip")
					   .arg(equip->getId())
					   .arg(objectName()));
	}

	foreach(const Card *card, getJudgingArea()){
		player->invoke("moveCard",
					   QString("%1:_@=->%2@judging")
					   .arg(card->getId())
					   .arg(objectName()));
	}

	foreach(QString mark_name, marks.keys()){
		if(mark_name.startsWith("@")){
			int value = getMark(mark_name);
			if(value != 0){
				QJsonArray mark_arg;
				mark_arg.append(objectName());
				mark_arg.append(mark_name);
				mark_arg.append(value);

				player->notify(BP::SetMark, mark_arg);
			}
		}
	}

	foreach(QString skill_name, acquired_skills){
		QJsonArray arg;
		arg.append(objectName());
		arg.append(skill_name);
		player->notify(BP::AcquireSkill, arg);
	}

	QJsonArray arg;
	arg.append(false);
	arg.append(objectName());
	arg.append(QString("flags"));
	arg.append(QJsonValue());
	foreach(QString flag, flags){
		arg[3] = flag;
		player->notify(BP::SetPlayerProperty, arg);
	}

	foreach(QString item, history.keys()){
		int value = history.value(item);
		if(value > 0){
			player->notify(BP::AddHistory, QString("%1#%2").arg(item).arg(value));
		}
	}
}

void ServerPlayer::addToPile(const QString &pile_name, const Card *card, bool open){
	if(card->isVirtualCard()){
		QList<int> cards_id = card->getSubcards();
		foreach(int card_id, cards_id)
			piles[pile_name] << card_id;
	}
	else
		piles[pile_name] << card->getEffectiveId();

	room->moveCardTo(card, this, Player::SpecialArea, open);
}

void ServerPlayer::addToPile(const QString &pile_name, int card_id, bool open){
	piles[pile_name] << card_id;

	room->moveCardTo(Bang->getCard(card_id), this, Player::SpecialArea, open);
}

void ServerPlayer::addToPile(const QString &pile_name, QList<int> card_ids, bool open){
	piles[pile_name].append(card_ids);
	CardsMoveStruct move;
	move.card_ids = card_ids;
	move.to = this;
	move.to_place = Player::SpecialArea;
	room->moveCards(move, open, false);
}

void ServerPlayer::gainAnExtraTurn(ServerPlayer *clearflag){
	ServerPlayer *current = room->getCurrent();

	room->setCurrent(this);
	room->removeTag("Zhichi");
	if(clearflag)
		clearflag->clearFlags();
	room->getThread()->trigger(TurnStart, this);
	if(clearflag)
		clearflag->clearHistory();
	room->setCurrent(current);
}

void ServerPlayer::copyFrom(ServerPlayer* sp)
{
	ServerPlayer *b = this;
	ServerPlayer *a = sp;

	b->handcards    = QList<const Card *> (a->handcards);
	b->phases       = QList<ServerPlayer::Phase> (a->phases);
	b->selected     = QStringList (a->selected);

	Player* c = b;
	c->copyFrom(a);
}
