#include "alabastan.h"
#include "skill.h"
#include "carditem.h"
#include "engine.h"
#include "client.h"

class FirePunch: public OneCardViewAsSkill{
public:
	FirePunch(): OneCardViewAsSkill("firepunch"){

	}

	virtual bool viewFilter(const CardItem *to_select) const{
		const Card *card = to_select->getFilteredCard();
		return card->getSuit() == Card::Spade;
	}

	virtual bool isEnabledAtPlay(const Player *player) const{
		return Slash::IsAvailable(player);
	}

	virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
		return pattern == "slash" || pattern == "fireslash";
	}

	virtual const Card *viewAs(CardItem *card_item) const{
		const Card *sub = card_item->getCard();
		FireSlash *slash = new FireSlash(sub->getSuit(), sub->getNumber());
		slash->setSkillName(objectName());
		slash->addSubcard(sub);
		return slash;
	}
};

class FirePunchEx: public PropertySkill{
public:
	FirePunchEx(): PropertySkill("#firepunchex"){

	}

	virtual QVariant getCorrect(const Player *player, const Card *card, const QString &property) const{
		int correct = 0;

		if(property == "slash_extra_target" && card->inherits("FireSlash") && card->getSkillName() == "firepunch"){
			correct++;
		}

		return correct;
	}
};

class AntiWar: public TriggerSkill{
public:
	AntiWar(): TriggerSkill("antiwar"){
		events << Damage;
	}

	virtual bool triggerable(const ServerPlayer *) const{
		return true;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *target, QVariant &data) const{
		DamageStruct damage = data.value<DamageStruct>();
		if(damage.card == NULL || (!damage.card->inherits("Slash") && !damage.card->inherits("Duel")) || damage.from == NULL){
			return false;
		}

		const Card *weapon = damage.from->getWeapon();
		if(weapon == NULL){
			return false;
		}

		Room *room = target->getRoom();
		foreach(ServerPlayer *player, room->findPlayersBySkillName(objectName())){
			if(player->isKongcheng()){
				continue;
			}

			QString prompt = QString("antiwar-invoke:").append(damage.from->getGeneralName());
			const Card *card = room->askForCard(player, ".", prompt, data, NonTrigger);
			if(card == NULL){
				continue;
			}
			room->sendLog("#InvokeSkill", player, objectName());
			room->throwCard(card);
			room->throwCard(weapon, damage.from);
		}

		return false;
	}
};


class Alliance: public TriggerSkill{
public:
	Alliance(): TriggerSkill("alliance"){
		events << CardDrawnDone;
		frequency = Frequent;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return target->getPhase() == Player::Play;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *target, QVariant &data) const{
		Room *room = target->getRoom();
		foreach(ServerPlayer *player, room->getOtherPlayers(target)){
			if(player->hasSkill(objectName()) && player->faceUp() && player->askForSkillInvoke(objectName(), data)){
				player->drawCards(1);
			}
		}

		return false;
	}
};

FleurCard::FleurCard(){

}

bool FleurCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
	return targets.length() < 2 && !to_select->isKongcheng();
}

bool FleurCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
	return targets.length() == 2;
}

void FleurCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
	room->throwCard(this);

	if(targets.length() < 2 || targets.at(0)->isKongcheng() || targets.at(1)->isKongcheng()){
		return;
	}

	const Card *card1 = room->askForCardShow(targets.at(0), source, "fleur");
	const Card *card2 = room->askForCardShow(targets.at(1), source, "fleur");

	room->showCard(targets.at(0), card1->getId());
	room->showCard(targets.at(1), card2->getId());

	if(card1->getColor() == card2->getColor()){
		int card_id;
		LogMessage log;
		log.type = "$Dismantlement";

		card_id = room->askForCardChosen(source, targets.at(0), "he", "fleur");
		room->throwCard(card_id);
		log.from = targets.at(0);
		log.card_str = QString::number(card_id);
		room->sendLog(log);

		card_id = room->askForCardChosen(source, targets.at(1), "he", "fleur");
		room->throwCard(card_id);
		log.from = targets.at(1);
		log.card_str = QString::number(card_id);
		room->sendLog(log);
	}else{
		Duel *duel = new Duel(Card::NoSuit, 0);
		duel->setSkillName("fleur");
		CardUseStruct use;
		use.card = duel;

		if(card1->isBlack()){
			use.from = targets.at(0);
			use.to.append(targets.at(1));
		}else{
			use.from = targets.at(1);
			use.to.append(targets.at(0));
		}
		room->useCard(use, true);
	}
}

class Fleur: public OneCardViewAsSkill{
public:
	Fleur(): OneCardViewAsSkill("fleur"){

	}

	virtual bool isEnabledAtPlay(const Player *player) const{
		return !player->hasUsed("FleurCard");
	}

	virtual bool viewFilter(const CardItem *) const{
		return true;
	}

	virtual const Card *viewAs(CardItem *card_item) const{
		FleurCard *card = new FleurCard;
		card->addSubcard(card_item->getCard());
		return card;
	}
};

class Survivor: public TriggerSkill{
public:
	Survivor(): TriggerSkill("survivor"){
		events << CardEffected;
		frequency = Compulsory;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		CardEffectStruct effect = data.value<CardEffectStruct>();
		if(effect.card != NULL && (effect.card->inherits("NeptunianAttack") || effect.card->inherits("BusterCall"))){
			player->getRoom()->sendLog("#TriggerSkill", player, objectName());
			return true;
		}
		return false;
	}
};

class Leechcraft: public OneCardViewAsSkill{
public:
	Leechcraft(): OneCardViewAsSkill("leechcraft"){

	}

	virtual bool isEnabledAtPlay(const Player *player) const{
		return false;
	}

	virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
		return  pattern.contains("wine") && player->getPhase() == Player::NotActive;
	}

	virtual bool viewFilter(const CardItem *to_select) const{
		return to_select->getFilteredCard()->isRed();
	}

	virtual const Card *viewAs(CardItem *card_item) const{
		const Card *first = card_item->getCard();
		Wine *wine = new Wine(first->getSuit(), first->getNumber());
		wine->addSubcard(first->getId());
		wine->setSkillName(objectName());
		return wine;
	}
};

class RumbleBall: public TriggerSkill{
public:
	RumbleBall(): TriggerSkill("rumbleball"){
		events << Predamage << CardEffect;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return TriggerSkill::triggerable(target) && target->getHp() == 1;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		Room *room = player->getRoom();

		if(event == Predamage){
			room->sendLog("#TriggerSkill", player, objectName());

			DamageStruct damage = data.value<DamageStruct>();
			damage.damage++;
			data = QVariant::fromValue(damage);
		}else{
			CardEffectStruct effect = data.value<CardEffectStruct>();
			if(effect.card->inherits("Wine")){
				room->sendLog("#TriggerSkill", player, objectName());

				RecoverStruct recover;
				recover.card = effect.card;
				recover.who = player;
				room->recover(effect.to, recover);
			}
		}

		return false;
	}
};

class Imitate: public TriggerSkill{
public:
	Imitate(): TriggerSkill("imitate"){
		events << PhaseChange;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return TriggerSkill::triggerable(target) && target->getPhase() == Player::Start;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		if(!player->askForSkillInvoke(objectName())){
			return false;
		}

		Room *room = player->getRoom();

		if(player->tag.contains("imitated_skill_name")){
			room->detachSkillFromPlayer(player, player->tag.value("imitated_skill_name").toString());
		}

		ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName());
		if(target != NULL){
			QStringList skills;
			foreach(const Skill *skill, target->getVisibleSkillList()){
				skills.append(skill->objectName());
			}

			if(!skills.isEmpty()){
				QString skill;
				if(skills.length() > 1){
					skill = room->askForChoice(player, objectName(), skills.join("+"));
				}else{
					skill = skills.at(0);
				}
				room->acquireSkill(player, skill);
				player->tag["imitated_skill_name"] = skill;

				room->setPlayerProperty(player, "kingdom", target->getKingdom());
			}
		}

		return false;
	}
};

class Memoir: public TriggerSkill{
public:
	Memoir(): TriggerSkill("memoir"){
		events << CardLostOneTime;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return target->isKongcheng() && target->isAlive();
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *target, QVariant &data) const{
		Room *room = target->getRoom();
		foreach(ServerPlayer *player, room->getOtherPlayers(target)){
			if(player->hasSkill(objectName()) && player->askForSkillInvoke(objectName(), data)){
				foreach(int card_id, player->handCards()){
					room->obtainCard(target, card_id, false);
				}

				player->drawCards(1, true, objectName());
				if(player->tag.contains("imitated_skill_name")){
					room->detachSkillFromPlayer(player, player->tag.value("imitated_skill_name").toString());
					player->tag.remove("imitated_skill_name");
				}
			}
		}

		return false;
	}
};

class Corrasion: public TriggerSkill{
public:
	Corrasion(): TriggerSkill("corrasion"){
		events << Predamaged;
		frequency = Compulsory;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		DamageStruct damage = data.value<DamageStruct>();
		if(damage.from == NULL || damage.damage != 1 || damage.card == NULL || !damage.card->inherits("Slash")){
			return false;
		}

		Room *room = player->getRoom();

		room->sendLog("#TriggerSkill", player, objectName());

		QString prompt = "corrasion-effect:" + player->getGeneralName();
		const Card *card = room->askForCard(damage.from, ".S", prompt, QVariant(), NonTrigger);
		if(card != NULL){
			room->obtainCard(player, card);
		}else{
			return true;
		}

		return false;
	}
};

class SandStorm: public TriggerSkill{
public:
	SandStorm(): TriggerSkill("sandstorm"){
		events << CardDiscarded << PhaseChange;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		if(event == CardDiscarded){
			const Card *card = data.value<CardStar>();

			if(card == NULL){
				return false;
			}

			bool spade = false;
			if(card->subcardsLength() > 0){
				foreach(int card_id, card->getSubcards()){
					if(Bang->getCard(card_id)->getSuit() == Card::Spade){
						spade = true;
					}
					break;
				}
			}else{
				spade = card->getSuit() == Card::Spade;
			}

			player->tag["SandStormEnabled"] = spade;

		}else if(player->tag.value("SandStormEnabled").toBool() && player->askForSkillInvoke(objectName(), data)){
			Room *room = player->getRoom();
			foreach(ServerPlayer *target, room->getAlivePlayers()){
				if(!target->isAlive()){
					continue;
				}

				const Card *discard_card = NULL;
				QString prompt = "sandstorm-effected:" + player->getGeneralName();
				if(target->isKongcheng() || (discard_card = room->askForCard(target, ".S", prompt, QVariant(), NonTrigger)) == NULL){
					room->loseHp(target, 1);
				}else if(discard_card != NULL){
					room->throwCard(discard_card, target);
				}
			}
		}

		return false;
	}
};

class Quack: public TriggerSkill{
public:
	Quack(): TriggerSkill("quack"){
		 events << HpRecover;
		 frequency = Compulsory;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return true;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *target, QVariant &data) const{
		RecoverStruct recover = data.value<RecoverStruct>();
		Room *room = target->getRoom();

		if(recover.who != NULL && recover.who->hasSkill(objectName())){
			room->sendLog("#TriggerSkill", recover.who, objectName());
			for(int i = 0; i < recover.recover; i++){
				target->turnOver();
				target->drawCards(recover.who->getLostHp());
			}
		}
		return false;
	}
};

class WinterSakura: public TriggerSkill{
public:
	WinterSakura(): TriggerSkill("wintersakura"){
		events << Dying;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		if(target == NULL){
			return false;
		}

		Room *room = target->getRoom();
		foreach(ServerPlayer *player, room->getAlivePlayers()){
			if(player->hasSkill(objectName()) && !player->isKongcheng()){
				return true;
			}
		}

		return false;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *target, QVariant &data) const{
		Room *room = target->getRoom();

		foreach(ServerPlayer *player, room->findPlayersBySkillName(objectName())){
			if(!player->askForSkillInvoke(objectName(), data)){
				continue;
			}

			room->showAllCards(player);

			RecoverStruct recover;
			recover.who = player;

			foreach(const Card *card, player->getHandcards()){
				if(card->getSuit() == Card::Club){
					Wine *wine = new Wine(card->getSuit(), card->getNumber());
					wine->addSubcard(card);
					wine->setSkillName(objectName());

					CardUseStruct use;
					use.from = player;
					use.to << target;
					use.card = wine;
					room->useCard(use, true);
				}
			}
		}

		return false;
	}
};

CandleWaxCard::CandleWaxCard(){
	will_throw = false;
}

bool CandleWaxCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
	return !Bang->getCard(subcards.at(0))->targetFixed() && Bang->getCard(subcards.at(0))->targetFilter(targets, to_select, Self);
}

bool CandleWaxCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
	return Bang->getCard(subcards.at(0))->targetsFeasible(targets, Self);
}

void CandleWaxCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
	room->showCard(source, subcards.at(0));

	const Card *trick = Bang->getCard(subcards.at(0));
	const Card *sub = Bang->getCard(subcards.at(1));

	const QMetaObject *meta = trick->metaObject();
	QObject *card_obj = meta->newInstance(Q_ARG(Card::Suit, sub->getSuit()), Q_ARG(int, sub->getNumber()));

	if(card_obj){
		Card *card = qobject_cast<Card *>(card_obj);
		card->setObjectName(trick->objectName());
		card->addSubcard(sub);
		card->setSkillName("candlewax");

		CardUseStruct use;
		use.from = source;
		use.to = targets;
		use.card = card;
		room->useCard(use);
	}
}

class CandleWax: public ViewAsSkill{
public:
	CandleWax():ViewAsSkill("candlewax"){

	}

	virtual bool isEnabledAtPlay(const Player *player) const{
		return true;
	}

	virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
		if(selected.length() == 0){
			return to_select->getCard()->isNDTrick();
		}else if(selected.length() == 1){
			return to_select->getCard()->isRed();
		}

		return false;
	}

	virtual const Card *viewAs(const QList<CardItem *> &cards) const{
		if(cards.length() == 2){
			CandleWaxCard *card = new CandleWaxCard;
			card->addSubcards(cards);
			card->setSkillName(objectName());
			const Card *sub = cards.at(1)->getCard();
			card->setSuit(sub->getSuit());
			card->setNumber(sub->getNumber());
			return card;
		}

		return NULL;
	}
};

class MedicalExpertise: public TriggerSkill{
public:
	MedicalExpertise(): TriggerSkill("medicalexpertise"){
		events << Dying;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		if(target == NULL){
			return false;
		}

		Room *room = target->getRoom();
		foreach(ServerPlayer *player, room->getAlivePlayers()){
			if(player->hasSkill(objectName()) && !player->isKongcheng()){
				return true;
			}
		}

		return false;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *target, QVariant &data) const{
		Room *room = target->getRoom();

		foreach(ServerPlayer *player, room->findPlayersBySkillName(objectName())){
			DyingStruct dying = data.value<DyingStruct>();
			if(dying.who->isKongcheng() || !player->askForSkillInvoke(objectName(), data)){
				return false;
			}

		room->showAllCards(dying.who);

			static RecoverStruct recover;
			recover.who = player;
		recover.recover = 1;
			foreach(const Card *card, dying.who->getHandcards()){
				if(card->isRed()){
		room->throwCard(card, dying.who);
		room->recover(dying.who, recover);
		}
			}

		int card_id = room->askForCardChosen(player, dying.who, "he", objectName());
		if(card_id > 0){
		room->obtainCard(player, card_id);
		}
		}

		return false;
	}
};

class WitheredFlower: public TriggerSkill{
public:
	WitheredFlower(): TriggerSkill("witheredflower"){
		events << TargetSelected;
		frequency = Frequent;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		CardUseStruct use = data.value<CardUseStruct>();

		if(!use.card || use.card->getSuit() != Card::Club || !player->askForSkillInvoke(objectName())){
			return false;
		}

		Room *room = player->getRoom();

		bool draw_card = false;

		if(use.from->isKongcheng())
			draw_card = true;
		else{
			QString prompt = "witheredflower-discard:" + player->getGeneralName();
			const Card *card = room->askForCard(use.from, ".", prompt, QVariant(), CardDiscarded);
			if(card){
				room->throwCard(card);
			}else
				draw_card = true;
		}

		if(draw_card)
			player->drawCards(1);

		return false;
	}
};

AlabastanPackage::AlabastanPackage():Package("Alabastan")
{
	General *ace = new General(this, "ace", "pirate", 4);
	ace->addSkill(new FirePunch);
	ace->addSkill(new FirePunchEx);

	General *vivi = new General(this, "vivi", "citizen", 3, false);
	vivi->addSkill(new AntiWar);
	vivi->addSkill(new Alliance);

	General *robin = new General(this, "robin", "pirate", 3, false);
	robin->addSkill(new Fleur);
	robin->addSkill(new Survivor);
	addMetaObject<FleurCard>();

	General *chopper = new General(this, "chopper", "pirate", 3);
	chopper->addSkill(new Leechcraft);
	chopper->addSkill(new RumbleBall);

	General *bonkure = new General(this, "bonkure", "pirate", 3);
	bonkure->addSkill(new Imitate);
	bonkure->addSkill(new Memoir);
	bonkure->addSkill(new Skill("okama", Skill::Compulsory));

	General *crocodile = new General(this, "crocodile", "government", 3);
	crocodile->addSkill(new Corrasion);
	crocodile->addSkill(new SandStorm);

	General *hiluluk = new General(this, "hiluluk", "citizen", 3);
	hiluluk->addSkill(new Quack);
	hiluluk->addSkill(new WinterSakura);

	General *galdino = new General(this, "galdino", "pirate", 4);
	galdino->addSkill(new CandleWax);
	addMetaObject<CandleWaxCard>();

	General *kureha = new General(this, "kureha", "citizen", 3, false);
	kureha->addSkill(new MedicalExpertise);
	kureha->addSkill(new WitheredFlower);
}

ADD_PACKAGE(Alabastan)
