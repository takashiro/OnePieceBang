#include "standard-generals.h"
#include "client.h"

class Insulator: public TriggerSkill{
public:
	Insulator(): TriggerSkill("insulator"){
		events << Damaged;
		frequency = Compulsory;
	}

	virtual bool trigger(TriggerEvent, ServerPlayer *player, QVariant &data) const{
		DamageStruct damage = data.value<DamageStruct>();

		if(damage.nature == DamageStruct::Thunder){
			Room *room = player->getRoom();
			room->broadcastSkillInvoked(player, objectName());
			room->sendLog("#TriggerSkill", player, objectName());
			return true;
		}

		return false;
	}
};

RubberPistolCard::RubberPistolCard(){
	slash = new Slash(Card::NoSuit, 0);
	slash->setSkillName("rubberpistol");
}

RubberPistolCard::~RubberPistolCard(){
}

bool RubberPistolCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
	return slash->targetFilter(targets, to_select, Self);
}

bool RubberPistolCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
	return slash->targetsFeasible(targets, Self);
}

void RubberPistolCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
	JudgeStruct judge;
	judge.pattern = QRegExp("(.*):(heart|diamond):(.*)");
	judge.good = true;
	judge.reason = "rubberpistol";
	judge.who = source;

	room->judge(judge);

	if(judge.isGood()){
		CardUseStruct use;
		use.from = source;
		use.to = targets;
		use.card = slash;

		room->useCard(use);
	}else{
		room->setPlayerFlag(source, "slash_forbidden");
	}
}

class RubberPistol: public ZeroCardViewAsSkill{
public:
	RubberPistol(): ZeroCardViewAsSkill("rubberpistol"){

	}

	virtual bool isEnabledAtPlay(const Player *player) const{
		return !player->hasFlag("slash_forbidden") && Slash::IsAvailable(player);
	}

	virtual const Card *viewAs() const{
		return new RubberPistolCard;
	}
};

class RubberPistolEx: public TriggerSkill{
public:
	RubberPistolEx():TriggerSkill("#rubberpistol"){
		events << CardAsked;
	}

	virtual int getPriority() const{
		return 2;
	}

	virtual bool trigger(TriggerEvent, ServerPlayer *player, QVariant &data) const{
		QString asked = data.toString();
		if(asked == "slash"){
			Room *room = player->getRoom();
			if(player->askForSkillInvoke("rubberpistol", data)){
				JudgeStruct judge;
				judge.pattern = QRegExp("(.*):(heart|diamond):(.*)");
				judge.good = true;
				judge.reason = "rubberpistol";
				judge.who = player;

				room->judge(judge);
				if(judge.isGood()){
					Slash *slash = new Slash(Card::NoSuit, 0);
					slash->setSkillName("rubberpistol");
					room->provide(slash);

					return true;
				}
			}
		}
		return false;
	}
};

OuKiCard::OuKiCard(){
	target_fixed = true;
}

void OuKiCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const{
	source->loseMark("@haouhaki");

	DamageStruct damage;
	damage.from = source;
	foreach(ServerPlayer *target, room->getOtherPlayers(source)){
		if(!target->isAlive()){
			continue;
		}
		const Card *card = room->askForCard(target, "jink", "ouki");
		if(card != NULL){
			room->throwCard(card);
		}else{
			damage.to = target;
			room->damage(damage);
		}
	}
}

class OuKi: public ZeroCardViewAsSkill{
public:
	OuKi(): ZeroCardViewAsSkill("ouki"){
		frequency = Limited;
	}

	virtual bool isEnabledAtPlay(const Player *player) const{
		return player->getMark("@haouhaki") > 0;
	}

	virtual const Card *viewAs() const{
		return new OuKiCard;
	}
};

LieCard::LieCard(){
	target_fixed = false;
}

bool LieCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
	return targets.length() < 1;
}

bool LieCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
	return targets.length() <= 1;
}

void LieCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
	const Card *subcard = Bang->getCard(subcards.at(0));
	TreasureChest *card = new TreasureChest(subcard->getSuit(), subcard->getNumber());
	card->addSubcard(subcard);
	card->setSkillName("lie");

	CardUseStruct use;
	use.from = source;
	use.to << (targets.isEmpty() ? source : targets.at(0));
	use.card = card;
	room->useCard(use);
}

class Lie: public OneCardViewAsSkill{
public:
	Lie(): OneCardViewAsSkill("lie"){

	}

	virtual bool viewFilter(const CardItem *to_select) const{
		const Card *card = to_select->getCard();
		return !card->isEquipped() && (card->getSuit() == Card::Heart || card->inherits("TreasureChest"));
	}

	virtual const Card *viewAs(CardItem *card_item) const{
		const Card *subcard = card_item->getCard();
		LieCard *card = new LieCard;
		card->setSkillName(objectName());
		card->addSubcard(subcard);
		return card;
	}
};

class Shoot: public TriggerSkill{
public:
	Shoot(): TriggerSkill("shoot"){
		events << Damaging;
		frequency = Compulsory;
	}

	virtual bool trigger(TriggerEvent, ServerPlayer *, QVariant &data) const{
		DamageStruct damage = data.value<DamageStruct>();

		if(damage.from && damage.to && damage.card && damage.card->inherits("Slash") && damage.to->distanceTo(damage.from) > damage.from->getHandcardNum()){
			Room *room = damage.from->getRoom();
			room->playSkillEffect(objectName());
			room->sendLog("#TriggerSkill", damage.from, objectName());

			damage.damage++;
			data = QVariant::fromValue(damage);
		}

		return false;
	}
};

class Divided: public TriggerSkill{
public:
	Divided(): TriggerSkill("divided"){
		events << SlashEffected;
		frequency = Compulsory;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		SlashEffectStruct effect = data.value<SlashEffectStruct>();
		if(effect.slash->getNature() == DamageStruct::Normal){
			Room *room = player->getRoom();
			room->broadcastSkillInvoked(player, objectName());
			room->sendLog("#TriggerSkill", player, objectName());
			return true;
		}

		return false;
	}
};

CannonBallCard::CannonBallCard(){

}

bool CannonBallCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const{
	return targets.isEmpty();
}

bool CannonBallCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
	return targets.length() == 1;
}

void CannonBallCard::onEffect(const CardEffectStruct &effect) const{
	JudgeStruct judge;
	judge.who = effect.to;
	judge.reason = objectName();
	judge.good = false;
	judge.pattern = QRegExp("(.*):(spade|heart):(.*)");

	Room *room = effect.from->getRoom();
	room->judge(judge);

	if(judge.isBad()){
		DamageStruct damage;
		damage.from = effect.from;
		damage.to = effect.to;
		room->damage(damage);
	}
}

class CannonBallViewAsSkill: public ZeroCardViewAsSkill{
public:
	CannonBallViewAsSkill(): ZeroCardViewAsSkill("cannonball"){

	}

	virtual bool isEnabledAtPlay(const Player *) const{
		return false;
	}

	virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
		return pattern == "@@cannonball";
	}

	virtual const Card *viewAs() const{
		return new CannonBallCard;
	}
};

class CannonBall: public DrawCardsSkill{
public:
	CannonBall(): DrawCardsSkill("cannonball"){
		view_as_skill = new CannonBallViewAsSkill;
	}

	virtual int getDrawNum(ServerPlayer *player, int n) const{
		Room *room = player->getRoom();
		if(room->askForUseCard(player, "@@cannonball", "@cannonball-card")){
			return n - 1;
		}

		return n;
	}
};

class Clima: public OneCardViewAsSkill{
public:
	Clima():OneCardViewAsSkill("clima"){

	}

	virtual bool viewFilter(const CardItem *to_select) const{
		const Card *card = to_select->getCard();
		if(to_select->isEquipped() || card->getSuit() == Card::Club || (card->getTypeId() != Card::Basic && card->getTypeId() != Card::Equip)){
			return false;
		}
		switch(card->getSuit()){
		case Card::Spade:
			if(Self->containsTrick("lightning")){
				return false;
			}
			break;
		case Card::Heart:
			if(Self->containsTrick("rain")){
				return false;
			}
			break;
		case Card::Diamond:
			if(Self->containsTrick("tornado")){
				return false;
			}
			break;
		default:;
		}
		return true;
	}

	virtual const Card *viewAs(CardItem *card_item) const{
		const Card *sub = card_item->getCard();
		Card *card = NULL;
		switch(sub->getSuit()){
		case Card::Spade:
			if(Self->containsTrick("lightning")){
				return NULL;
			}
			card = new Lightning(sub->getSuit(), sub->getNumber());
			break;
		case Card::Heart:
			if(Self->containsTrick("rain")){
				return NULL;
			}
			card = new Rain(sub->getSuit(), sub->getNumber());
			break;
		case Card::Diamond:
			if(Self->containsTrick("tornado")){
				return NULL;
			}
			card = new Tornado(sub->getSuit(), sub->getNumber());
		default:;
		}

		if(card != NULL){
			card->setSkillName(objectName());
			card->addSubcard(sub);
		}
		return card;
	}
};

ForecastCard::ForecastCard(){
}

class ForecastViewAsSkill: public OneCardViewAsSkill{
public:
	ForecastViewAsSkill(): OneCardViewAsSkill("forecast"){

	}

	virtual bool viewFilter(const CardItem *to_select) const{
		return !to_select->isEquipped();
	}

	virtual const Card *viewAs(CardItem *card_item) const{
		ForecastCard *card = new ForecastCard;
		card->addSubcard(card_item->getFilteredCard());
		card->setSkillName("forecast");
		return card;
	}

	virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
		return !player->isKongcheng() && pattern == "@forecast-card";
	}

	virtual bool isEnabledAtPlay(const Player *) const{
		return false;
	}
};

class Forecast: public TriggerSkill{
public:
	Forecast():TriggerSkill("forecast"){
		events << AskForRetrial;
		view_as_skill = new ForecastViewAsSkill;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return TriggerSkill::triggerable(target) && !target->isKongcheng();
	}

	virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
		Room *room = player->getRoom();
		JudgeStar judge = data.value<JudgeStar>();
		if(judge->who->getPhase() != Player::Judge){
			return false;
		}

		QStringList prompt_list;
		prompt_list << "@forecast-card" << judge->who->objectName()
					<< "forecast" << judge->reason << judge->card->getEffectIdString();
		QString prompt = prompt_list.join(":");
		const Card *card = room->askForCard(player, "@forecast-card", prompt, data);

		if(card){
			QList<CardsMoveStruct> moves;

			CardsMoveStruct old_move;
			old_move.card_ids << judge->card->getId();
			old_move.to_place = Player::DiscardPile;
			moves << old_move;

			judge->card = Bang->getCard(card->getEffectiveId());

			CardsMoveStruct new_move;
			new_move.card_ids << judge->card->getId();
			new_move.to_place = Player::HandlingArea;
			moves << new_move;

			room->moveCardsAtomic(moves, true);

			LogMessage log;
			log.type = "$ChangedJudge";
			log.from = player;
			log.to << judge->who;
			log.card_str = QString::number(judge->card->getId());
			room->sendLog(log);

			room->sendJudgeResult(judge);
			delete card;
		}

		return false;
	}
};

class Mirage: public TriggerSkill{
public:
	Mirage(): TriggerSkill("mirage"){
		events << Damaged;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return TriggerSkill::triggerable(target) && !target->getJudgingArea().isEmpty();
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		if(!player->askForSkillInvoke(objectName(), data)){
			return false;
		}

		Room *room = player->getRoom();
		int card_id = room->askForCardChosen(player, player, "j", objectName());
		room->throwCard(card_id, NULL);
		return true;
	}
};

class FretyWind: public TriggerSkill{
public:
	FretyWind():TriggerSkill("fretywind"){
		events << OneCardLost;
		default_choice = "uninvoke";
	}

	virtual bool trigger(TriggerEvent, ServerPlayer *player, QVariant &data) const{
		CardMoveStar move = data.value<CardMoveStar>();
		if(move->from_place != Player::EquipArea || move->to == move->from){
			return false;
		}

		Room *room = player->getRoom();

		QList<ServerPlayer *> duel_targets = room->getOtherPlayers(player);

		QList<ServerPlayer *> slash_targets;
		foreach(ServerPlayer *target, duel_targets){
			if(player->inMyAttackRange(target)){
				slash_targets.append(target);
			}
		}

		QStringList choices;
		if(!slash_targets.isEmpty()){
			choices << "slash";
		}
		choices << "duel" << "uninvoke";

		QString choice = room->askForChoice(player, objectName(), choices.join("+"));
		if(choice == "uninvoke"){
			return false;
		}

		ServerPlayer *target;
		Card *card;

		if(choice == "slash"){
			target = room->askForPlayerChosen(player, slash_targets, objectName());
			card = new Slash(Card::NoSuit, 0);
		}else{
			target = room->askForPlayerChosen(player, duel_targets, objectName());
			card = new Duel(Card::NoSuit, 0);
		}
		card->setSkillName(objectName());

		CardUseStruct use;
		use.card = card;
		use.from = player;
		use.to << target;
		room->useCard(use);

		return false;
	}
};

class TripleSword: public TriggerSkill{
public:
	TripleSword(): TriggerSkill("triplesword"){
		events << CardUsed << OneCardLost;
		frequency = Compulsory;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		if(event == OneCardLost){
			CardMoveStar move = data.value<CardMoveStar>();
			if(move->to != move->from && move->from_place == Player::EquipArea && Bang->getCard(move->card_id)->getSubtype() == "weapon" && player->getWeapon() == NULL && !player->getPile("sword").isEmpty()){
				Room *room = player->getRoom();

				QList<int> swords = player->getPile("sword");
				room->fillAG(swords, player);
				int weapon_id = room->askForAG(player, swords, false, "triplesword");
				room->clearAG();
				room->moveCardTo(Bang->getCard(weapon_id), player, Player::EquipArea, true);
			}
		}else{
			CardUseStruct use = data.value<CardUseStruct>();
			if(use.card && use.card->getSubtype() == "weapon"){
				const Card *weapon = player->getWeapon();
				Room *room = player->getRoom();
				if(weapon != NULL && player->getPile("sword").length() < 2){
					player->addToPile("sword", weapon);
				}
			}
		}

		return false;
	}
};

class Gentleman: public TriggerSkill{
public:
	Gentleman(): TriggerSkill("gentleman"){
		events << AfterRecovering << AfterRecovered;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		RecoverStruct recover = data.value<RecoverStruct>();

		if(event == AfterRecovering){
			if(recover.to->getGender() == General::Female){
				if(player->askForSkillInvoke(objectName(), data)){
					player->drawCards(recover.recover, true, objectName());
				}
			}
		}else{
			if(recover.from->getGender() == General::Female){
				if(player->askForSkillInvoke(objectName(), data)){
					recover.from->drawCards(recover.recover, true, objectName());
				}
			}
		}

		return false;
	}
};

class BlackFeet: public OneCardViewAsSkill{
public:
	BlackFeet(): OneCardViewAsSkill("blackfeet"){

	}

	virtual bool viewFilter(const CardItem *to_select) const{
		const Card *card = to_select->getCard();
		switch(ClientInstance->getStatus()){
		case Client::Playing:{
			return (Slash::IsAvailable(Self) && card->inherits("Jink")) || (Self->isWounded() && card->inherits("Slash"));
		}
		case Client::Responsing:{
			if(ClientInstance->getPattern() == "slash"){
				return card->inherits("Jink");
			}else if(ClientInstance->getPattern() == "wine"){
				return card->inherits("Slash");
			}
		}
		}

		return false;
	}

	virtual bool isEnabledAtPlay(const Player *player) const{
		return Slash::IsAvailable(player) || player->isWounded();
	}

	virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
		return pattern == "slash" || pattern.contains("wine");
	}

	virtual const Card *viewAs(CardItem *card_item) const{
		const Card *sub = card_item->getCard();
		Card *card = NULL;
		if(sub->inherits("Jink")){
			card = new FireSlash(sub->getSuit(), sub->getNumber());
		}else if(sub->inherits("Slash")){
			card = new Wine(sub->getSuit(), sub->getNumber());
		}

		if(card != NULL){
			card->addSubcard(sub);
			card->setSkillName(objectName());
		}

		return card;
	}
};

SwordFanCard::SwordFanCard(){
	target_fixed = true;
}

void SwordFanCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const{
	room->throwCard(this, source);
}

class SwordFanViewAsSkill: public OneCardViewAsSkill{
public:
	SwordFanViewAsSkill(): OneCardViewAsSkill("swordfan"){

	}

	virtual bool viewFilter(const CardItem *to_select) const{
		return !to_select->isEquipped();
	}

	virtual bool isEnabledAtPlay(const Player *player) const{
		return false;
	}

	virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
		return pattern == "@@swordfan";
	}

	virtual const Card *viewAs(CardItem *card_item) const{
		SwordFanCard *card = new SwordFanCard;
		card->addSubcard(card_item->getFilteredCard());
		card->setSkillName("swordfan");
		return card;
	}
};

class SwordFan: public TriggerSkill{
public:
	SwordFan(): TriggerSkill("swordfan"){
		events << Postdamaged;
		view_as_skill = new SwordFanViewAsSkill;
	}

	virtual bool trigger(TriggerEvent, ServerPlayer *player, QVariant &data) const{
		DamageStruct damage = data.value<DamageStruct>();
		if(damage.from && player->getHandcardNum() > 0 && !damage.from->getCards("he").isEmpty()){
			Room *room = player->getRoom();

			player->tag["SwordFanDamage"] = data;
			QString prompt = QString("@swordfan-card:%1").arg(damage.from->objectName());
			if(room->askForUseCard(player, "@@swordfan", prompt)){
				int card_id = room->askForCardChosen(player, damage.from, "he", objectName());
				const Card *card = Bang->getCard(card_id);
				player->obtainCard(card, true);

				if(card->inherits("Weapon")){
					RecoverStruct recover;
					recover.from = player;
					recover.recover = 1;
					recover.to = player;
					room->recover(recover);
				}
			}
		}

		return false;
	}
};

class Myopia: public TriggerSkill{
public:
	Myopia(): TriggerSkill("myopia"){
		events << CardEffected;
		frequency = Compulsory;
	}

	virtual bool trigger(TriggerEvent, ServerPlayer *player, QVariant &data) const{
		CardEffectStruct effect = data.value<CardEffectStruct>();
		if(effect.from && effect.card && effect.card->isNDTrick() && !player->inMyAttackRange(effect.from)){
			Room *room = player->getRoom();
			room->broadcastSkillInvoked(player, objectName());
			room->sendLog("#TriggerSkill", player, objectName());
			return true;
		}
		return false;
	}
};

class SharkOnTooth: public TriggerSkill{
public:
	SharkOnTooth(): TriggerSkill("sharkontooth"){
		events << SlashMissed;
	}

	virtual bool trigger(TriggerEvent, ServerPlayer *player, QVariant &data) const{
		SlashEffectStruct effect = data.value<SlashEffectStruct>();
		if(!effect.slash || !effect.slash->isBlack()){
		return false;
		}

		if(player->askForSkillInvoke(objectName())){
		Room *room = player->getRoom();

		Slash *slash = new Slash(Card::NoSuit, 0);
		slash->setSkillName(objectName());

		CardUseStruct use;
		use.from = player;
		use.card = slash;

		foreach(ServerPlayer *target, room->getOtherPlayers(player)){
			if(player->inMyAttackRange(target)){
				use.to << target;
			}
		}

		room->useCard(use);
		}

		return false;
	}
};

class Waltz: public TriggerSkill{
public:
	Waltz(): TriggerSkill("waltz"){
		events << SlashMissed;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		if(player->askForSkillInvoke(objectName(), data)){
			SlashEffectStruct effect = data.value<SlashEffectStruct>();
			Room *room = player->getRoom();

			int card_id = room->askForCardChosen(player, effect.to, "he", objectName());
			room->throwCard(card_id, player);
		}

		return false;
	}
};

class FogBarrier: public TriggerSkill{
public:
	FogBarrier(): TriggerSkill("fogbarrier"){
		events << PhaseChange;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		Room *room = player->getRoom();

		if(player->getPhase() == Player::Start){
			foreach(ServerPlayer *player, room->getAllPlayers()){
				room->setPlayerMark(player, "@fogbarrier", 0);
			}
		}else if(player->getPhase() == Player::Finish && player->askForSkillInvoke(objectName())){
			ServerPlayer *target = room->askForPlayerChosen(player, room->getAllPlayers(), objectName());
			room->setPlayerMark(target, "@fogbarrier", 1);
		}

		return false;
	}
};

class FogBarrierEffect: public TriggerSkill{
public:
	FogBarrierEffect(): TriggerSkill("#fogbarriereffect"){
		events << CardEffected << Damaged;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return target->getMark("@fogbarrier") > 0;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		Room *room = player->getRoom();
		if(event == CardEffected){
			CardEffectStruct effect = data.value<CardEffectStruct>();
			if(effect.card && effect.card->isNDTrick()){
				room->broadcastSkillInvoked(player, "fogbarrier");
				room->sendLog("#TriggerSkill", player, "fogbarrier");
				return true;
			}

		}else if(event == Damaged){
			DamageStruct damage = data.value<DamageStruct>();
			if(damage.nature != DamageStruct::Normal){
				room->broadcastSkillInvoked(player, "fogbarrier");
				room->sendLog("#TriggerSkill", player, "fogbarrier");
				damage.damage++;
				data = QVariant::fromValue(damage);
			}
		}

		return false;
	}
};

class Justice: public TriggerSkill{
public:
	Justice(): TriggerSkill("justice"){
		events << Damaged;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		DamageStruct damage = data.value<DamageStruct>();
		if(damage.card && damage.card->getSuit() == Card::Spade){
			Room *room = player->getRoom();
			room->broadcastSkillInvoked(player, objectName());
			room->sendLog("#TriggerSkill", player, objectName());
			return true;
		}
		return false;
	}
};

class TopSwordman: public TriggerSkill{
public:
	TopSwordman(): TriggerSkill("topswordman"){
		events << Damaged << Damaging;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		DamageStruct damage = data.value<DamageStruct>();
		if(damage.nature != DamageStruct::Normal || damage.card == NULL || !damage.card->inherits("Slash")){
			return false;
		}

		Room *room = player->getRoom();
		if(event == Damaged){
			if(damage.from && damage.from->getWeapon()){
				damage.damage--;
				data = QVariant::fromValue(damage);

				room->broadcastSkillInvoked(player, objectName());
				room->sendLog("#TriggerSkill", player, objectName());
			}
		}else if(event == Damaging){
			if(player->getWeapon()){
				damage.damage++;
				data = QVariant::fromValue(damage);

				room->broadcastSkillInvoked(player, objectName());
				player->getRoom()->sendLog("#TriggerSkill", player, objectName());
			}
		}

		return false;
	}
};


class Upright: public TriggerSkill{
public:
	Upright(): TriggerSkill("upright"){
		events << Postdamaged;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		if(!player->askForSkillInvoke(objectName())){
			return false;
		}

		player->drawCards(1);

		DamageStruct damage = data.value<DamageStruct>();
		if(damage.from){
			CardUseStruct use;
			use.from = player;
			use.to << damage.from;
			use.card = new Duel(Card::NoSuit, 0);

			Room *room = player->getRoom();
			room->useCard(use);
		}

		return false;
	}
};

class Protect: public TriggerSkill{
public:
	Protect(): TriggerSkill("protect"){
		events << TargetSelect;
	}

	bool triggerable(const ServerPlayer *target) const{
		if(target == NULL){
			return false;
		}

		Room *room = target->getRoom();
		foreach(ServerPlayer *player, room->findPlayersBySkillName(objectName())){
			if(!target->inMyAttackRange(player) || target == player){
				continue;
			}

			if(player->hasSkill(objectName())){
				return true;
			}
		}

		return false;
	}

	bool trigger(TriggerEvent event, ServerPlayer *target, QVariant &data) const{
		CardUseStruct use = data.value<CardUseStruct>();
		if(!use.card->inherits("Slash")){
			return false;
		}

		Room *room = target->getRoom();
		foreach(ServerPlayer *player, room->findPlayersBySkillName(objectName())){
			if(!target->inMyAttackRange(player) || target == player){
				continue;
			}

			if(player->hasSkill(objectName())){
				if(use.to.length() == 1 && use.to.contains(player)){
					continue;
				}

				if(player->askForSkillInvoke(objectName())){
					use.to.clear();
					use.to << player;
					data = QVariant::fromValue(use);

					LogMessage log;
					log.type = "#ProtectTargetChange";
					log.from = target;
					log.to << player;
					log.arg = use.card->objectName();
					room->sendLog(log);
				}
			}
		}

		return false;
	}
};

class MassiveAxe: public TriggerSkill{
public:
	MassiveAxe(): TriggerSkill("massiveaxe"){
		events << SlashMissed << Predamaging << PhaseChange << HpChanged;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		Room *room = player->getRoom();
		if(event == SlashMissed){
			room->setPlayerFlag(player, "massiveaxe_enabled");
		}else if(event == PhaseChange || event == HpChanged){
			if(player->getPhase() == Player::Play){
				if(player->isWounded()){
					room->setPlayerFlag(player, "slash_count_unlimited");
				}else{
					room->setPlayerFlag(player, "-slash_count_unlimited");
				}
			}
		}else{
			if(player->hasFlag("massiveaxe_enabled")){
				DamageStruct damage = data.value<DamageStruct>();

				if(damage.card && (damage.card->inherits("Slash") || damage.card->inherits("Duel"))){
					damage.damage++;
					data = QVariant::fromValue(damage);
					room->setPlayerFlag(player, "-massiveaxe_enabled");

					room->broadcastSkillInvoked(player, objectName());
					room->sendLog("#TriggerSkill", player, objectName());
				}
			}
		}
	}
};

void StandardPackage::addGenerals()
{
	General *luffy = new General(this, "luffy$", "pirate", 4);
	luffy->addSkill(new Insulator);
	luffy->addSkill(new RubberPistol);
	luffy->addSkill(new RubberPistolEx);
	related_skills.insertMulti("rubberpistol", "#rubberpistol");
	addMetaObject<RubberPistolCard>();

	General *zoro = new General(this, "zoro", "pirate", 4);
	zoro->addSkill(new FretyWind);
	zoro->addSkill(new TripleSword);

	General *nami = new General(this, "nami", "pirate", 3, false);
	nami->addSkill(new Clima);
	nami->addSkill(new Forecast);
	nami->addSkill(new Mirage);
	addMetaObject<ForecastCard>();

	General *sanji = new General(this, "sanji", "pirate", 4);
	sanji->addSkill(new Gentleman);
	sanji->addSkill(new BlackFeet);

	General *usopp = new General(this, "usopp", "pirate", 3);
	usopp->addSkill(new Lie);
	usopp->addSkill(new Shoot);
	addMetaObject<LieCard>();

	General *buggy = new General(this, "buggy", "pirate", 3);
	buggy->addSkill(new Divided);
	buggy->addSkill(new CannonBall);
	addMetaObject<CannonBallCard>();

	General *arlong = new General(this, "arlong", "pirate", 4);
	arlong->addSkill(new SharkOnTooth);

	General *hatchan = new General(this, "hatchan", "pirate", 4);
	hatchan->addSkill(new Waltz);

	General *tashigi = new General(this, "tashigi", "government", 3, false);
	tashigi->addSkill(new SwordFan);
	tashigi->addSkill(new Myopia);
	addMetaObject<SwordFanCard>();

	General *smoker = new General(this, "smoker", "government", 3);
	smoker->addSkill(new FogBarrier);
	smoker->addSkill(new Justice);
	smoker->addSkill(new FogBarrierEffect);
	related_skills.insert("fogbarrier", "#fogbarriereffect");

	General *bellmere = new General(this, "bellmere", "government", 3, false);
	bellmere->addSkill(new Upright);
	bellmere->addSkill(new Protect);

	General *mihawk = new General(this, "mihawk", "government", 4);
	mihawk->addSkill(new TopSwordman);

	General *morgan = new General(this, "morgan", "government", 4);
	morgan->addSkill(new MassiveAxe);
}
