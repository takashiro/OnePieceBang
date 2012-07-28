#include "test.h"


class GodThunder: public TriggerSkill{
public:
    GodThunder(): TriggerSkill("godthunder"){
        events << Predamage << Predamaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        DamageStruct damage = data.value<DamageStruct>();
        if(event == Predamage){
            if(damage.nature != DamageStruct::Thunder){
                room->sendLog("#TriggerSkill", player, objectName());

                damage.nature = DamageStruct::Thunder;
                data = QVariant::fromValue(damage);
            }
        }else{
            if(damage.nature == DamageStruct::Thunder){
                room->sendLog("#TriggerSkill", player, objectName());

                RecoverStruct recover;
                recover.card = damage.card;
                recover.who = damage.from;
                recover.recover = damage.damage;
                room->recover(player, recover);

                return true;
            }
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
        return TriggerSkill::triggerable(target) && target->getPhase() == Player::NotActive;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(!player->askForSkillInvoke(objectName())){
            return false;
        }

        Room *room = player->getRoom();
        const Card *card = room->askForCard(player, ".S", "thunderbot-invoke", QVariant(), NonTrigger);

        if(card){
            room->throwCard(card);

            DamageStruct damage;
            damage.from = player;
            damage.to = room->askForPlayerChosen(player, room->getAlivePlayers(), "thunderbot-invoke");
            damage.damage = qMax(player->getLostHp(), 1);
            room->damage(damage);
        }

        return false;
    }
};

OperatingRoomCard::OperatingRoomCard(){
    target_fixed = true;
}

void OperatingRoomCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const{
    QList<int> cards;
    cards.append(subcards.first());
    foreach(ServerPlayer *target, room->getOtherPlayers(source)){
        int card_id = room->askForCardChosen(source, target, "hej", "operatingroom");
        if(card_id){
            cards.append(card_id);
            room->obtainCard(source, card_id);
        }
    }

    foreach(ServerPlayer *target, room->getOtherPlayers(source)){
        if(cards.empty()){
            break;
        }

        room->fillAG(cards, source);
        int card_id = room->askForAG(source, cards, false, "operatingroom");
        cards.removeOne(card_id);
        room->obtainCard(target, card_id);
        room->broadcastInvoke("clearAG");
    }
}

class OperatingRoom: public ZeroCardViewAsSkill{
public:
    OperatingRoom(): ZeroCardViewAsSkill("operatingroom"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("OperatingRoomCard");
    }

    virtual const Card *viewAs() const{
        return new OperatingRoomCard;
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

TestPackage::TestPackage():Package("Test")
{
    General *chopper = new General(this, "chopper", "pirate", 3);
    chopper->addSkill(new Leechcraft);
    chopper->addSkill(new RumbleBall);
    chopper->addSkill(new RumbleBallAttack);
    chopper->addSkill(new RumbleBallDefense);
    related_skills.insertMulti("rumbleball", "#rumbleballattack");
    related_skills.insertMulti("rumbleball", "#rumbleballdefense");
    patterns[".rumbleball"] = new RumbleBallPattern;

    General *law = new General(this, "law", "pirate", 4);
    law->addSkill(new OperatingRoom);
    addMetaObject<OperatingRoomCard>();

    General *enil = new General(this, "enil", "other", 4);
    enil->addSkill(new GodThunder);
    enil->addSkill(new ThunderBot);
}

ADD_PACKAGE(Test)
