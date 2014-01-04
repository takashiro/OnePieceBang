#include "standard-generals.h"
#include "client.h"

class Insulator: public TriggerSkill{
public:
	Insulator(): TriggerSkill("insulator"){
		events << Predamaged;
		frequency = Compulsory;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		DamageStruct damage = data.value<DamageStruct>();

		if(damage.nature == DamageStruct::Thunder){
			player->getRoom()->sendLog("#TriggerSkill", player, objectName());
			return true;
		}else if(damage.nature == DamageStruct::Fire){
			player->getRoom()->sendLog("#TriggerSkill", player, objectName());
			damage.damage++;
			data = QVariant::fromValue(damage);
		}

		return false;
	}
};

RubberPistolCard::RubberPistolCard(){
	slash = new Slash(Card::NoSuit, 0);
	slash->setSkillName("rubberpistol");
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

		room->useCard(use, false);
	}else{
		room->setPlayerFlag(source, "rubberpistol_forbidden");
	}
}

class RubberPistol: public ZeroCardViewAsSkill{
public:
	RubberPistol(): ZeroCardViewAsSkill("rubberpistol"){

	}

	virtual bool isEnabledAtPlay(const Player *player) const{
		return !player->hasFlag("slash_forbidden") && !player->hasFlag("rubberpistol_forbidden");
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
		events << Predamage;
		frequency = Compulsory;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
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
			player->getRoom()->sendLog("#TriggerSkill", player, objectName());
			return true;
		}

		return false;
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
	ForecastViewAsSkill(): OneCardViewAsSkill(""){

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
			judge->card = Bang->getCard(card->getEffectiveId());
			CardsMoveStruct move(QList<int>(), NULL, Player::DiscardPile);
			move.card_ids.append(card->getEffectiveId());
			QList<CardsMoveStruct> moves;
			moves.append(move);
			room->moveCardsAtomic(moves, true);

			LogMessage log;
			log.type = "$ChangedJudge";
			log.from = player;
			log.to << judge->who;
			log.card_str = card->getEffectIdString();
			room->sendLog(log);

			room->sendJudgeResult(judge);
		}

		return false;
	}
};

class Mirage: public TriggerSkill{
public:
	Mirage(): TriggerSkill("mirage"){
		events << Predamaged;
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
		events << CardLostOnePiece;
	}

	virtual bool trigger(TriggerEvent, ServerPlayer *player, QVariant &data) const{
		CardMoveStar move = data.value<CardMoveStar>();
		if(move->from_place == Player::EquipArea && move->to != move->from){
			Room *room = player->getRoom();

			QList<ServerPlayer *> targets = room->getOtherPlayers(player);
			if(!targets.isEmpty() && room->askForSkillInvoke(player, objectName())){
				room->playSkillEffect(objectName());

				ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
				if(target != NULL){
					CardUseStruct use;

					if(room->askForChoice(player, objectName(), "slash+duel") == "slash"){
						Slash *slash = new Slash(Card::NoSuit, 0);
						slash->setSkillName(objectName());
						use.card = slash;
					}else{
						Duel *duel = new Duel(Card::NoSuit, 0);
						duel->setSkillName(objectName());
						use.card = duel;
					}

					use.from = player;
					use.to << target;
					room->useCard(use);
				}
			}
		}

		return false;
	}
};

class TripleSword: public TriggerSkill{
public:
	TripleSword(): TriggerSkill("triplesword"){
		events << CardUsed << CardLostOnePiece;
		frequency = Compulsory;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		if(event == CardLostOnePiece){
			CardMoveStar move = data.value<CardMoveStar>();
			if(move->to != move->from && move->from_place == Player::EquipArea && Bang->getCard(move->card_id)->getSubtype() == "weapon" && player->getWeapon() == NULL && !player->getPile("sword").isEmpty()){
				Room *room = player->getRoom();

				QList<int> swords = player->getPile("sword");
				room->fillAG(swords, player);
				int weapon_id = room->askForAG(player, swords, false, "triplesword");
				room->broadcastInvoke("clearAG");
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
		events << HpRecover;
		frequency = Frequent;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return target != NULL && (target->getGender() == General::Female || target->hasSkill(objectName()));
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		RecoverStruct recover = data.value<RecoverStruct>();

		if(recover.who == NULL || player == NULL){
			return false;
		}

		if(recover.who->hasSkill(objectName()) && player->getGender() == General::Female && recover.who->askForSkillInvoke(objectName())){
			recover.who->drawCards(1);
		}else if(recover.who->getGender() == General::Female && player->hasSkill(objectName()) && player->askForSkillInvoke(objectName())){
			player->drawCards(1);
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

	virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
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

class SwordsExpert: public TriggerSkill{
public:
	SwordsExpert(): TriggerSkill("swordsexpert"){
		events << PhaseChange;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return TriggerSkill::triggerable(target) && (target->getPhase() == Player::Start || target->getPhase() == Player::Finish);
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		Room *room = player->getRoom();

		if(player->getPhase() == Player::Start){
			QList<ServerPlayer *> targets;
			foreach(ServerPlayer *target, room->getOtherPlayers(player)){
				if(target->getWeapon() != NULL){
					targets.append(target);
				}
			}
			if(!targets.isEmpty() && player->askForSkillInvoke(objectName())){
				ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
				room->obtainCard(player, target->getWeapon());
				target->setFlags("SwordsExpertTarget");
			}

		}else if(player->getPhase() == Player::Finish){
			foreach(ServerPlayer *target, room->getOtherPlayers(player)){
				if(!target->hasFlag("SwordsExpertTarget")){
					continue;
				}

				target->setFlags("-SwordsExpertTarget");
				QString prompt = "swordsexpert-return:" + target->getGeneralName();
				const Card *card = room->askForCard(player, ".Equip", prompt, QVariant(), NonTrigger);
				if(card != NULL){
					room->moveCardTo(card, target, Player::HandArea, true);
				}else{
					DamageStruct damage;
					damage.from = target;
					damage.to = player;
					room->damage(damage);
				}
			}
		}

		return false;
	}
};

class Myopia: public DistanceSkill{
public:
    Myopia(): DistanceSkill("myopia"){

    }

    virtual int getCorrect(const Player *from, const Player *to) const{
        if(to->hasSkill(objectName()) && !to->inMyAttackRange(from)){
            return 1;
        }

        return 0;
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

WaltzCard::WaltzCard(){
	once = true;
	target_fixed = true;
}

void WaltzCard::onUse(Room *room, const CardUseStruct &card_use) const{
	CardUseStruct use = card_use;

	use.to.clear();
	foreach(ServerPlayer *target, room->getOtherPlayers(use.from)){
		if(use.from->inMyAttackRange(target)){
			use.to << target;
		}
	}

	SkillCard::onUse(room, use);
}

void WaltzCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
	CardUseStruct use;
	use.from = source;
	use.to = targets;
	use.card = Bang->getCard(subcards.at(0));

	room->useCard(use, false);
}

class Waltz: public OneCardViewAsSkill{
public:
	Waltz(): OneCardViewAsSkill("waltz"){

	}

	virtual bool viewFilter(const CardItem *to_select) const{
		return to_select->getCard()->inherits("Slash");
	}

	virtual bool isEnabledAtPlay(const Player *player) const{
		return !player->hasUsed("WaltzCard");
	}

	virtual const Card *viewAs(CardItem *card_item) const{
		WaltzCard *card = new WaltzCard;
		card->addSubcard(card_item->getCard());
		card->setSkillName(objectName());
		return card;
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
		events << CardEffected << Predamaged;
	}

	virtual bool triggerable(const ServerPlayer *target) const{
		return target->getMark("@fogbarrier") > 0;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		if(event == CardEffected){
			CardEffectStruct effect = data.value<CardEffectStruct>();
			if(effect.card && effect.card->isNDTrick()){
				player->getRoom()->sendLog("#TriggerSkill", player, "fogbarrier");
				return true;
			}

		}else if(event == Predamaged){
			DamageStruct damage = data.value<DamageStruct>();
			if(damage.nature != DamageStruct::Normal){
				player->getRoom()->sendLog("#TriggerSkill", player, "fogbarrier");
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
		events << Predamaged;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		DamageStruct damage = data.value<DamageStruct>();
		if(damage.card && damage.card->getSuit() == Card::Spade){
			player->getRoom()->sendLog("#TriggerSkill", player, objectName());
			return true;
		}
		return false;
	}
};

class TopSwordman: public TriggerSkill{
public:
	TopSwordman(): TriggerSkill("topswordman"){
		events << Predamaged << Predamage;
	}

	virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
		if(event == Predamaged){
			DamageStruct damage = data.value<DamageStruct>();
			if(damage.from && damage.from->getWeapon() && damage.card && damage.card->inherits("Slash")){
				damage.damage--;
				data = QVariant::fromValue(damage);

				player->getRoom()->sendLog("#TriggerSkill", player, objectName());
			}
		}else if(event == Predamage){
			DamageStruct damage = data.value<DamageStruct>();
			if(player->getWeapon() && damage.nature == DamageStruct::Normal && damage.card && damage.card->inherits("Slash")){
				damage.damage++;
				data = QVariant::fromValue(damage);

				player->getRoom()->sendLog("#TriggerSkill", player, objectName());
			}
		}

		return false;
	}
};


class Upright: public TriggerSkill{
public:
    Upright(): TriggerSkill("upright"){
        events << Damaged;
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
        events << TargetSelecting;
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

void StandardPackage::addGenerals()
{
	General *luffy = new General(this, "luffy", "pirate", 4);
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

	General *arlong = new General(this, "arlong", "pirate", 4);
	arlong->addSkill(new SharkOnTooth);

	General *hatchan = new General(this, "hatchan", "pirate", 4);
	hatchan->addSkill(new Waltz);
	addMetaObject<WaltzCard>();

	General *tashigi = new General(this, "tashigi", "government", 3, false);
	tashigi->addSkill(new SwordsExpert);
    tashigi->addSkill(new Myopia);

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
}
