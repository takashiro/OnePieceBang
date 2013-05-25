#include "fishmanisland.h"
#include "engine.h"

class NeptunianAttackAvoid: public TriggerSkill{
public:
	NeptunianAttackAvoid(const QString &avoid_skill)
		:TriggerSkill("#na_avoid_" + avoid_skill), avoid_skill(avoid_skill)
	{
		events << CardEffected;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		CardEffectStruct effect = data.value<CardEffectStruct>();
		if(effect.card->inherits("NeptunianAttack")){
			LogMessage log;
			log.type = "#SkillNullify";
			log.from = player;
			log.arg = avoid_skill;
			log.arg2 = "neptunian_attack";
			player->getRoom()->sendLog(log);

			return true;
		}else
			return false;
	}

private:
	QString avoid_skill;
};

class SeaQueen: public TriggerSkill{
public:
	SeaQueen():TriggerSkill("seaqueen"){
		events << CardFinished;
		frequency = Compulsory;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return !target->hasSkill(objectName());
	}

	virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
		CardUseStruct use = data.value<CardUseStruct>();
		if(use.card->inherits("NeptunianAttack") &&
				((!use.card->isVirtualCard()) ||
				  (use.card->getSubcards().length() == 1 &&
				  Bang->getCard(use.card->getSubcards().first())->inherits("NeptunianAttack")))){
			Room *room = player->getRoom();
			if(room->getCardPlace(use.card->getEffectiveId()) == Player::DiscardPile){
				QList<ServerPlayer *> players = room->getAllPlayers();
				foreach(ServerPlayer *p, players){
					if(p->hasSkill(objectName())){
						p->obtainCard(use.card);
						room->playSkillEffect(objectName());
						break;
					}
				}
			}
		}

		return false;
	}
};

FishmanIslandPackage::FishmanIslandPackage():Package("FishmanIsland")
{
	General *shirahoshi = new General(this, "shirahoshi", "noble", 3, false);
	shirahoshi->addSkill(new SeaQueen);
	shirahoshi->addSkill(new NeptunianAttackAvoid("seaqueen"));
	related_skills.insertMulti("seaqueen", "#na_avoid_seaqueen");
}

ADD_PACKAGE(FishmanIsland)
