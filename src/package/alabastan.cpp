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

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(player->isKongcheng() || !player->askForSkillInvoke(objectName())){
            return false;
        }

        Room *room = player->getRoom();
        const Card *card = room->askForCard(player, ".black", objectName());
        if(card == NULL){
            return false;
        }

        DamageStruct damage = data.value<DamageStruct>();
        damage.from->turnOver();
        damage.to->turnOver();

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
    return targets.length() < 2;
}

bool FleurCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() == 2;
}

void FleurCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    room->throwCard(this);

    if(targets.length() < 2){
        return;
    }

    targets.at(0)->drawCards(1);
    targets.at(1)->drawCards(1);

    if(targets.at(0)->isKongcheng() || targets.at(1)->isKongcheng()){
        return;
    }

    DamageStruct damage;
    if(targets.at(0)->pindian(targets.at(1), "fleur")){
        damage.from = targets.at(0);
        damage.to = targets.at(1);
    }else{
        damage.from = targets.at(1);
        damage.to = targets.at(0);
    }
    room->damage(damage);
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
}

ADD_PACKAGE(Alabastan)
