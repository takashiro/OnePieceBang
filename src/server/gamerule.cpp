#include "gamerule.h"
#include "serverplayer.h"
#include "room.h"
#include "standard.h"
#include "engine.h"
#include "settings.h"

#include <QTime>

GameRule::GameRule(QObject *)
	:TriggerSkill("game_rule")
{
	//@todo: this setParent is illegitimate in QT and is equivalent to calling
	// setParent(NULL). So taking it off at the moment until we figure out
	// a way to do it.
	//setParent(parent);

	events << GameStart << TurnStart << PhaseChange << CardUsed << CardFinished
			<< CardEffected << Recovered << HpLost << AskForWineDone
			<< AskForWine << Death << Dying << GameOverJudge
			<< SlashHit << SlashMissed << SlashEffected << SlashProceed
			<< Damaged << DamageDone << DamageComplete
			<< StartJudge << FinishJudge << Pindian;
}

bool GameRule::triggerable(const ServerPlayer *) const{
	return true;
}

int GameRule::getPriority() const{
	return 0;
}

void GameRule::onPhaseChange(ServerPlayer *player) const{
	Room *room = player->getRoom();
	switch(player->getPhase()){
	case Player::RoundStart:{
			break;
		}
	case Player::Start: {
			player->setMark("SlashCount", 0);
			break;
		}
	case Player::Judge: {
			QList<const DelayedTrick *> tricks = player->delayedTricks();
			while(!tricks.isEmpty() && player->isAlive()){
				const DelayedTrick *trick = tricks.takeLast();
				bool on_effect = room->cardEffect(trick, NULL, player);
				if(!on_effect)
					trick->onNullified(player);
			}
			break;
		}
	case Player::Draw: {
			QVariant num = 2;
			if(room->getTag("FirstRound").toBool()){
				room->setTag("FirstRound", false);
				if(room->getMode() == "02_1v1")
					num = 1;
			}

			room->getThread()->trigger(DrawNCards, player, num);
			int n = num.toInt();
			if(n > 0)
				player->drawCards(n, false);
			break;
		}

	case Player::Play: {
			player->clearHistory();

			while(player->isAlive()){
				CardUseStruct card_use;
				room->activate(player, card_use);
				if(card_use.isValid()){
					room->useCard(card_use);
				}else
					break;
			}
			break;
		}

	case Player::Discard:{
			while(player->getHandcardNum() > player->getMaxCards()){
				int discard_num = player->getHandcardNum() - player->getMaxCards();
				if(player->hasFlag("jilei")){
					QSet<const Card *> jilei_cards;
					QList<const Card *> handcards = player->getHandcards();
					foreach(const Card *card, handcards){
						if(player->isJilei(card))
							jilei_cards << card;
					}

					if(jilei_cards.size() > player->getMaxCards()){
						// show all his cards
						room->showAllCards(player);

						DummyCard *dummy_card = new DummyCard;
						foreach(const Card *card, handcards.toSet() - jilei_cards){
							dummy_card->addSubcard(card);
						}
						room->throwCard(dummy_card, player);

						return;
					}
				}
				if(discard_num > 0){
					room->askForDiscard(player, "gamerule", discard_num, 1);
				}
			}
			break;
		}
	case Player::Finish: {
			break;
		}

	case Player::NotActive:{
			if(player->hasFlag("drank")){
				LogMessage log;
				log.type = "#UnsetDrankEndOfTurn";
				log.from = player;
				room->sendLog(log);

				room->setPlayerFlag(player, "-drank");
			}

			player->clearFlags();
			player->clearHistory();
			room->broadcastNotification(BP::ClearPile);
			return;
		}
	}
}

void GameRule::setGameProcess(Room *room) const{
	int good = 0, bad = 0;
	QList<ServerPlayer *> players = room->getAlivePlayers();
	foreach(ServerPlayer *player, players){
		switch(player->getRoleEnum()){
		case Player::Lord:
		case Player::Loyalist: good ++; break;
		case Player::Rebel: bad++; break;
		case Player::Renegade: break;
		}
	}

	QString process;
	if(good == bad)
		process = "Balance";
	else if(good > bad)
		process = "LordSuperior";
	else
		process = "RebelSuperior";

	room->setTag("GameProcess", process);
}

bool GameRule::trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
	Room *room;
	if(player == NULL)
		room = data.value<RoomStar>();
	else
		room = player->getRoom();

	if(room->getTag("SkipGameRule").toBool()){
		room->removeTag("SkipGameRule");
		return false;
	}

	// Handle global events
	if(player == NULL){
		if(event == GameStart){
			foreach(ServerPlayer* player, room->getPlayers()){
				if(player->getGeneral()->getKingdom() == "god" && player->getGeneralName() != "anjiang"){
					QString new_kingdom = room->askForKingdom(player);
					room->setPlayerProperty(player, "kingdom", new_kingdom);

					LogMessage log;
					log.type = "#ChooseKingdom";
					log.from = player;
					log.arg = new_kingdom;
					room->sendLog(log);
				}
			}
			setGameProcess(room);
			room->setTag("FirstRound", true);
			room->drawCards(room->getPlayers(), 4, false);
		}
		return false;
	}

	switch(event){
	case TurnStart:{
		player = room->getCurrent();
		if(!player->faceUp())
			player->turnOver();
		else if(player->isAlive())
			player->play();

		break;
	}

	case PhaseChange: onPhaseChange(player); break;

	case CardUsed:{
		if(data.canConvert<CardUseStruct>()){
			RoomThread *thread = room->getThread();
			CardUseStruct card_use = data.value<CardUseStruct>();

			if(thread->trigger(TargetSelect, card_use.from, data)){
				break;
			}

			foreach(ServerPlayer *target, card_use.to){
				thread->trigger(TargetSelected, target, data);
			}

			card_use = data.value<CardUseStruct>();
			card_use.from->playCardEffect(card_use.card);
			card_use.card->use(room, card_use.from, card_use.to);
		}

		break;
	}

	case CardFinished: {
		CardUseStruct use = data.value<CardUseStruct>();
		room->clearCardFlag(use.card);

		break;
	}

	case Recovered:{
		RecoverStruct recover_struct = data.value<RecoverStruct>();
		int recover = recover_struct.recover;

		room->setPlayerStatistics(player, "recover", recover);

		int new_hp = qMin(player->getHp() + recover, player->getMaxHp());
		room->setPlayerProperty(player, "hp", new_hp);

		QJsonArray arg;
		arg.append(player->objectName());
		arg.append(recover);
		room->broadcastNotification(BP::HpChange, arg);

		break;
	}

	case HpLost:{
		int lose = data.toInt();

		LogMessage log;
		log.type = "#LoseHp";
		log.from = player;
		log.arg = QString::number(lose);
		room->sendLog(log);

		room->setPlayerProperty(player, "hp", player->getHp() - lose);

		QJsonArray arg;
		arg.append(player->objectName());
		arg.append(-lose);
		arg.append(QString("L"));
		room->broadcastNotification(BP::HpChange, arg);

		if(player->getHp() <= 0)
			room->enterDying(player, NULL);

		break;
	}

	case Dying:{
		if(player->getHp() > 0){
			player->setFlags("-dying");
			break;
		}

		DyingStruct dying = data.value<DyingStruct>();

		LogMessage log;
		log.type = "#AskForWine";
		log.from = player;
		log.to = dying.savers;
		log.arg = QString::number(1 - player->getHp());
		room->sendLog(log);

		RoomThread *thread = room->getThread();
		foreach(ServerPlayer *saver, dying.savers){
			if(player->getHp() > 0)
				break;

			thread->trigger(AskForWine, saver, data);
		}

		player->setFlags("-dying");
		thread->trigger(AskForWineDone, player, data);

		break;
	}

	case AskForWine:{
		DyingStruct dying = data.value<DyingStruct>();

		while(dying.who->getHp() <= 0){
			const Card *wine = room->askForSingleWine(player, dying.who);
			if(wine == NULL)
				break;

			CardUseStruct use;
			use.card = wine;
			use.from = player;
			if(player != dying.who)
				use.to << dying.who;

			room->useCard(use, false);

			if(player != dying.who && dying.who->getHp() > 0)
				room->setPlayerStatistics(player, "save", 1);
		}

		break;
	}

	case AskForWineDone:{
		if(player->getHp() <= 0 && player->isAlive()){
			DyingStruct dying = data.value<DyingStruct>();
			room->killPlayer(player, dying.damage);

			if(dying.damage && dying.damage->from)
				room->setPlayerStatistics(dying.damage->from, "kill", 1);
		}

		break;
	}

	case Damaged:{
		DamageStruct damage = data.value<DamageStruct>();
		if(damage.to != NULL && damage.damage > 1){
			const Card *haki = room->askForCard(damage.to, "busou_haki", "damage-busou-haki", data);
			if(haki != NULL){
				LogMessage log;
				log.type = "#BusouHakiDefense";
				log.from = damage.to;
				log.arg = haki->objectName();
				room->sendLog(log);

				return true;
			}
		}
		break;
	}

	case DamageDone:{
		DamageStruct damage = data.value<DamageStruct>();
		room->sendDamageLog(damage);

		if(damage.from)
			room->setPlayerStatistics(damage.from, "damage", damage.damage);

		room->applyDamage(player, damage);
		if(player->getHp() <= 0){
			room->enterDying(player, &damage);
		}

		break;
	}

	case DamageComplete:{
		if(room->getMode() == "02_1v1" && player->isDead()){
			QString new_general = player->tag["1v1ChangeGeneral"].toString();
			if(!new_general.isEmpty())
				changeGeneral1v1(player);
		}

		bool chained = player->isChained();
		if(!chained)
			break;

		DamageStruct damage = data.value<DamageStruct>();
		if(damage.nature != DamageStruct::Normal){
			room->setPlayerProperty(player, "chained", false);

			// iron chain effect
			QList<ServerPlayer *> chained_players = room->getAlivePlayers();
			foreach(ServerPlayer *chained_player, chained_players){
				if(chained_player->isChained()){
					room->setPlayerProperty(chained_player, "chained", false);

					LogMessage log;
					log.type = "#TamaDragonDamage";
					log.from = chained_player;
					room->sendLog(log);

					DamageStruct chain_damage = damage;
					chain_damage.to = chained_player;
					chain_damage.chain = true;

					room->damage(chain_damage);
				}
			}
		}

		break;
	}

	case CardEffected:{
		if(data.canConvert<CardEffectStruct>()){
			CardEffectStruct effect = data.value<CardEffectStruct>();
			if(room->isCanceled(effect))
				return true;

			effect.card->onEffect(effect);
		}

		break;
	}

	case SlashEffected:{
		SlashEffectStruct effect = data.value<SlashEffectStruct>();

		QVariant data = QVariant::fromValue(effect);
		room->getThread()->trigger(SlashProceed, effect.from, data);

		break;
	}

	case SlashProceed:{
		SlashEffectStruct effect = data.value<SlashEffectStruct>();

		QString slasher = effect.from->objectName();
		const Card *jink = room->askForCard(effect.to, "jink", "slash-jink:" + slasher, data, NonTrigger);
		room->slashResult(effect, jink);

		break;
	}

	case SlashHit:{
		SlashEffectStruct effect = data.value<SlashEffectStruct>();

		DamageStruct damage;
		damage.card = effect.slash;

		damage.damage = 1;
		if(effect.drank){
			LogMessage log;
			log.type = "#BusouHakiBuff";
			log.from = effect.from;
			log.to << effect.to;
			log.arg = "busou_haki";
			room->sendLog(log);

			damage.damage++;
		}

		damage.from = effect.from;
		damage.to = effect.to;
		damage.nature = effect.nature;

		room->damage(damage);

		effect.to->removeMark("qinggang");
		break;
	}

	case SlashMissed:{
		SlashEffectStruct effect = data.value<SlashEffectStruct>();
		effect.to->removeMark("qinggang");
		break;
	}

	case GameOverJudge:{
		if(room->getMode() == "02_1v1"){
			QStringList list = player->tag["1v1Arrange"].toStringList();

			if(!list.isEmpty())
				return false;
		}

		QString winner = getWinner(player);
		if(!winner.isNull()){
			player->bury();
			room->gameOver(winner);
			return true;
		}

		break;
	}

	case Death:{
		player->bury();

		if(room->getTag("SkipNormalDeathProcess").toBool())
			return false;

		DamageStar damage = data.value<DamageStar>();
		ServerPlayer *killer = damage ? damage->from : NULL;
		if(killer){
			rewardAndPunish(killer, player);
		}

		setGameProcess(room);

		if(room->getMode() == "02_1v1"){
			QStringList list = player->tag["1v1Arrange"].toStringList();

			if(!list.isEmpty()){
				player->tag["1v1ChangeGeneral"] = list.takeFirst();
				player->tag["1v1Arrange"] = list;

				DamageStar damage = data.value<DamageStar>();

				if(damage == NULL){
					changeGeneral1v1(player);
					return false;
				}
			}
		}

		break;
	}

	case StartJudge:{
		int card_id = room->drawCard();

		JudgeStar judge = data.value<JudgeStar>();
		judge->card = Bang->getCard(card_id);
		room->moveCardTo(judge->card, judge->who, Player::HandlingArea);

		LogMessage log;
		log.type = "$InitialJudge";
		log.from = player;
		log.card_str = judge->card->getEffectIdString();
		room->sendLog(log);

		room->sendJudgeResult(judge);

		break;
	}

	case FinishJudge:{
		JudgeStar judge = data.value<JudgeStar>();
		room->throwCard(judge->card);

		LogMessage log;
		log.type = "$JudgeResult";
		log.from = player;
		log.card_str = judge->card->getEffectIdString();
		room->sendLog(log);

		room->sendJudgeResult(judge);
		break;
	}

	case Pindian:{
		PindianStar pindian = data.value<PindianStar>();

		LogMessage log;

		room->throwCard(pindian->from_card);
		log.type = "$PindianResult";
		log.from = pindian->from;
		log.card_str = pindian->from_card->getEffectIdString();
		room->sendLog(log);

		room->throwCard(pindian->to_card);
		log.type = "$PindianResult";
		log.from = pindian->to;
		log.card_str = pindian->to_card->getEffectIdString();
		room->sendLog(log);

		break;
	}

	default:
		;
	}

	return false;
}

void GameRule::changeGeneral1v1(ServerPlayer *player) const{
	Room *room = player->getRoom();
	QString new_general = player->tag["1v1ChangeGeneral"].toString();
	player->tag.remove("1v1ChangeGeneral");
	room->transfigure(player, new_general, true, true);
	room->revivePlayer(player);

	if(player->getKingdom() != player->getGeneral()->getKingdom())
		room->setPlayerProperty(player, "kingdom", player->getGeneral()->getKingdom());

	room->broadcastNotification(BP::RevealGeneral, BP::toJsonArray(player->objectName(), new_general), player);

	if(!player->faceUp())
		player->turnOver();

	if(player->isChained())
		room->setPlayerProperty(player, "chained", false);

	player->drawCards(4);
}

void GameRule::rewardAndPunish(ServerPlayer *killer, ServerPlayer *victim) const{
	if(killer->isDead())
		return;

	if(killer->getRoom()->getMode() == "06_3v3"){
		if(Config.value("3v3/UsingNewMode", false).toBool())
			killer->drawCards(2);
		else
			killer->drawCards(3);
	}
	else{
		if(victim->getRole() == "rebel" && killer != victim){
			killer->drawCards(3);
		}else if(victim->getRole() == "loyalist" && killer->getRole() == "lord"){
			killer->throwAllEquips();
			killer->throwAllHandCards();
		}
	}
}

QString GameRule::getWinner(ServerPlayer *victim) const{
	Room *room = victim->getRoom();
	QString winner;

	if(room->getMode() == "06_3v3"){
		switch(victim->getRoleEnum()){
		case Player::Lord: winner = "renegade+rebel"; break;
		case Player::Renegade: winner = "lord+loyalist"; break;
		default:
			break;
		}
	}else if(Config.EnableHegemony){
		bool has_anjiang = false, has_diff_kingdoms = false;
		QString init_kingdom;
		foreach(ServerPlayer *p, room->getAlivePlayers()){
			if(room->getTag(p->objectName()).toStringList().size()){
				has_anjiang = true;
			}

			if(init_kingdom.isEmpty()){
				init_kingdom = p->getKingdom();
			}
			else if(init_kingdom != p->getKingdom()){
				has_diff_kingdoms = true;
			}
		}

		if(!has_anjiang && !has_diff_kingdoms){
			QStringList winners;
			QString aliveKingdom = room->getAlivePlayers().first()->getKingdom();
			foreach(ServerPlayer *p, room->getPlayers()){
				if(p->isAlive())winners << p->objectName();
				if(p->getKingdom() == aliveKingdom){
					QStringList generals = room->getTag(p->objectName()).toStringList();
					if(generals.size()&&!Config.Enable2ndGeneral)continue;
					if(generals.size()>1)continue;

					//if someone showed his kingdom before death,
					//he should be considered victorious as well if his kingdom survives
					winners << p->objectName();
				}
			}

			winner = winners.join("+");
		}
	}else{
		QStringList alive_roles = room->aliveRoles(victim);
		switch(victim->getRoleEnum()){
		case Player::Lord:{
				if(alive_roles.length() == 1 && alive_roles.first() == "renegade")
					winner = room->getAlivePlayers().first()->objectName();
				else
					winner = "rebel";
				break;
			}

		case Player::Rebel:
		case Player::Renegade:
			{
				if(!alive_roles.contains("rebel") && !alive_roles.contains("renegade")){
					winner = "lord+loyalist";
					if(victim->getRole() == "renegade" && !alive_roles.contains("loyalist"))
						room->setTag("RenegadeInFinalPK", true);
				}
				break;
			}

		default:
			break;
		}
	}

	return winner;
}

HulaoPassMode::HulaoPassMode(QObject *parent)
	:GameRule(parent)
{
	setObjectName("hulaopass_mode");

	events << HpChanged << StageChange;
	default_choice = "recover";
}

bool HulaoPassMode::trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
	Room *room;
	if(player == NULL)
		room = data.value<RoomStar>();
	else
		room = player->getRoom();
	
	switch(event) {
	case StageChange: {
		ServerPlayer* lord = room->getLord();
		room->transfigure(lord, "shenlvbu2", true, true);

		QList<const Card *> tricks = lord->getJudgingArea();
		foreach(const Card *trick, tricks)
			room->throwCard(trick);
		break;
					  }
	case GameStart: {
		// Handle global events
		if(player == NULL){
			ServerPlayer* lord = room->getLord();
			lord->drawCards(8, false);
			foreach(ServerPlayer* player, room->getPlayers()){
				if(player->isLord())
					continue;
				else
					player->drawCards(player->getSeat() + 1, false);
			}
			return false;
		}
		break;
					}
	case CardUsed:{
		CardUseStruct use = data.value<CardUseStruct>();
		if(use.card->inherits("Weapon") && player->askForSkillInvoke("weapon_recast", data)){
			player->playCardEffect("@recast");
			room->throwCard(use.card);
			player->drawCards(1, false);
			return false;
		}

			break;
		}

	case HpChanged:{
			if(player->getGeneralName() == "shenlvbu1" && player->getHp() <= 4){
				throw StageChange;
			}

			return false;
		}

	case Death:{
			if(player->isLord()){
				room->gameOver("rebel");
			}else{
				if(room->aliveRoles(player).length() == 1)
					room->gameOver("lord");

				LogMessage log;
				log.type = "#Reforming";
				log.from = player;
				room->sendLog(log);

				player->bury();
				room->setPlayerProperty(player, "hp", 0);

				foreach(ServerPlayer *player, room->getOtherPlayers(room->getLord())){
					if(player->askForSkillInvoke("draw_1v3"))
						player->drawCards(1, false);
				}
			}

			return false;
		}

	case TurnStart:{
			if(player->isLord()){
				if(!player->faceUp())
					player->turnOver();
				else
					player->play();
			}else{
				if(player->isDead()){
					if(player->getHp() + player->getHandcardNum() == 6){
						LogMessage log;
						log.type = "#ReformingRevive";
						log.from = player;
						room->sendLog(log);

						room->revivePlayer(player);
					}else if(player->isWounded()){
						if(player->getHp() > 0 && (room->askForChoice(player, "Hulaopass", "recover+draw") == "draw")){
							LogMessage log;
							log.type = "#ReformingDraw";
							log.from = player;
							room->sendLog(log);
							player->drawCards(1, false);
							return false;
						}

						LogMessage log;
						log.type = "#ReformingRecover";
						log.from = player;
						room->sendLog(log);

						room->setPlayerProperty(player, "hp", player->getHp() + 1);
					}else
						player->drawCards(1, false);
				}else if(!player->faceUp())
					player->turnOver();
				else
					player->play();
			}

			return false;
		}

	default:
		break;
	}

	return GameRule::trigger(event, player, data);
}

BasaraMode::BasaraMode(QObject *parent)
	:GameRule(parent)
{
	setObjectName("basara_mode");

	events << OneCardLost << Predamaged;

	skill_mark["niepan"] = "@nirvana";
	skill_mark["smallyeyan"] = "@flame";
	skill_mark["luanwu"] = "@chaos";
	skill_mark["fuli"] = "@laoji";
	skill_mark["zuixiang"] = "@sleep";
}

QString BasaraMode::getMappedRole(const QString &role){
	static QMap<QString, QString> roles;
	if(roles.isEmpty()){
		roles["wei"] = "lord";
		roles["shu"] = "loyalist";
		roles["wu"] = "rebel";
		roles["qun"] = "renegade";
	}
	return roles[role];
}

int BasaraMode::getPriority() const{
	return 5;
}

void BasaraMode::askForGeneralRevealed(ServerPlayer *player) const{
	Room *room = player->getRoom();
	QStringList names = room->getTag(player->objectName()).toStringList();
	if(names.isEmpty())
		return;

	if(Config.EnableHegemony){
		QMap<QString, int> kingdom_roles;
		foreach(ServerPlayer *p, room->getOtherPlayers(player)){
			kingdom_roles[p->getKingdom()]++;
		}

		if(kingdom_roles[Bang->getGeneral(names.first())->getKingdom()] >= 2
				&& player->getGeneralName() == "anjiang")
			return;
	}

	QString answer = room->askForChoice(player, "RevealGeneral", "yes+no");
	if(answer == "yes"){

		QString general_name = room->askForGeneral(player,names);

		showGeneral(player,general_name);
		if(Config.EnableHegemony)room->getThread()->trigger(GameOverJudge, player);
		askForGeneralRevealed(player);
	}
}

void BasaraMode::showGeneral(ServerPlayer *player, QString general_name) const
{
	Room * room = player->getRoom();
	QStringList names = room->getTag(player->objectName()).toStringList();
	if(names.isEmpty())return;

	if(player->getGeneralName() == "anjiang"){
		player->notify(BP::Transfigure, BP::toJsonArray(player->getGeneralName(), general_name));
		room->setPlayerProperty(player,"general",general_name);

		foreach(QString skill_name, skill_mark.keys()){
			if(player->hasSkill(skill_name))
				room->setPlayerMark(player, skill_mark[skill_name], 1);
		}
	}else{
		player->notify(BP::Transfigure, BP::toJsonArray(player->getGeneral2Name(), general_name));
		room->setPlayerProperty(player,"general2",general_name);
	}

	room->setPlayerProperty(player, "kingdom", player->getGeneral()->getKingdom());
	if(Config.EnableHegemony)room->setPlayerProperty(player, "role", getMappedRole(player->getGeneral()->getKingdom()));

	names.removeOne(general_name);
	room->setTag(player->objectName(),QVariant::fromValue(names));

	LogMessage log;
	log.type = "#BasaraReveal";
	log.from = player;
	log.arg  = player->getGeneralName();
	log.arg2 = player->getGeneral2Name();

	room->sendLog(log);
	room->broadcastNotification(BP::PlayAudio, QString("choose-item"));
}

bool BasaraMode::trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
	Room *room;
	if(player == NULL)
		room = data.value<RoomStar>();
	else
		room = player->getRoom();

	// Handle global events
	if(player == NULL){
		if(event == GameStart){
			if(Config.EnableHegemony)
				room->setTag("SkipNormalDeathProcess", true);
			foreach(ServerPlayer* sp, room->getAlivePlayers()){
				sp->notify(BP::Transfigure, BP::toJsonArray(sp->getGeneralName(), QString("anjiang")));
				room->setPlayerProperty(sp,"general","anjiang");
				room->setPlayerProperty(sp,"kingdom","god");

				LogMessage log;
				log.type = "#BasaraGeneralChosen";
				log.arg = room->getTag(sp->objectName()).toStringList().at(0);

				if(Config.Enable2ndGeneral){
					sp->notify(BP::Transfigure, BP::toJsonArray(sp->getGeneral2Name(), QString("anjiang")));
					room->setPlayerProperty(sp,"general2","anjiang");

					log.arg2 = room->getTag(sp->objectName()).toStringList().at(1);
				}

				sp->notify(BP::Log, log.toString());
				sp->tag["roles"] = room->getTag(sp->objectName()).toStringList().join("+");
			}
		}
	}


	player->tag["event"] = event;
	player->tag["event_data"] = data;

	switch(event){
	case CardEffected:{
		if(player->getPhase() == Player::NotActive){
			CardEffectStruct ces = data.value<CardEffectStruct>();
			if(ces.card)
				if(ces.card->inherits("TrickCard") ||
						ces.card->inherits("Slash"))
				askForGeneralRevealed(player);
		}
		break;
	}

	case PhaseChange:{
		if(player->getPhase() == Player::Start)
			askForGeneralRevealed(player);

		break;
	}
	case Predamaged:{
		askForGeneralRevealed(player);
		break;
	}
	case GameOverJudge:{
		if(Config.EnableHegemony){
			if(player->getGeneralName() == "anjiang"){
				QStringList generals = room->getTag(player->objectName()).toStringList();
				room->setPlayerProperty(player, "general", generals.at(0));
				if(Config.Enable2ndGeneral)room->setPlayerProperty(player, "general2", generals.at(1));
				room->setPlayerProperty(player, "kingdom", player->getGeneral()->getKingdom());
				room->setPlayerProperty(player, "role", getMappedRole(player->getKingdom()));
			}
		}
		break;
	}

	case Death:{
		if(Config.EnableHegemony){
			DamageStar damage = data.value<DamageStar>();
			ServerPlayer *killer = damage ? damage->from : NULL;
			if(killer && killer->getKingdom() == damage->to->getKingdom()){
				killer->throwAllEquips();
				killer->throwAllHandCards();
			}
			else if(killer && killer->isAlive()){
				killer->drawCards(3);
			}
		}

		break;
	}

	default:
		break;
	}

	return false;
}
