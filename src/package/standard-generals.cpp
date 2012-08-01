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

class RubberPistol: public TriggerSkill{
public:
    RubberPistol(): TriggerSkill("rubberpistol"){
        events << SlashMissed;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        SlashEffectStruct effect = data.value<SlashEffectStruct>();

        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *target, room->getAlivePlayers()){
            if(effect.to->distanceTo(target) == 1){
                targets.append(target);
            }
        }

        if(targets.empty() || !player->askForSkillInvoke(objectName())){
            return false;
        }

        effect.to = room->askForPlayerChosen(player, targets, objectName());
        room->slashEffect(effect);

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
    setObjectName("ex_nihilo");
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
        return !card->isEquipped() && (card->getSuit() == Card::Heart || card->inherits("Exnihilo"));
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

        if(effect.from->getHandcardNum() > effect.from->getHp() || effect.to->getHandcardNum() > effect.from->getHandcardNum()){
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
            if(Self->containsTrick("lighting")){
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

class ClimaEx: public TriggerSkill{
public:
    ClimaEx():TriggerSkill("#clima"){
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
            const Card* oldJudge = judge->card;
            judge->card = QPirate->getCard(card->getEffectiveId());
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

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(player->getJudgingArea().empty() || !player->askForSkillInvoke(objectName())){
            return false;
        }

        Room *room = player->getRoom();
        int card_id = room->askForCardChosen(player, player, "j", objectName());
        if(card_id > 0){
            room->throwCard(card_id, NULL);
            return true;
        }

        return false;
    }
};

TripleSwordsCard::TripleSwordsCard(){
    target_fixed = true;
}

void TripleSwordsCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const{
    room->throwCard(this);
    source->addToPile("sword", subcards.at(0));
    source->gainMark("@#slash_extra_targets");
}

class TripleSwords: public OneCardViewAsSkill{
public:
    TripleSwords(): OneCardViewAsSkill("tripleswords"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getWeapon() != NULL && player->getPile("sword").length() < 3;
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getCard()->getSubtype() == "weapon" && !to_select->isEquipped();
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        TripleSwordsCard *card = new TripleSwordsCard;
        card->addSubcard(card_item->getCard());
        return card;
    }
};

class TripleSwordsEx: public TriggerSkill{
public:
    TripleSwordsEx(): TriggerSkill("#tripleswords"){
        events << CardLostOnePiece;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        CardMoveStar move = data.value<CardMoveStar>();
        if(move->from_place != Player::Equip || QPirate->getCard(move->card_id)->getSubtype() != "weapon"){
            return false;
        }

        Room *room = player->getRoom();

        room->playSkillEffect("tripleswords");

        QList<int> swords = player->getPile("sword");
        if(!swords.empty()){
            room->fillAG(swords, player);
            int card_id = room->askForAG(player, swords, false, "tripleswords");
            player->loseMark("@#slash_extra_targets");
            room->broadcastInvoke("clearAG");

            room->obtainCard(player, card_id);
            CardUseStruct use;
            use.card = QPirate->getCard(card_id);
            use.from = player;
            room->useCard(use);
        }

        return false;
    }
};

class Feast: public OneCardViewAsSkill{
public:
    Feast(): OneCardViewAsSkill("feast"){

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getCard()->getSuit() == Card::Heart;
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *sub = card_item->getCard();
        GodSalvation *card = new GodSalvation(sub->getSuit(), sub->getNumber());
        card->addSubcard(sub);
        card->setSkillName(objectName());
        return card;
    }
};

class Gentleman: public TriggerSkill{
public:
    Gentleman(): TriggerSkill("gentleman"){
        events << HpRecover;
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->getGender() == General::Female;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        RecoverStruct recover = data.value<RecoverStruct>();
        if(recover.who == NULL || !recover.who->hasSkill(objectName()) || !recover.who->askForSkillInvoke(objectName())){
            return false;
        }
        recover.who->drawCards(1);
        return false;
    }
};

class BlackFeet: public OneCardViewAsSkill{
public:
    BlackFeet(): OneCardViewAsSkill("blackfeet"){

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getCard()->inherits("Jink");
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return true;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "slash" || pattern.contains("peach");
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *sub = card_item->getCard();
        Card *card = NULL;
        if(sub->inherits("Jink")){
            card = new FireSlash(sub->getSuit(), sub->getNumber());
        }else if(sub->inherits("Slash") && sub->isRed()){
            card = new Peach(sub->getSuit(), sub->getNumber());
        }

        if(card != NULL){
            card->addSubcard(sub);
            card->setSkillName(objectName());
        }

        return card;
    }
};

class Dream: public TriggerSkill{
public:
    Dream(): TriggerSkill("dream"){
        events << PhaseChange;
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->getPhase() == Player::NotActive;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        QString color;
        Room *room = player->getRoom();
        JudgeStruct judge;
        judge.reason = objectName();
        judge.who = player;
        judge.good = true;

        while(player->askForSkillInvoke(objectName())){
            color = room->askForChoice(player, objectName(), "red+black");            
            if(color == "red"){
                room->sendLog("#DreamRed", player);
                judge.pattern = QRegExp("(.*):(heart|diamond):(.*)");
            }else{
                room->sendLog("#DreamBlack", player);
                judge.pattern = QRegExp("(.*):(spade|club):(.*)");
            }

            room->judge(judge);
            if(judge.isGood()){
                room->obtainCard(player, judge.card->getId());
            }else{
                break;
            }
        }

        return false;
    }
};

class Will: public TriggerSkill{
public:
    Will(): TriggerSkill("will"){
        events << Dying;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->getMark("@willwaked") <= 0;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        RecoverStruct recover;
        recover.who = player;
        room->recover(player, recover);

        room->setPlayerProperty(player, "maxhp", player->getMaxHp() + 1);
        room->setPlayerProperty(player, "kingdom", "government");
        player->gainMark("@willwaked");

        return false;
    }
};

void StandardPackage::addGenerals()
{
    General *luffy = new General(this, "luffy", "pirate", 4);
    luffy->addSkill(new Insulator);
    luffy->addSkill(new RubberPistol);
    luffy->addSkill(new OuKi);
    luffy->addSkill(new MarkAssignSkill("@haouhaki"));
    addMetaObject<OuKiCard>();

    General *zoro = new General(this, "zoro", "pirate", 4);
    zoro->addSkill(new TripleSwords);
    zoro->addSkill(new TripleSwordsEx);
    zoro->addSkill(new Skill("fretywind"));
    related_skills.insertMulti("tripleswords", "#tripleswords");
    addMetaObject<TripleSwordsCard>();

    General *nami = new General(this, "nami", "pirate", 3, false);
    nami->addSkill(new Clima);
    nami->addSkill(new ClimaEx);
    nami->addSkill(new Mirage);
    related_skills.insertMulti("clima", "#clima");

    General *sanji = new General(this, "sanji", "pirate", 4);
    sanji->addSkill(new Feast);
    sanji->addSkill(new Gentleman);
    sanji->addSkill(new BlackFeet);

    General *usopp = new General(this, "usopp", "pirate", 3);
    usopp->addSkill(new Lie);
    usopp->addSkill(new Shoot);
    addMetaObject<LieCard>();

    General *baki = new General(this, "baki", "pirate", 3);
    baki->addSkill(new Divided);

    General *cobi = new General(this, "cobi", "citizen", 3);
    cobi->addSkill(new Dream);
    cobi->addSkill(new Will);
}
