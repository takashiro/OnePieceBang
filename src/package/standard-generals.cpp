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

        room->useCard(use, true);
    }else{
        room->setPlayerFlag(source, "slash_forbidden");
    }
}

class RubberPistol: public ZeroCardViewAsSkill{
public:
    RubberPistol(): ZeroCardViewAsSkill("rubberpistol"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
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
            if(room->askForSkillInvoke(player, "rubberpistol")){
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

LieCard::LieCard(Card::Suit suit, int number): SingleTargetTrick(suit, number, false){
    setObjectName("treasure_chest");
}

bool LieCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.length() < 1;
}

void LieCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->drawCards(2);
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
        LieCard *card = new LieCard(subcard->getSuit(), subcard->getNumber());
        card->setSkillName(objectName());
        card->addSubcard(subcard);
        return card;
    }
};

class Shoot: public SlashBuffSkill{
public:
    Shoot(): SlashBuffSkill("shoot"){
        frequency = Compulsory;
    }

    virtual bool buff(const SlashEffectStruct &effect) const{
        if(effect.from->distanceTo(effect.to) <= 1){
            return false;
        }

        if(effect.to->distanceTo(effect.from) > effect.to->getHandcardNum()){
            Room *room = effect.from->getRoom();
            room->playSkillEffect(objectName());
            room->sendLog("#TriggerSkill", effect.from, objectName());
            room->slashResult(effect, NULL);
            return true;
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
            if(Self->containsTrick("lighting")){
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

class Forecast: public TriggerSkill{
public:
    Forecast():TriggerSkill("forecast"){
        events << AskForRetrial;
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
        prompt_list << "clima" << judge->who->objectName()
                    << "clima" << judge->reason << judge->card->getEffectIdString();
        QString prompt = prompt_list.join(":");
        const Card *card = room->askForCard(player, ".", prompt, data);

        if(card){
            const Card *oldJudge = judge->card;
            judge->card = Bang->getCard(card->getEffectiveId());
            CardsMoveStruct move(QList<int>(), NULL, Player::DiscardPile);
            move.card_ids.append(card->getEffectiveId());
            QList<CardsMoveStruct> moves;
            moves.append(move);
            room->moveCardsAtomic(moves, true);
            player->obtainCard(oldJudge);

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
        return triggerable(target) && !target->getJudgingArea().isEmpty();
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(!player->askForSkillInvoke(objectName())){
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
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent, ServerPlayer *player, QVariant &data) const{
        CardMoveStar move = data.value<CardMoveStar>();
        if(move->from_place == Player::Equip){
            Room *room = player->getRoom();
            if(room->askForSkillInvoke(player, objectName())){
                room->playSkillEffect(objectName());

                ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName());
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
            if(move->from_place == Player::Equip && Bang->getCard(move->card_id)->getSubtype() == "weapon" && !player->getPile("sword").isEmpty()){
                int weapon_id = player->getPile("sword").last();
                Room *room = player->getRoom();
                room->moveCardTo(Bang->getCard(weapon_id), player, Player::Equip, true);
            }
        }else{
            CardUseStruct use = data.value<CardUseStruct>();
            if(use.card && use.card->getSubtype() == "weapon"){
                const Card *weapon = player->getWeapon();
                if(weapon != NULL){
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
            return (Slash::IsAvailable(Self) && card->inherits("Jink")) || (Self->isWounded() && card->inherits("Slash") && card->getSuit() == Card::Club);
        }
        case Client::Responsing:{
            if(ClientInstance->getPattern() == "slash"){
                return card->inherits("Jink");
            }else if( ClientInstance->getPattern() == "vulnerary"){
                return card->inherits("Slash") && card->getSuit() == Card::Club;
            }
        }
        }

        return false;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player) || player->isWounded();
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "slash" || pattern.contains("vulnerary");
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *sub = card_item->getCard();
        Card *card = NULL;
        if(sub->inherits("Jink")){
            card = new FireSlash(sub->getSuit(), sub->getNumber());
        }else if(sub->inherits("Slash") && sub->getSuit() == Card::Club){
            card = new Vulnerary(sub->getSuit(), sub->getNumber());
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
                    room->moveCardTo(card, target, Player::Hand, true);
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

class Kento: public TriggerSkill{
public:
    Kento(): TriggerSkill("kento"){
        events << Damaged;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(!player->askForSkillInvoke(objectName())){
            return false;
        }

        player->drawCards(qMin(player->getLostHp(), 2));
        QList<Player::Phase> phases = player->getPhases();
        phases.prepend(Player::Play);
        player->play(phases);

        return false;
    }
};

class SharkOnTooth: public TriggerSkill{
public:
    SharkOnTooth(): TriggerSkill("sharkontooth"){
        events << SlashMissed;
    }

    virtual bool trigger(TriggerEvent, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        static DamageStruct damage;
        damage.from = player;

        if(player->askForSkillInvoke(objectName())){
            foreach(ServerPlayer *target, room->getOtherPlayers(player)){
                if(!player->inMyAttackRange(target)){
                    continue;
                }

                const Card *card = room->askForCard(target, "jink", objectName());
                if(card != NULL){
                    room->throwCard(card);
                }else{
                    damage.to = target;
                    room->damage(damage);
                }
            }
        }

        return false;
    }
};

SixSwordCard::SixSwordCard(){
    once = true;
}

bool SixSwordCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.length() < subcards.length() && Self->inMyAttackRange(to_select);
}

bool SixSwordCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() == subcards.length();
}

void SixSwordCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    room->throwCard(this, source);

    foreach(ServerPlayer *target, targets){
        QString prompt = "sixsword-target:" + source->getGeneralName();
        const Card *card = room->askForCard(target, ".Equip", prompt, QVariant(), NonTrigger);
        if(card){
            room->throwCard(card, target);
        }else{
            source->drawCards(1);
        }
    }
}

class SixSword: public ViewAsSkill{
public:
    SixSword(): ViewAsSkill("sixsword"){

    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        return selected.length() > 0 && to_select->getCard()->inherits("Slash");
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("SixSwordCard");
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        SixSwordCard *card = new SixSwordCard();
        card->addSubcards(cards);
        card->setSkillName(objectName());
        return card;
    }
};

class FogBarrier: public TriggerSkill{
public:
    FogBarrier(): TriggerSkill("fogbarrier"){
        events << PhaseChange << CardEffected << Predamaged;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(event == PhaseChange){
            if(player->getPhase() == Player::Start){
                player->setFlags("-fogbarrier");
            }else if(player->getPhase() == Player::Finish && player->askForSkillInvoke(objectName())){
                player->setFlags("fogbarrier");
            }

        }else if(event == CardEffected){
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if(player->hasFlag("fogbarrier") && effect.card && effect.card->isNDTrick()){
                player->getRoom()->sendLog("#TriggerSkill", player, objectName());
                return true;
            }

        }else if(event == Predamaged){
            DamageStruct damage = data.value<DamageStruct>();
            if(player->hasFlag("fogbarrier") && damage.nature != DamageStruct::Normal){
                player->getRoom()->sendLog("#TriggerSkill", player, objectName());
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

    General *sanji = new General(this, "sanji", "pirate", 4);
    sanji->addSkill(new Gentleman);
    sanji->addSkill(new BlackFeet);

    General *usopp = new General(this, "usopp", "pirate", 3);
    usopp->addSkill(new Lie);
    usopp->addSkill(new Shoot);
    addMetaObject<LieCard>();

    General *baki = new General(this, "baki", "pirate", 3);
    baki->addSkill(new Divided);

    General *arlong = new General(this, "arlong", "pirate", 4);
    arlong->addSkill(new SharkOnTooth);

    General *hatchan = new General(this, "hatchan", "pirate", 4);
    hatchan->addSkill(new SixSword);
    addMetaObject<SixSwordCard>();

    General *tashigi = new General(this, "tashigi", "government", 3, false);
    tashigi->addSkill(new SwordsExpert);
    tashigi->addSkill(new Kento);

    General *smoker = new General(this, "smoker", "government", 3);
    smoker->addSkill(new FogBarrier);
    smoker->addSkill(new Justice);

    General *gaapu = new General(this, "gaapu", "government", 4);

    General *bellemere = new General(this, "bellemere", "government", 3);
}
