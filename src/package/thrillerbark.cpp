#include "thrillerbark.h"
#include "carditem.h"
#include "engine.h"
#include "client.h"

/*class MusicSoul: public TriggerSkill{
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
};*/

AcheronCard::AcheronCard(){

}

bool AcheronCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const{
	return targets.isEmpty();
}

bool AcheronCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
	return targets.length() == 1;
}

void AcheronCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
	ServerPlayer *target = targets.first();
	target->tag["acheron_player"] = source->objectName();
	room->acquireSkill(target, "#acheron_revive");
}

class AcheronViewAsSkill: public ZeroCardViewAsSkill{
public:
	AcheronViewAsSkill(): ZeroCardViewAsSkill("acheron"){

	}

	bool isEnabledAtPlay(const Player *) const{
		return false;
	}

	bool isEnabledAtResponse(const Player *, const QString &pattern) const{
		return pattern == "@@acheron";
	}

	const Card *viewAs() const{
		return new AcheronCard;
	}
};

class Acheron: public TriggerSkill{
public:
	Acheron(): TriggerSkill("acheron"){
		events << Death;
		frequency = Limited;
		view_as_skill = new AcheronViewAsSkill;
	}

	bool triggerable(const ServerPlayer *target) const{
		return target->hasSkill(objectName()) && target->getMark("@acheron") > 0;
	}

	bool trigger(TriggerEvent, ServerPlayer *player, QVariant &data) const{
		Room *room = player->getRoom();
		room->askForUseCard(player, "@@acheron", objectName());
		return false;
	}
};

class AcheronRevive: public PhaseChangeSkill{
public:
	AcheronRevive(): PhaseChangeSkill("#acheron_revive"){

	}

	bool onPhaseChange(ServerPlayer *target) const{
		if(target->getPhase() == Player::Finish){
			Room *room = target->getRoom();

			ServerPlayer *player = room->findChild<ServerPlayer *>(target->tag.value("acheron_player").toString());
			if(player){
				room->setPlayerProperty(player, "hp", player->getMaxHp());
				room->revivePlayer(player);
				player->drawCards(3, "acheron");
			}

			room->detachSkillFromPlayer(target, objectName());
			target->tag.remove("acheron_player");
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
					room->broadcastSkillInvoked(player, objectName());
					room->sendLog("#TriggerSkill", player, objectName());
					player->drawCards(1);
				}
			}else{
				foreach(ServerPlayer *player, room->findPlayersBySkillName(objectName())){
					room->broadcastSkillInvoked(player, objectName());
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

class ShadowAsgard: public TriggerSkill{
public:
	ShadowAsgard(): TriggerSkill("shadowasgard"){
		events << CardDiscarded;
	}

	bool triggerable(const ServerPlayer *target) const{
		return target != NULL;
	}

	bool trigger(TriggerEvent event, ServerPlayer *who, QVariant &data) const{
		Room *room = who->getRoom();
		const Card *discarded = data.value<CardStar>();
		if(discarded == NULL){
			return false;
		}

		if(discarded->isVirtualCard()){
			foreach(int card_id, discarded->getSubcards()){
				const Card *card = Bang->getCard(card_id);
				if(!card || !card->getSuit() == Card::Spade){
					continue;
				}

				foreach(ServerPlayer *player, room->getOtherPlayers(who)){
					if(player != who && player->hasSkill(objectName())){
						room->broadcastSkillInvoked(player, objectName());
						room->sendLog("#TriggerSkill", player, objectName());

						player->obtainCard(card);
					}
				}
			}
		}else if(discarded->getSuit() == Card::Spade){
			foreach(ServerPlayer *player, room->getOtherPlayers(who)){
				if(player != who && player->hasSkill(objectName())){
					room->broadcastSkillInvoked(player, objectName());
					room->sendLog("#TriggerSkill", player, objectName());

					player->obtainCard(discarded);
				}
			}
		}

		return false;
	}
};

class Nightmare: public PhaseChangeSkill{
public:
	Nightmare(): PhaseChangeSkill("nightmare"){
	}

	bool onPhaseChange(ServerPlayer *target) const{
		Room *room = target->getRoom();
		if(target->getPhase() == Player::Start){
			if(target->getHandcardNum() >= target->getHp() * 2){
				room->sendLog("#TriggerSkill", target, objectName());

				target->throwAllHandCards();
				DamageStruct damage;
				damage.from = target;
				foreach(ServerPlayer *victim, room->getOtherPlayers(target)){
					damage.damage = 1;
					damage.to = victim;
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
	//brook->addSkill(new MusicSoul);
	brook->addSkill(new Acheron);
	brook->addSkill(new MarkAssignSkill("@acheron"));
	addMetaObject<AcheronCard>();
	skills << new AcheronRevive;

	General *perona = new General(this, "perona", "pirate", 3, false);
	perona->addSkill(new NegativeHorror);
	perona->addSkill(new GhostRap);

	General *moriah = new General(this, "moriah", "pirate", 3);
	moriah->addSkill(new ShadowAsgard);
	moriah->addSkill(new Nightmare);
}

ADD_PACKAGE(ThrillerBark)
