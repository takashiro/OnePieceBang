#include "summitwar.h"
#include "carditem.h"

MeteorVolcanoCard::MeteorVolcanoCard(){
	target_fixed = true;
}

void MeteorVolcanoCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const{
	FireSlash *slash = new FireSlash(Card::NoSuit, 0);
	foreach(int subcard, subcards){
		slash->addSubcard(subcard);
	}
	slash->setSkillName("volcano");

	CardUseStruct use;
	use.from = source;
	use.to = room->getOtherPlayers(source);
	use.card = slash;

	room->useCard(use, false);
}

class MeteorVolcano: public ViewAsSkill{
public:
	MeteorVolcano(): ViewAsSkill("meteorvolcano"){

	}

	virtual bool isEnabledAtPlay(const Player *player) const{
		return !player->hasUsed("MeteorVolcanoCard");
	}

	virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
		return selected.length() < 2;
	}

	virtual const Card *viewAs(const QList<CardItem *> &cards) const{
		if(cards.length() != 2){
			return NULL;
		}

		MeteorVolcanoCard *card = new MeteorVolcanoCard;
		card->addSubcards(cards);
		card->setSkillName("volcano");
		return card;
	}
};

class Revive: public TriggerSkill{
public:
	Revive(): TriggerSkill("revive"){
		events << AskForWine;
	}

	bool trigger(TriggerEvent, ServerPlayer *player, QVariant &data) const{
		DyingStruct dying = data.value<DyingStruct>();
		if(dying.who == player){
			if(player->askForSkillInvoke(objectName(), data)){
				Room *room = player->getRoom();

				int max_hp = player->getMaxHp() - 1;
				room->setPlayerProperty(player, "maxhp", max_hp);

				if(max_hp > 0){
					RecoverStruct recover;
					recover.from = player;
					recover.to = player;
					recover.recover = max_hp - player->getHp();
					room->recover(recover);
				}
			}
		}

		return false;
	}
};

SummitWarPackage::SummitWarPackage():Package("SummitWar")
{
	General *sakazuki = new General(this, "sakazuki", "government", 4);
	sakazuki->addSkill(new MeteorVolcano);
	addMetaObject<MeteorVolcanoCard>();

	General *marco = new General(this, "marco", "pirate", 3);
	marco->addSkill(new Revive);
}

ADD_PACKAGE(SummitWar)
