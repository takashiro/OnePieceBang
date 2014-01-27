#include "skypiea.h"


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
		events << Damaged;
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

class ThunderBot: public TriggerSkill{
public:
	ThunderBot():TriggerSkill("thunderbot"){
		events << PhaseChange;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return TriggerSkill::triggerable(target) && target->getPhase() == Player::Finish;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		if(!player->askForSkillInvoke(objectName())){
			return false;
		}

		Room *room = player->getRoom();
		const Card *card = room->askForCard(player, ".S", "thunderbot-invoke", QVariant(), NonTrigger);

		if(card){
			room->throwCard(card, player);

			DamageStruct damage;
			damage.from = player;
			damage.to = room->askForPlayerChosen(player, room->getAlivePlayers(), "thunderbot-invoke");
			damage.damage = player->getLostHp();
			damage.nature = DamageStruct::Thunder;
			room->damage(damage);
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
}

ADD_PACKAGE(Skypiea)
