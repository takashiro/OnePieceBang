#include "water7.h"
#include "client.h"
#include "carditem.h"

class IronPunch: public OneCardViewAsSkill{
public:
    IronPunch(): OneCardViewAsSkill("ironpunch"){

    }

    virtual bool viewFilter(const CardItem *) const{
        return Self->getWeapon() == NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "slash";
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *subcard = card_item->getCard();
        Slash *slash = new Slash(subcard->getSuit(), subcard->getNumber());
        slash->setSkillName(objectName());
        return slash;
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

        room->acquireSkill(player, "rankyaku");
        room->setPlayerProperty(player, "kingdom", "marine");
        player->gainMark("@willwaked");

        return false;
    }
};

class Rankyaku: public TriggerSkill{
public:
    Rankyaku(): TriggerSkill("rankyaku"){
        events << FinishJudge;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        JudgeStar judge = data.value<JudgeStar>();
        if(!judge->card || judge->card->isBlack() || !player->askForSkillInvoke(objectName())){
            return false;
        }

        Room *room = player->getRoom();
        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *target, room->getOtherPlayers(player)){
            if(player->inMyAttackRange(target)){
                targets.append(target);
            }
        }
        ServerPlayer *slash_target = room->askForPlayerChosen(player, targets, objectName());
        if(slash_target != NULL){
            Slash *slash = new Slash(Card::NoSuit, 0);
            slash->setSkillName(objectName());
            CardUseStruct use;
            use.card = slash;
            use.from = player;
            use.to.append(slash_target);
            room->useCard(use);
        }

        return false;
    }
};

Water7Package::Water7Package():Package("Water7")
{
    General *garp = new General(this, "garp", "government", 4);
    garp->addSkill(new IronPunch);

    General *coby = new General(this, "coby", "citizen", 3);
    coby->addSkill(new Dream);
    coby->addSkill(new Will);
    skills << new Rankyaku;
}

ADD_PACKAGE(Water7)
