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
            if(use.card->isRed() && player->askForSkillInvoke(objectName())){
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

class NeptunianAttackAvoid: public TriggerSkill{
public:
    NeptunianAttackAvoid(const QString &avoid_skill)
        :TriggerSkill("#na_avoid_" + avoid_skill), avoid_skill(avoid_skill)
    {
        events << CardEffected;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if(effect.card->inherits("NeptunianAttack")){
            LogMessage log;
            log.type = "#SkillNullify";
            log.from = player;
            log.arg = avoid_skill;
            log.arg2 = "neptunian_attack";
            player->getRoom()->sendLog(log);

            return true;
        }else
            return false;
    }

private:
    QString avoid_skill;
};

class SeaQueen: public TriggerSkill{
public:
    SeaQueen():TriggerSkill("seaqueen"){
        events << CardFinished;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return !target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if(use.card->inherits("NeptunianAttack") &&
                ((!use.card->isVirtualCard()) ||
                  (use.card->getSubcards().length() == 1 &&
                  Bang->getCard(use.card->getSubcards().first())->inherits("NeptunianAttack")))){
            Room *room = player->getRoom();
            if(room->getCardPlace(use.card->getEffectiveId()) == Player::DiscardPile){
                QList<ServerPlayer *> players = room->getAllPlayers();
                foreach(ServerPlayer *p, players){
                    if(p->hasSkill(objectName())){
                        p->obtainCard(use.card);
                        room->playSkillEffect(objectName());
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

    General *enil = new General(this, "enil", "citizen", 4);
    enil->addSkill(new GodThunder);
    enil->addSkill(new ThunderBot);

    General *blackbear = new General(this, "blackbear", "pirate", 3);
    blackbear->addSkill(new DarkWater);
    blackbear->addSkill(new BlackHole);

    General *shirahoshi = new General(this, "shirahoshi", "noble", 3, false);
    shirahoshi->addSkill(new SeaQueen);
    shirahoshi->addSkill(new NeptunianAttackAvoid("seaqueen"));
    related_skills.insertMulti("seaqueen", "#na_avoid_seaqueen");
}

ADD_PACKAGE(Test)
