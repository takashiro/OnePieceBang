#include "test.h"

OperatingRoomCard::OperatingRoomCard(){
	target_fixed = true;
}

void OperatingRoomCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const{
	QList<int> cards;
	cards.append(subcards.first());
	foreach(ServerPlayer *target, room->getOtherPlayers(source)){
		int card_id = room->askForCardChosen(source, target, "hej", "operatingroom");
		if(card_id){
			cards.append(card_id);
			room->obtainCard(source, card_id);
		}
	}

	foreach(ServerPlayer *target, room->getOtherPlayers(source)){
		if(cards.empty()){
			break;
		}

		room->fillAG(cards, source);
		int card_id = room->askForAG(source, cards, false, "operatingroom");
		cards.removeOne(card_id);
		room->obtainCard(target, card_id);
		room->broadcastInvoke("clearAG");
	}
}

class OperatingRoom: public ZeroCardViewAsSkill{
public:
	OperatingRoom(): ZeroCardViewAsSkill("operatingroom"){

	}

	virtual bool isEnabledAtPlay(const Player *player) const{
		return !player->hasUsed("OperatingRoomCard");
	}

	virtual const Card *viewAs() const{
		return new OperatingRoomCard;
	}
};

class DarkWater: public TriggerSkill{
public:
	DarkWater(): TriggerSkill("darkwater"){
		events << DamagedProceed << TargetSelected;
		frequency = Compulsory;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		if(event == DamagedProceed){
			DamageStruct damage = data.value<DamageStruct>();
			if(damage.nature == DamageStruct::Normal){
				player->getRoom()->sendLog("#TriggerSkill", player, objectName());
				damage.damage++;
				data = QVariant::fromValue(damage);
			}
		}else{
			CardUseStruct use = data.value<CardUseStruct>();
			if(use.card->isRed() && player->isWounded() && player->askForSkillInvoke(objectName())){
				RecoverStruct recover;
				recover.card = use.card;
				recover.from = player;
				recover.to = player;
				player->getRoom()->recover(recover);
			}
		}
		return false;
	}
};

class BlackHole: public TriggerSkill{
public:
	BlackHole(): TriggerSkill("blackhole"){
		events << CardEffect;
		frequency = Compulsory;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		Room *room = target->getRoom();
		foreach(ServerPlayer *player, room->findPlayersBySkillName("darkwater")){
			if(player->getPhase() != Player::NotActive){
				return true;
			}
		}
		return false;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *target, QVariant &data) const{
		Room *room = target->getRoom();

		const Card *card = NULL;
		if(event == CardEffect){
			CardEffectStruct effect = data.value<CardEffectStruct>();
			card = effect.card;

			if(card && card->isRed() && card->getTypeId() == Card::Basic){
				ServerPlayer *player = room->findPlayerBySkillName("blackhole");
				room->sendLog("#TriggerSkill", player, "blackhole");
				return true;
			}
		}

		return false;
	}
};

TestPackage::TestPackage():Package("Test")
{
	General *law = new General(this, "law", "pirate", 4);
	law->addSkill(new OperatingRoom);
	addMetaObject<OperatingRoomCard>();

	General *blackbear = new General(this, "blackbear", "pirate", 3);
	blackbear->addSkill(new DarkWater);
	blackbear->addSkill(new BlackHole);
}

ADD_PACKAGE(Test)
