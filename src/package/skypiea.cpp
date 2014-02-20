#include "skypiea.h"
#include "carditem.h"

class GodThunder: public TriggerSkill{
public:
	GodThunder(): TriggerSkill("godthunder"){
		events << Damaged;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		Room *room = player->getRoom();

		DamageStruct damage = data.value<DamageStruct>();
		if(damage.nature == DamageStruct::Thunder){
			room->broadcastSkillInvoked(player, objectName());
			room->sendLog("#TriggerSkill", player, objectName());

			RecoverStruct recover;
			recover.card = damage.card;
			recover.from = damage.from;
			recover.to = damage.to;
			recover.recover = damage.damage;
			room->recover(recover);

			return true;
		}

		return false;
	}
};

class GodThunderEx: public TriggerSkill{
public:
	GodThunderEx(): TriggerSkill("#godthunderex"){
		events << Predamaged;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return true;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		Room *room = player->getRoom();

		DamageStruct damage = data.value<DamageStruct>();
		if(damage.card && damage.card->inherits("Lightning")){
			room->broadcastSkillInvoked(player, "godthunder");
			room->sendLog("#TriggerSkill", player, "godthunder");

			damage.from = room->findPlayerBySkillName("godthunder");
			data = QVariant::fromValue(damage);
		}

		return false;
	}
};

ThunderBotCard::ThunderBotCard(){
}

bool ThunderBotCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const{
	return targets.isEmpty();
}

bool ThunderBotCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
	return targets.length() == 1;
}

void ThunderBotCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
	room->throwCard(this, source);

	DamageStruct damage;
	damage.card = this;
	damage.nature = DamageStruct::Thunder;
	damage.from = source;
	damage.damage = source->getLostHp();

	foreach(damage.to, targets){
		room->damage(damage);
	}
}

class ThunderBotViewAsSkill: public OneCardViewAsSkill{
public:
	ThunderBotViewAsSkill(): OneCardViewAsSkill("thunderbot"){

	}

	bool viewFilter(const CardItem *to_select) const{
		return to_select->getCard()->getSuit() == Card::Spade;
	}

	bool isEnabledAtPlay(const Player *) const{
		return false;
	}

	bool isEnabledAtResponse(const Player *, const QString &pattern) const{
		return pattern == "@@thunderbot";
	}

	const Card *viewAs(CardItem *card_item) const{
		ThunderBotCard *card = new ThunderBotCard;
		card->addSubcard(card_item->getFilteredCard());
		card->setSkillName("thunderbot");
		return card;
	}
};

class ThunderBot: public TriggerSkill{
public:
	ThunderBot():TriggerSkill("thunderbot"){
		events << PhaseChange;
		view_as_skill = new ThunderBotViewAsSkill;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return TriggerSkill::triggerable(target) && target->getPhase() == Player::Finish;
	}

	virtual bool trigger(TriggerEvent, ServerPlayer *player, QVariant &) const{
		if(player->isWounded()){
			bool has_spade = false;
			foreach(const Card *card, player->getCards("he")){
				if(card->getSuit() == Card::Spade){
					has_spade = true;
					break;
				}
			}

			if(has_spade){
				Room *room = player->getRoom();
				room->askForUseCard(player, "@@thunderbot", objectName());
			}
		}

		return false;
	}
};

BallDragonCard::BallDragonCard(){
}

bool BallDragonCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const{
	return targets.isEmpty();
}

bool BallDragonCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
	return targets.length() == 1;
}

void BallDragonCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
	foreach(ServerPlayer *target, targets){
		target->setChained(true);
		room->broadcastProperty(target, "chained");
		room->setEmotion(target, "chain");
	}
}

class BallDragonViewAsSkill: public ZeroCardViewAsSkill{
public:
	BallDragonViewAsSkill(): ZeroCardViewAsSkill("balldragon"){

	}

	bool isEnabledAtPlay(const Player *) const{
		return false;
	}

	bool isEnabledAtResponse(const Player *, const QString &pattern) const{
		return pattern == "@@balldragon";
	}

	const Card *viewAs() const{
		return new BallDragonCard;
	}
};

class BallDragon: public TriggerSkill{
public:
	BallDragon(): TriggerSkill("balldragon"){
		events << TargetSelect;
		view_as_skill = new BallDragonViewAsSkill;
	}

	bool trigger(TriggerEvent, ServerPlayer *player, QVariant &data) const{
		CardUseStruct use = data.value<CardUseStruct>();
		if(use.card && use.card->getTypeId() == Card::Trick){
			Room *room = player->getRoom();
			room->askForUseCard(player, "@@balldragon", objectName());
		}

		return false;
	}
};

class SurpriseBall: public TriggerSkill{
public:
	SurpriseBall(): TriggerSkill("surpriseball"){
		events << CardEffect << Damaged;
	}

	bool triggerable(const ServerPlayer *target) const{
		return target && target->isAlive() && target->isChained();
	}

	bool trigger(TriggerEvent event, ServerPlayer *target, QVariant &data) const{
		if(event == CardEffect){
			CardEffectStruct effect = data.value<CardEffectStruct>();
			if(effect.card && effect.card->inherits("Jink") && effect.card->getSuit() == Card::Diamond){
				Room *room = target->getRoom();
				ServerPlayer *player = room->findPlayerBySkillName(objectName());
				if(player){
					room->broadcastSkillInvoked(player, objectName());
					room->sendLog("#TriggerSkill", player, objectName());
					return true;
				}
			}
		}else{
			DamageStruct damage = data.value<DamageStruct>();
			if(damage.nature == DamageStruct::Normal){
				Room *room = target->getRoom();
				ServerPlayer *player = room->findPlayerBySkillName(objectName());
				if(player){
					room->broadcastSkillInvoked(player, objectName());
					room->sendLog("#TriggerSkill", player, objectName());

					damage.nature = DamageStruct::Fire;
					data = QVariant::fromValue(damage);
				}
			}
		}

		return false;
	}
};

SkypieaPackage::SkypieaPackage()
	:Package("Skypiea")
{
	General *enil = new General(this, "enil", "citizen", 3);
	enil->addSkill(new GodThunder);
	enil->addSkill(new GodThunderEx);
	related_skills.insertMulti("godthunder", "#godthunderex");
	enil->addSkill(new ThunderBot);
	addMetaObject<ThunderBotCard>();

	General *satori = new General(this, "satori", "citizen", 3);
	satori->addSkill(new BallDragon);
	addMetaObject<BallDragonCard>();
	satori->addSkill(new SurpriseBall);
}

ADD_PACKAGE(Skypiea)
