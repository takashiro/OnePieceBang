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

class FirePunchEx: public CardTargetSkill{
public:
	FirePunchEx(): CardTargetSkill("#firepunchex"){

	}

	virtual int getExtraTargetNum(const Player *player, const Card *card) const{
		int correct = 0;

		if(card->inherits("FireSlash")){
			correct++;
		}

		return correct;
	}
};

class AntiWar: public TriggerSkill{
public:
	AntiWar(): TriggerSkill("antiwar"){
		events << Postdamaging;
	}

	virtual bool triggerable(const ServerPlayer *) const{
		return true;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *target, QVariant &data) const{
		DamageStruct damage = data.value<DamageStruct>();
		if(damage.card == NULL || !damage.card->inherits("Slash") || damage.from == NULL){
			return false;
		}

		const Card *weapon = damage.from->getWeapon();
		if(weapon == NULL){
			return false;
		}

		Room *room = target->getRoom();
		foreach(ServerPlayer *player, room->findPlayersBySkillName(objectName())){
			if(player->askForSkillInvoke(objectName())){
				room->throwCard(weapon, player);

				int card_id = room->askForCardChosen(damage.from, player, "h", objectName());
				room->obtainCard(damage.from, card_id);
			}
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
		Duel *duel = new Duel(Card::NoSuit, 0);
		duel->setSkillName("fleur");
		CardUseStruct use;
		use.card = duel;

		use.from = targets.at(0);
		use.to << targets.at(1);

		room->useCard(use, true);
	}else{
		ServerPlayer *target = targets.at(card1->isRed() ? 0 : 1);

		int card_id;
		LogMessage log;
		log.type = "$Dismantlement";
		log.from = target;

		for(int i = 0; i < 2; i++){
			card_id = room->askForCardChosen(source, target, "he", "fleur");
			room->throwCard(card_id, source);
			log.card_str = QString::number(card_id);
			room->sendLog(log);
		}
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
			Room *room = player->getRoom();
			room->broadcastSkillInvoked(player, objectName());
			room->sendLog("#TriggerSkill", player, objectName());
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
		return to_select->getFilteredCard()->getSuit() == Card::Club;
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
		events << Damaging << Recovering;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return TriggerSkill::triggerable(target) && target->getHp() == 1;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		Room *room = player->getRoom();

		if(event == Damaging){
			room->broadcastSkillInvoked(player, objectName());
			room->sendLog("#TriggerSkill", player, objectName());

			DamageStruct damage = data.value<DamageStruct>();
			damage.damage++;
			data = QVariant::fromValue(damage);
		}else{
			RecoverStruct recover = data.value<RecoverStruct>();
			if(recover.card && recover.card->inherits("Wine")){
				room->broadcastSkillInvoked(player, objectName());
				room->sendLog("#TriggerSkill", player, objectName());

				recover.recover++;
				data = QVariant::fromValue(recover);
			}
		}

		return false;
	}
};

ImitateCard::ImitateCard(){

}

bool ImitateCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const{
	return targets.isEmpty() && !to_select->getVisibleSkillList().isEmpty();
}

bool ImitateCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
	return targets.length() == 1;
}

void ImitateCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
	QStringList skills;
	foreach(ServerPlayer *target, targets){
		foreach(const ::Skill *skill, target->getVisibleSkillList()){
			skills.append(skill->objectName());
		}
	}

	if(skills.isEmpty()){
		return;
	}

	if(source->tag.contains("imitated_skill_name")){
		room->detachSkillFromPlayer(source, source->tag.value("imitated_skill_name").toString());
	}

	QString skill;
	if(skills.length() > 1){
		skill = room->askForChoice(source, "imitate", skills.join("+"));
	}else{
		skill = skills.at(0);
	}
	room->acquireSkill(source, skill);
	source->tag["imitated_skill_name"] = skill;

	room->setPlayerProperty(source, "kingdom", source->getKingdom());
}

class ImitateViewAsSkill: public ZeroCardViewAsSkill{
public:
	ImitateViewAsSkill(): ZeroCardViewAsSkill("imitate"){

	}

	bool isEnabledAtPlay(const Player *) const{
		return false;
	}

	bool isEnabledAtResponse(const Player *, const QString &pattern) const{
		return pattern == "@@imitate";
	}

	const Card *viewAs() const{
		return new ImitateCard;
	}
};

class Imitate: public TriggerSkill{
public:
	Imitate(): TriggerSkill("imitate"){
		events << PhaseChange;
		view_as_skill = new ImitateViewAsSkill;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return TriggerSkill::triggerable(target) && target->getPhase() == Player::Start;
	}

	virtual bool trigger(TriggerEvent, ServerPlayer *player, QVariant &) const{
		Room *room = player->getRoom();
		room->askForUseCard(player, "@@imitate", objectName());
		return false;
	}
};

class Memoir: public TriggerSkill{
public:
	Memoir(): TriggerSkill("memoir"){
		events << OneCardLost;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return target->isAlive();
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *target, QVariant &data) const{
		CardMoveStar move = data.value<CardMoveStar>();
		if(move->from_place != Player::HandArea || !target->isLastHandCard(Bang->getCard(move->card_id))){
			return false;
		}

		Room *room = target->getRoom();
		foreach(ServerPlayer *player, room->getOtherPlayers(target)){
			if(player->hasSkill(objectName()) && player->askForSkillInvoke(objectName(), data)){
				CardsMoveStruct move;
				move.to = target;
				move.to_place = Player::HandArea;
				move.as_one_time = true;
				foreach(const Card *card, player->getHandcards()){
					move.card_ids << card->getEffectiveId();
				}
				room->moveCards(move, false);

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
		events << Damaged;
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
		 events << AfterRecovering;
		 frequency = Compulsory;
	}

	virtual bool trigger(TriggerEvent, ServerPlayer *target, QVariant &data) const{
		RecoverStruct recover = data.value<RecoverStruct>();
		Room *room = target->getRoom();

		for(int i = 0; i < recover.recover; i++){
			room->sendLog("#TriggerSkill", recover.from, objectName());
			target->turnOver();
			target->drawCards(recover.from->getLostHp());
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
			recover.from = player;

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
		return target != NULL && target->getCardCount(true) <= 0;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *target, QVariant &data) const{
		Room *room = target->getRoom();
		static RecoverStruct recover;

		foreach(ServerPlayer *player, room->findPlayersBySkillName(objectName())){
			if(player == target){
				continue;
			}

			DyingStruct dying = data.value<DyingStruct>();
			if(dying.who->isKongcheng() || !player->askForSkillInvoke(objectName(), data)){
				return false;
			}

			room->showAllCards(dying.who);

			int card_id = room->askForCardChosen(player, dying.who, "he", objectName());
			if(card_id > 0){
				room->obtainCard(player, card_id, true);

				const Card *card = Bang->getCard(card_id);
				if(card && card->isRed()){
					recover.from = player;
					recover.recover = 1;
					recover.to = dying.who;
					room->recover(recover);
				}
			}
		}

		return false;
	}
};

class WitheredFlower: public TriggerSkill{
public:
	WitheredFlower(): TriggerSkill("witheredflower"){
		events << CardEffected;
		frequency = Frequent;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		CardEffectStruct effect = data.value<CardEffectStruct>();

		if(!effect.from || effect.from == player || !effect.card || effect.card->getSuit() != Card::Club || !player->askForSkillInvoke(objectName())){
			return false;
		}

		Room *room = player->getRoom();

		bool draw_card = false;

		if(effect.from->isKongcheng())
			draw_card = true;
		else{
			QString prompt = "witheredflower-discard:" + player->getGeneralName();
			const Card *card = room->askForCard(effect.from, ".", prompt, QVariant(), CardDiscarded);
			if(card == NULL)
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
	related_skills.insertMulti("firepunch", "#firepunchex");

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

	General *bonkure = new General(this, "bonkure", "pirate", 3, true);
	bonkure->addSkill(new Imitate);
	addMetaObject<ImitateCard>();
	bonkure->addSkill(new Memoir);
	bonkure->setGender(General::Neuter);

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
