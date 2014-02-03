#include "roomdriver.h"
#include "room.h"
#include "engine.h"
#include "server.h"
#include "scenario.h"
#include "ai.h"
#include "settings.h"
#include "gamerule.h"

#include <QTime>

LogMessage::LogMessage()
	:from(NULL)
{
}

QString LogMessage::toString() const{
	QStringList tos;
	foreach(ServerPlayer *player, to)
		if(player != NULL) tos << player->objectName();

	return QString("%1:%2->%3:%4:%5:%6")
			.arg(type)
			.arg(from ? from->objectName() : "")
			.arg(tos.join("+"))
			.arg(card_str).arg(arg).arg(arg2);
}

DamageStruct::DamageStruct()
	:from(NULL), to(NULL), card(NULL), damage(1), nature(Normal), chain(false)
{
}

CardEffectStruct::CardEffectStruct()
	:card(NULL), from(NULL), to(NULL), multiple(false)
{
}

SlashEffectStruct::SlashEffectStruct()
	:slash(NULL), jink(NULL), from(NULL), to(NULL), drank(false), nature(DamageStruct::Normal)
{
}

DyingStruct::DyingStruct()
	:who(NULL), damage(NULL)
{
}

RecoverStruct::RecoverStruct()
	:recover(1), from(NULL), card(NULL)
{

}

PindianStruct::PindianStruct()
	:from(NULL), to(NULL), from_card(NULL), to_card(NULL)
{

}

bool PindianStruct::isSuccess() const{
	return from_card->getNumber() > to_card->getNumber();
}

JudgeStructPattern::JudgeStructPattern(){
}

bool JudgeStructPattern::match(const Player *player, const Card *card) const{
	if(pattern.isEmpty())
		return false;

	if(is_regex){
		QString class_name = card->metaObject()->className();
		Card::Suit suit = card->getSuit();
		if(player->hasSkill("hongyan") && suit == Card::Spade)
			suit = Card::Heart;

		QString number = card->getNumberString();
		QString card_str = QString("%1:%2:%3").arg(class_name).arg(Card::Suit2String(suit)).arg(number);

		return QRegExp(pattern).exactMatch(card_str);
	}else{
		const CardPattern *card_pattern = Bang->getPattern(pattern);
		return card_pattern && card_pattern->match(player, card);
	}
}

JudgeStructPattern &JudgeStructPattern::operator =(const QRegExp &rx){
	pattern = rx.pattern();
	is_regex = true;

	return *this;
}

JudgeStructPattern &JudgeStructPattern::operator =(const QString &str){
	pattern = str;
	is_regex = false;

	return *this;
}

JudgeStruct::JudgeStruct()
	:who(NULL), card(NULL), good(true), time_consuming(false)
{

}

bool JudgeStruct::isGood(const Card *card) const{
	if(card == NULL)
		card = this->card;

	if(good)
		return pattern.match(who, card);
	else
		return !pattern.match(who, card);
}

bool JudgeStruct::isBad() const{
	return ! isGood();
}

PhaseChangeStruct::PhaseChangeStruct()
	:from(Player::NotActive), to(Player::NotActive)
{}

CardUseStruct::CardUseStruct()
	:card(NULL), from(NULL)
{
}

bool CardUseStruct::isValid() const{
	return card != NULL;
}

bool CardUseStruct::tryParse(const QJsonValue &usage_data, Room *room){
	QJsonArray usage = usage_data.toArray();
	if(usage.size() < 2 || !usage[0].isString() || !usage[1].isArray())
		return false;

	card = Card::Parse(usage[0].toString());

	const QJsonArray targets = usage[1].toArray();

	for(unsigned int i = 0; i < targets.size(); i++){
		if(!targets[i].isString()) return false;
		this->to << room->findChild<ServerPlayer *>(targets[i].toString());
	}
	return true;
}

void CardUseStruct::parse(const QString &str, Room *room){
	QStringList words = str.split("->", QString::KeepEmptyParts);
	
	Q_ASSERT(words.length() == 1 || words.length() == 2);

	QString card_str = words.at(0);
	QString target_str = ".";
	
	//@todo: it's observed that when split on "a->."
	// only returns one QString, which is "a". Suspect
	// it's a bug with QT regular expression. Figure out
	// the cause of the issue.
	if(words.length() == 2 && !words.at(1).isEmpty())
		target_str = words.at(1);

	card = Card::Parse(card_str);

	if(target_str != "."){
		QStringList target_names = target_str.split("+");
		foreach(QString target_name, target_names)
			to << room->findChild<ServerPlayer *>(target_name);
	}
}

QString EventTriplet::toString() const{
	return QString("event = %1, target = %2[%3], data = %4[%5]")
			.arg(event)
			.arg(target->objectName()).arg(target->getGeneralName())
			.arg(data->toString()).arg(data->typeName());
}

RoomDriver::RoomDriver(Room *room)
	:room(room)
{
	setParent(room);

	controller = new RoomDriverController(this);
	controller->moveToThread(&thread);
	connect(&thread, SIGNAL(finished()), controller, SLOT(deleteLater()));
	connect(this, SIGNAL(driver_start()), controller, SLOT(run()));
	connect(controller, SIGNAL(game_finished()), this, SIGNAL(driver_finished()));
	connect(controller, SIGNAL(game_finished()), &thread, SLOT(quit()));
	thread.start();
}

RoomDriver::~RoomDriver(){
	thread.wait();
}

void RoomDriver::start(){
	emit driver_start();
}

void RoomDriver::addPlayerSkills(ServerPlayer *player, bool invoke_game_start){
	QVariant void_data;

	foreach(const TriggerSkill *skill, player->getTriggerSkills()){
		addTriggerSkill(skill);

		if(invoke_game_start && skill->getTriggerEvents().contains(GameStart))
			skill->trigger(GameStart, player, void_data);
	}
}

void RoomDriver::constructTriggerTable(){
	foreach(ServerPlayer *player, room->getPlayers()){
		addPlayerSkills(player, true);
	}
}

void RoomDriver::run3v3(){
	QList<ServerPlayer *> warm, cool;
	foreach(ServerPlayer *player, room->m_players){
		switch(player->getRoleEnum()){
		case Player::Lord: warm.prepend(player); break;
		case Player::Loyalist: warm.append(player); break;
		case Player::Renegade: cool.prepend(player); break;
		case Player::Rebel: cool.append(player); break;
		}
	}

	QString order = room->askForOrder(cool.first());
	QList<ServerPlayer *> *first, *second;

	if(order == "warm"){
		first = &warm;
		second = &cool;
	}else{
		first = &cool;
		second = &warm;
	}

	action3v3(first->first());

	forever{
		qSwap(first, second);

		QList<ServerPlayer *> targets;
		foreach(ServerPlayer *player, *first){
			if(!player->hasFlag("actioned") && player->isAlive())
				targets << player;
		}

		ServerPlayer *to_action = room->askForPlayerChosen(first->first(), targets, "3v3-action");
		if(to_action){
			action3v3(to_action);

			if(to_action != first->first()){
				ServerPlayer *another;
				if(to_action == first->last())
					another = first->at(1);
				else
					another = first->last();

				if(!another->hasFlag("actioned") && another->isAlive())
					action3v3(another);
			}
		}
	}
}

void RoomDriver::action3v3(ServerPlayer *player){
	room->setCurrent(player);
	trigger(TurnStart, room->getCurrent());
	room->setPlayerFlag(player, "actioned");

	bool all_actioned = true;
	foreach(ServerPlayer *player, room->m_alivePlayers){
		if(!player->hasFlag("actioned")){
			all_actioned = false;
			break;
		}
	}

	if(all_actioned){
		foreach(ServerPlayer *player, room->m_alivePlayers){
			room->setPlayerFlag(player, "-actioned");
		}
	}
}

RoomDriverController::RoomDriverController(RoomDriver *driver)
	:driver(driver)
{
	room = driver->room;
}

void RoomDriverController::run(){
	qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));
	
	Server *server = room->getServer();

	const GameRule *game_rule;
	if(room->getMode() == "04_1v3")
		game_rule = server->getHulaoPassMode();
	else if(Config.EnableScene)	//changjing
		game_rule = server->getSceneRule();	//changjing
	else
		game_rule = server->getGameRule();

	driver->addTriggerSkill(game_rule);

	driver->addTriggerSkill(server->getAbortGameRule());

	if(Config.EnableBasara){
		driver->addTriggerSkill(server->getBasaraMode());
	}

	if(room->getScenario() != NULL){
		const ScenarioRule *rule = room->getScenario()->getRule();
		if(rule)
			driver->addTriggerSkill(rule);
	}

	// start game, draw initial 4 cards
	try {
		QVariant roomdata = QVariant::fromValue<RoomStar>(room);
		driver->trigger(GameStart, NULL, roomdata);
		driver->constructTriggerTable();

		if(room->getMode() == "06_3v3"){
			driver->run3v3();
		}else if(room->getMode() == "04_1v3"){
			ServerPlayer *shenlvbu = room->getLord();
			try {
				QList<ServerPlayer *> league = room->getPlayers();
				league.removeOne(shenlvbu);

				forever{
					foreach(ServerPlayer *player, league){
						if(player->hasFlag("actioned"))
							room->setPlayerFlag(player, "-actioned");
					}

					foreach(ServerPlayer *player, league){
						room->setCurrent(player);
						driver->trigger(TurnStart, room->getCurrent());

						if(!player->hasFlag("actioned"))
							room->setPlayerFlag(player, "actioned");

						if(player->isAlive()){
							room->setCurrent(shenlvbu);
							driver->trigger(TurnStart, room->getCurrent());
						}
					}
				}
			}catch(TriggerEvent event){
				driver->trigger(event, NULL, roomdata);
				foreach(ServerPlayer *player, room->getPlayers()){
					if(player != shenlvbu){
						if(player->hasFlag("actioned"))
							room->setPlayerFlag(player, "-actioned");

						if(player->getPhase() != Player::NotActive){
							PhaseChangeStruct phase;
							phase.from = player->getPhase();
							room->setPlayerProperty(player, "phase", "not_active");
							phase.to = player->getPhase();
							QVariant data = QVariant::fromValue(phase);
							driver->trigger(PhaseChange, player, data);
						}
					}
				}

				room->setCurrent(shenlvbu);

				forever{
					driver->trigger(TurnStart, room->getCurrent());
					room->setCurrent(room->getCurrent()->getNext());
				}
			}
		}else{
			if(room->getMode() == "02_1v1")
				room->setCurrent(room->getPlayers().at(1));

			forever{
				driver->trigger(TurnStart, room->getCurrent());
				if(room->isFinished()) break;
				room->setCurrent(room->getCurrent()->getNextAlive());
			}
		}
	}catch(TriggerEvent event){
		if(event == GameFinished){
			emit game_finished();
			return;
		}
	}
}

static bool CompareByPriority(const TriggerSkill *a, const TriggerSkill *b){
	return a->getPriority() > b->getPriority();
}

bool RoomDriver::trigger(TriggerEvent event, ServerPlayer *target, QVariant &data){
	// push it to event stack
	event_stack.push_back(EventTriplet(event, target, &data));

	bool broken = false;
	foreach(const TriggerSkill *skill, skill_table[event]){
		if(skill->triggerable(target)){
			broken = skill->trigger(event, target, data);
			if(broken)
				break;
		}
	}

	if(target){
		foreach(AI *ai, room->ais){
			ai->filterEvent(event, target, data);
		}
	}

	// pop event stack
	event_stack.pop_back();

	return broken;
}

const QList<EventTriplet> *RoomDriver::getEventStack() const{
	return &event_stack;
}

bool RoomDriver::trigger(TriggerEvent event, ServerPlayer *target){
	QVariant data;
	return trigger(event, target, data);
}

void RoomDriver::addTriggerSkill(const TriggerSkill *skill){
	if(skill_set.contains(skill))
		return;

	skill_set << skill;

	QList<TriggerEvent> events = skill->getTriggerEvents();
	foreach(TriggerEvent event, events){
		QList<const TriggerSkill *> &table = skill_table[event];

		table << skill;
		qStableSort(table.begin(), table.end(), CompareByPriority);
	}

	if(skill->isVisible()){
		foreach(const Skill *skill, Bang->getRelatedSkills(skill->objectName())){
			const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(skill);
			addTriggerSkill(trigger_skill);
		}
	}
}

void RoomDriver::delay(unsigned long secs){
	if(room->property("to_test").toString().isEmpty()&& Config.AIDelay>0)
		QThread::msleep(secs);
}
