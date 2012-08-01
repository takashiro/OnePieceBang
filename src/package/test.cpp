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

TestPackage::TestPackage():Package("Test")
{
    General *law = new General(this, "law", "pirate", 4);
    law->addSkill(new OperatingRoom);
    addMetaObject<OperatingRoomCard>();

    General *enil = new General(this, "enil", "other", 4);
    enil->addSkill(new GodThunder);
    enil->addSkill(new ThunderBot);
}

ADD_PACKAGE(Test)
