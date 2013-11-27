#include "test.h"

class GodThunder: public TriggerSkill{
public:
    GodThunder(): TriggerSkill("godthunder"){
        events << Predamaged;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        DamageStruct damage = data.value<DamageStruct>();
        if(damage.nature == DamageStruct::Thunder){
            room->sendLog("#TriggerSkill", player, objectName());

            RecoverStruct recover;
            recover.card = damage.card;
            recover.who = damage.from;
            recover.recover = damage.damage;
            room->recover(player, recover);

            return true;
        }

        return false;
    }
};

class GodThunderEx: public TriggerSkill{
public:
    GodThunderEx(): TriggerSkill("#godthunderex"){
        events << Predamaged;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        DamageStruct damage = data.value<DamageStruct>();
        if(damage.card && damage.card->inherits("Lightning")){
            room->sendLog("#TriggerSkill", player, "godthunder");

            damage.from = room->findPlayerBySkillName("godthunder");
            data = QVariant::fromValue(damage);
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
        return TriggerSkill::triggerable(target) && target->getPhase() == Player::Finish;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(!player->askForSkillInvoke(objectName())){
            return false;
        }

        Room *room = player->getRoom();
        const Card *card = room->askForCard(player, ".S", "thunderbot-invoke", QVariant(), NonTrigger);

        if(card){
            room->throwCard(card, player);

            DamageStruct damage;
            damage.from = player;
            damage.to = room->askForPlayerChosen(player, room->getAlivePlayers(), "thunderbot-invoke");
            damage.damage = player->getLostHp();
            damage.nature = DamageStruct::Thunder;
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

class DarkWater: public TriggerSkill{
public:
    DarkWater(): TriggerSkill("darkwater"){
        events << Predamaged << TargetSelected;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(event == Predamaged){
            DamageStruct damage = data.value<DamageStruct>();
            if(damage.nature == DamageStruct::Normal){
                player->getRoom()->sendLog("#TriggerSkill", player, objectName());
                damage.damage++;
                data = QVariant::fromValue(damage);
            }
        }else{
            CardUseStruct use = data.value<CardUseStruct>();
			if(use.card->isRed() && player->isWounded() && player->askForSkillInvoke(objectName())){
                RecoverStruct recover;
                recover.card = use.card;
                recover.who = player;
                player->getRoom()->recover(player, recover);
            }
        }
        return false;
    }
};

class BlackHole: public TriggerSkill{
public:
    BlackHole(): TriggerSkill("blackhole"){
        events << CardEffect << SlashMissed;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        Room *room = target->getRoom();
        foreach(ServerPlayer *player, room->findPlayersBySkillName("darkwater")){
            if(player->getPhase() != Player::NotActive){
                return true;
            }
        }
        return false;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *target, QVariant &data) const{
        Room *room = target->getRoom();

        const Card *card = NULL;
        if(event == CardEffect){
            CardEffectStruct effect = data.value<CardEffectStruct>();
            card = effect.card;

            if(card && card->isRed() && card->getTypeId() == Card::Basic){
                ServerPlayer *player = room->findPlayerBySkillName("blackhole");
                room->sendLog("#TriggerSkill", player, "blackhole");
                return true;
            }

        }else if(event == SlashMissed){
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if(effect.from->hasSkill("blackhole") && effect.from->getPhase() != Player::NotActive && effect.jink->isRed()){
                room->sendLog("#TriggerSkill", effect.from, "blackhole");
                const Card *jink = NULL;
                forever{
                    QString prompt = "slash-jink:" + effect.from->getGeneralName();
                    jink = room->askForCard(effect.to, "jink", prompt, QVariant(), CardUsed);
                    if(jink == NULL){
                        room->slashResult(effect, NULL);
                        return true;
                    }else if(jink->isBlack()){
                        break;
                    }
                }
            }
        }

        return false;
    }
};

TestPackage::TestPackage():Package("Test")
{
    General *law = new General(this, "law", "pirate", 4);
    law->addSkill(new OperatingRoom);
    addMetaObject<OperatingRoomCard>();

    General *enil = new General(this, "enil", "citizen", 3);
    enil->addSkill(new GodThunder);
    enil->addSkill(new GodThunderEx);
    related_skills.insertMulti("godthunder", "#godthunderex");
    enil->addSkill(new ThunderBot);

    General *blackbear = new General(this, "blackbear", "pirate", 3);
    blackbear->addSkill(new DarkWater);
    blackbear->addSkill(new BlackHole);
}

ADD_PACKAGE(Test)
