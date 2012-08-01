#include "alabastan.h"
#include "skill.h"
#include "maneuvering.h"
#include "carditem.h"

class FirePunch: public FilterSkill{
public:
    FirePunch(): FilterSkill("firepunch"){

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getCard()->objectName() == "slash";
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *sub = card_item->getCard();
        FireSlash *slash = new FireSlash(sub->getSuit(), sub->getNumber());
        slash->setSkillName(objectName());
        slash->addSubcard(sub);
        return slash;
    }
};

class FlameRing: public TriggerSkill{
public:
    FlameRing(): TriggerSkill("flamering"){
        events << Predamage;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.nature != DamageStruct::Fire || player->askForSkillInvoke(objectName())){
            return false;
        }

        damage.to->setChained(true);
        return false;
    }
};

class AntiWar: public TriggerSkill{
public:
    AntiWar(): TriggerSkill("antiwar"){
        events << Damaged;
    }

    virtual bool triggerable(const ServerPlayer *) const{
        return true;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *target, QVariant &data) const{
        Room *room = target->getRoom();
        foreach(ServerPlayer *player, room->findPlayersBySkillName(objectName())){
            if(player->isKongcheng()){
                continue;
            }

            const Card *card = room->askForCard(player, ".black", "#AntiWarPrompt");
            if(card == NULL){
                continue;
            }
            room->sendLog("#InvokeSkill", player, objectName());
            room->throwCard(card);

            DamageStruct damage = data.value<DamageStruct>();
            damage.from->turnOver();
            damage.to->turnOver();
        }

        return false;
    }
};


class Alliance: public TriggerSkill{
public:
    Alliance(): TriggerSkill("alliance"){
        events << Predamaged;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && !target->faceUp();
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.from != NULL && damage.from->faceUp()){
            return true;
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
        DamageStruct damage;
        if(card1->isBlack()){
            damage.from = targets.at(0);
            damage.to = targets.at(1);
        }else{
            damage.from = targets.at(1);
            damage.to = targets.at(0);
        }
        room->damage(damage);
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
        if(effect.card != NULL && effect.card->inherits("BusterCall")){
            player->getRoom()->sendLog("#TriggerSkill", player, objectName());
            player->drawCards(1);
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
        return  pattern.contains("peach") && player->getPhase() == Player::NotActive;
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getFilteredCard()->isRed();
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *first = card_item->getCard();
        Peach *peach = new Peach(first->getSuit(), first->getNumber());
        peach->addSubcard(first->getId());
        peach->setSkillName(objectName());
        return peach;
    }
};

class RumbleBallPattern: public CardPattern{
public:
    virtual bool match(const Player *player, const Card *card) const{
        return !player->hasEquip(card) && (card->getSuit() == Card::Spade || card->getSuit() == Card::Heart);
    }

    virtual bool willThrow() const{
        return true;
    }
};

class RumbleBall: public TriggerSkill{
public:
    RumbleBall(): TriggerSkill("rumbleball"){
        events << PhaseChange;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->getPhase() == Player::Start;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        player->setFlags("-rumbleballAttack");
        player->setFlags("-rumbleballDefense");

        Room *room = player->getRoom();
        const Card *card = room->askForCard(player, ".rumbleball", "rumbleball-invoke", QVariant(), NonTrigger);
        if(card == NULL){
            return false;
        }

        switch(card->getSuit()){
        case Card::Spade:
            player->setFlags("rumbleballAttack");
            break;
        case Card::Heart:
            player->setFlags("rumbleballDefense");
            break;
        default:;
        }

        return false;
    }
};

class RumbleBallAttack: public TriggerSkill{
public:
    RumbleBallAttack(): TriggerSkill("#rumbleballattack"){
        events << Predamage;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->hasFlag("rumbleballAttack");
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        damage.damage++;
        data = QVariant::fromValue(damage);
        return false;
    }
};

class RumbleBallDefense: public TriggerSkill{
public:
    RumbleBallDefense(): TriggerSkill("#rumbleballdefense"){
        events << HpRecover;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        RecoverStruct recover = data.value<RecoverStruct>();
        if(recover.who != NULL && recover.who->hasSkill("rumbleball") && recover.who->hasFlag("rumbleDefense")){
            recover.recover++;
            data = QVariant::fromValue(recover);
        }
        return false;
    }
};

AlabastanPackage::AlabastanPackage():Package("Alabastan")
{
    General *ace = new General(this, "ace", "pirate", 4);
    ace->addSkill(new FirePunch);
    ace->addSkill(new FlameRing);

    General *vivi = new General(this, "vivi", "noble", 3, false);
    vivi->addSkill(new AntiWar);
    vivi->addSkill(new Alliance);

    General *robin = new General(this, "robin", "pirate", 3, false);
    robin->addSkill(new Fleur);
    robin->addSkill(new Survivor);
    addMetaObject<FleurCard>();

    General *chopper = new General(this, "chopper", "pirate", 3);
    chopper->addSkill(new Leechcraft);
    chopper->addSkill(new RumbleBall);
    chopper->addSkill(new RumbleBallAttack);
    chopper->addSkill(new RumbleBallDefense);
    related_skills.insertMulti("rumbleball", "#rumbleballattack");
    related_skills.insertMulti("rumbleball", "#rumbleballdefense");
    patterns[".rumbleball"] = new RumbleBallPattern;
}

ADD_PACKAGE(Alabastan)
