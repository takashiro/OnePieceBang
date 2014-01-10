#include "thrillerbark.h"
#include "carditem.h"
#include "engine.h"
#include "client.h"

class MusicSoul: public TriggerSkill{
public:
	MusicSoul(): TriggerSkill("musicsoul"){
		events << CardResponsed;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return target->getCardCount(true) > 0;
	}

	virtual bool trigger(TriggerEvent, ServerPlayer *target, QVariant &data) const{
		CardStar card = data.value<CardStar>();
		if(!card->inherits("Jink")){
			return false;
		}

		Room *room = target->getRoom();

		foreach(ServerPlayer *player, room->findPlayersBySkillName(objectName())){
			if(player == target){
				continue;
			}

			if(player->askForSkillInvoke(objectName(), data)){
				int card_id = room->askForCardChosen(target, player, "he", objectName());
				if(card_id){
					room->throwCard(card_id, player);
				}

				if(player->getCardCount(true) > 0){
					card_id = room->askForCardChosen(player, target, "he", objectName());
					if(card_id){
						room->throwCard(card_id, target);
					}
				}
			}
		}
	}
};

class Acheron: public TriggerSkill{
public:
	Acheron(): TriggerSkill("acheron"){
		events << Dying;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		if(!player->askForSkillInvoke(objectName())){
			return false;
		}

		Room *room = player->getRoom();

		QList<int> souls = player->getPile("soul");
		QString soul_pattern;
		if(souls.isEmpty()){
			soul_pattern = "n";
		}else{
			const Card *soul = Bang->getCard(souls.at(0));
			soul_pattern.append(QString::number(soul->getNumber()));
			for(int i = 1; i < souls.length(); i++){
				soul_pattern.append('|');
				soul = Bang->getCard(souls.at(i));
				soul_pattern.append(QString::number(soul->getNumber()));
			}
		}

		qDebug() << QString("(.*):(.*):(%1)").arg(soul_pattern);

		JudgeStruct judge;
		judge.good = false;
		judge.pattern = QRegExp(QString("(.*):(.*):(%1)").arg(soul_pattern));
		judge.who = player;
		judge.reason = objectName();
		room->judge(judge);

		if(judge.isGood()){
			RecoverStruct recover;
			recover.who = player;
			recover.recover = 1 - player->getHp();
			room->recover(player, recover);

			player->addToPile("soul", judge.card);
		}

		return false;
	}
};

class NegativeHorror: public OneCardViewAsSkill{
public:
	NegativeHorror(): OneCardViewAsSkill("negativehorror"){

	}

	virtual bool isEnabledAtPlay(const Player *player) const{
		return player->getCardCount(true) > 0;
	}

	virtual bool viewFilter(const CardItem *to_select) const{
		const Card *card = to_select->getCard();
		return card->getSuit() == Card::Club && !card->inherits("DelayedTrick");
	}

	virtual const Card *viewAs(CardItem *card_item) const{
		const Card *sub = card_item->getCard();
		NegativeSoul *negative_soul = new NegativeSoul(sub->getSuit(), sub->getNumber());
		negative_soul->addSubcard(sub);
		negative_soul->setSkillName(objectName());
		return negative_soul;
	}
};

class GhostRap: public TriggerSkill{
public:
	GhostRap(): TriggerSkill("ghostrap"){
		events << FinishJudge;
	}

	virtual bool triggerable(const ServerPlayer *) const{
		return true;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *target, QVariant &data) const{
		if(target == NULL){
			return false;
		}

		JudgeStar judge = data.value<JudgeStar>();
		if(judge->reason == "negative_soul"){
			Room *room = target->getRoom();

			if(judge->isBad()){//effect
				foreach(ServerPlayer *player, room->findPlayersBySkillName(objectName())){
					room->sendLog("#TriggerSkill", player, objectName());
					player->drawCards(1);
				}
			}else{
				foreach(ServerPlayer *player, room->findPlayersBySkillName(objectName())){
					room->sendLog("#TriggerSkill", player, objectName());

					DamageStruct damage;
					damage.from = player;
					damage.to = judge->who;
					room->damage(damage);
				}
			}
		}

		return false;
	}
};

ThrillerBarkPackage::ThrillerBarkPackage():Package("ThrillerBark")
{
	General *brook = new General(this, "brook", "pirate", 3);
	brook->addSkill(new Acheron);
	brook->addSkill(new MusicSoul);

	General *perona = new General(this, "perona", "pirate", 3, false);
	perona->addSkill(new NegativeHorror);
	perona->addSkill(new GhostRap);
}

ADD_PACKAGE(ThrillerBark)
