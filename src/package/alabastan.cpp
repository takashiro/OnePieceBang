#include "alabastan.h"
#include "skill.h"
#include "carditem.h"
#include "engine.h"

class FirePunch: public OneCardViewAsSkill{
public:
    FirePunch(): OneCardViewAsSkill("firepunch"){

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        const Card *card = to_select->getFilteredCard();
        return card->inherits("Slash") && !card->isEquipped();
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
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.nature != DamageStruct::Fire || !player->askForSkillInvoke(objectName())){
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
        events << CardGotOneTime;
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->getPhase() != Player::Draw;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *target, QVariant &data) const{
        Room *room = target->getRoom();
        foreach(ServerPlayer *player, room->getAlivePlayers()){
            if(player->hasSkill(objectName()) && player->askForSkillInvoke(objectName(), data)){
                player->drawCards(1);
            }
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
        Duel *duel = new Duel(Card::NoSuit, 0);
        CardUseStruct use;
        use.card = duel;

        if(card1->isBlack()){
            use.from = targets.at(0);
            use.to.append(targets.at(1));
        }else{
            use.from = targets.at(1);
            use.to.append(targets.at(0));
        }
        room->useCard(use, true);
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
        if(effect.card != NULL && effect.card->inherits("NeptunianAttack")){
            player->getRoom()->sendLog("#TriggerSkill", player, objectName());
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
        return  pattern.contains("vulnerary") && player->getPhase() == Player::NotActive;
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getFilteredCard()->isRed();
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *first = card_item->getCard();
        Vulnerary *vulnerary = new Vulnerary(first->getSuit(), first->getNumber());
        vulnerary->addSubcard(first->getId());
        vulnerary->setSkillName(objectName());
        return vulnerary;
    }
};

class RumbleBall: public TriggerSkill{
public:
    RumbleBall(): TriggerSkill("rumbleball"){
        events << Predamage << CardEffect;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->getHp() == 1;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        if(event == Predamage){
            room->sendLog("#TriggerSkill", player, objectName());

            DamageStruct damage = data.value<DamageStruct>();
            damage.damage++;
            data = QVariant::fromValue(damage);
        }else{
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if(effect.card->inherits("Vulnerary")){
                room->sendLog("#TriggerSkill", player, objectName());

                RecoverStruct recover;
                recover.card = effect.card;
                recover.who = player;
                room->recover(effect.to, recover);
            }
        }

        return false;
    }
};

class Imitate: public TriggerSkill{
public:
    Imitate(): TriggerSkill("imitate"){
        events << PhaseChange;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && (target->getPhase() == Player::Start || target->getPhase() == Player::Finish);
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(!player->askForSkillInvoke(objectName())){
            return false;
        }

        Room *room = player->getRoom();

        if(player->tag.contains("imitated_skill_name")){
            room->detachSkillFromPlayer(player, player->tag.value("imitated_skill_name").toString());
        }

        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName());
        if(target != NULL){
            QStringList skills;
            foreach(const Skill *skill, target->getVisibleSkillList()){
                skills.append(skill->objectName());
            }

            if(!skills.isEmpty()){
                QString skill;
                if(skills.length() > 1){
                    skill = room->askForChoice(player, objectName(), skills.join("+"));
                }else{
                    skill = skills.at(0);
                }
                room->acquireSkill(player, skill);
                player->tag["imitated_skill_name"] = skill;

                room->setPlayerProperty(player, "gender", target->getGenderString());
                room->setPlayerProperty(player, "kingdom", target->getKingdom());
            }
        }

        return false;
    }
};

class Corrasion: public TriggerSkill{
public:
    Corrasion(): TriggerSkill("corrasion"){
        events << Predamaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.from == NULL || damage.damage != 1 || damage.card == NULL || !damage.card->inherits("Slash")){
            return false;
        }

        Room *room = player->getRoom();

        room->sendLog("#TriggerSkill", player, objectName());

        QString prompt = "corrasion-effect:" + player->getGeneralName();
        const Card *card = room->askForCard(damage.from, ".S", prompt, QVariant(), NonTrigger);
        if(card != NULL){
            room->obtainCard(player, card);
        }else{
            return true;
        }

        return false;
    }
};

class SandStorm: public TriggerSkill{
public:
    SandStorm(): TriggerSkill("sandstorm"){
        events << CardDiscarded << PhaseChange;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(event == CardDiscarded){
            const Card *card = data.value<CardStar>();

            if(card == NULL){
                return false;
            }

            bool spade = false;
            if(card->subcardsLength() > 0){
                foreach(int card_id, card->getSubcards()){
                    if(Bang->getCard(card_id)->getSuit() == Card::Spade){
                        spade = true;
                    }
                    break;
                }
            }else{
                spade = card->getSuit() == Card::Spade;
            }

            player->tag["SandStormEnabled"] = spade;

        }else if(player->tag.value("SandStormEnabled").toBool() && player->askForSkillInvoke(objectName(), data)){
            Room *room = player->getRoom();
            foreach(ServerPlayer *target, room->getAlivePlayers()){
                if(!target->isAlive()){
                    continue;
                }

                const Card *discard_card = NULL;
                QString prompt = "sandstorm-effected:" + player->getGeneralName();
                if(target->isKongcheng() || (discard_card = room->askForCard(target, ".S", prompt, QVariant(), NonTrigger)) == NULL){
                    room->loseHp(target, 1);
                }else if(discard_card != NULL){
                    room->throwCard(discard_card, target);
                }
            }
        }

        return false;
    }
};

class Quack: public TriggerSkill{
public:
    Quack(): TriggerSkill("quack"){
         events << HpRecover;
         frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *target, QVariant &data) const{
        RecoverStruct recover = data.value<RecoverStruct>();
        if(recover.who != NULL && recover.who->hasSkill(objectName())){
            target->turnOver();
            target->drawCards(recover.who->getLostHp());
        }
        return false;
    }
};

class WinterSakura: public TriggerSkill{
public:
    WinterSakura(): TriggerSkill("wintersakura"){
        events << Dying;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        if(target == NULL){
            return false;
        }

        Room *room = target->getRoom();
        foreach(ServerPlayer *player, room->getAlivePlayers()){
            if(player->hasSkill(objectName()) && !player->isKongcheng()){
                return true;
            }
        }

        return false;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *target, QVariant &data) const{
        Room *room = target->getRoom();

        foreach(ServerPlayer *player, room->findPlayersBySkillName(objectName())){
            if(!player->askForSkillInvoke(objectName())){
                continue;
            }

            room->showAllCards(player);

            RecoverStruct recover;
            recover.who = player;

            foreach(const Card *card, player->getHandcards()){
                if(card->getSuit() == Card::Club){
                    room->throwCard(card, player);
                    room->recover(target, recover);
                }
            }
        }

        return false;
    }
};

CandleWaxCard::CandleWaxCard(){
    will_throw = false;
}

bool CandleWaxCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return !Bang->getCard(subcards.at(0))->targetFixed() && Bang->getCard(subcards.at(0))->targetFilter(targets, to_select, Self);
}

bool CandleWaxCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return Bang->getCard(subcards.at(0))->targetsFeasible(targets, Self);
}

void CandleWaxCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    room->showCard(source, subcards.at(0));

    const Card *trick = Bang->getCard(subcards.at(0));
    const Card *sub = Bang->getCard(subcards.at(1));

    const QMetaObject *meta = trick->metaObject();
    QObject *card_obj = meta->newInstance(Q_ARG(Card::Suit, sub->getSuit()), Q_ARG(int, sub->getNumber()));

    if(card_obj){
        Card *card = qobject_cast<Card *>(card_obj);
        card->setObjectName(trick->objectName());
        card->addSubcard(sub);
        card->setSkillName("candlewax");

        CardUseStruct use;
        use.from = source;
        use.to = targets;
        use.card = card;
        room->useCard(use);
    }
}

class CandleWax: public ViewAsSkill{
public:
    CandleWax():ViewAsSkill("candlewax"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return true;
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        if(selected.length() == 0){
            return to_select->getCard()->isNDTrick();
        }else if(selected.length() == 1){
            return to_select->getCard()->getSuit() == Card::Diamond;
        }

        return false;
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.length() == 2){
            CandleWaxCard *card = new CandleWaxCard;
            card->addSubcards(cards);
            card->setSkillName(objectName());
            const Card *sub = cards.at(1)->getCard();
            card->setSuit(sub->getSuit());
            card->setNumber(sub->getNumber());
            return card;
        }

        return NULL;
    }
};

class MedicalExpertise: public TriggerSkill{
public:
    MedicalExpertise(): TriggerSkill("medicalexpertise"){
        events << AskForVulneraries;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        DyingStruct dying = data.value<DyingStruct>();
        if(dying.who->isKongcheng() || !player->askForSkillInvoke(objectName())){
            return false;
        }

        static RecoverStruct recover;
        recover.who = player;
        recover.recover = 0;
        foreach(const Card *card, dying.who->getHandcards()){
            if(card->isRed()){
                recover.recover++;
            }
        }
        dying.who->throwAllHandCards();

        if(recover.recover > 0){
            player->getRoom()->recover(dying.who, recover);
        }

        return false;
    }
};

class WitheredFlower: public TriggerSkill{
public:
    WitheredFlower(): TriggerSkill("witheredflower"){
        events << TargetSelected;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();

        if(!use.card || use.card->getSuit() != Card::Club || !player->askForSkillInvoke(objectName())){
            return false;
        }

        Room *room = player->getRoom();

        bool draw_card = false;

        if(use.from->isKongcheng())
            draw_card = true;
        else{
            QString prompt = "okama-microphone-card:" + player->getGeneralName();
            const Card *card = room->askForCard(use.from, ".", prompt, QVariant(), CardDiscarded);
            if(card){
                room->throwCard(card);
            }else
                draw_card = true;
        }

        if(draw_card)
            player->drawCards(1);

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

    General *bonkure = new General(this, "bonkure", "pirate", 3);
    bonkure->addSkill(new Imitate);
    bonkure->addSkill(new Skill("okama", Skill::Compulsory));

    General *crockdile = new General(this, "crockdile", "government", 3);
    crockdile->addSkill(new Corrasion);
    crockdile->addSkill(new SandStorm);

    General *hiluluk = new General(this, "hiluluk", "citizen", 3);
    hiluluk->addSkill(new Quack);
    hiluluk->addSkill(new WinterSakura);

    General *galdino = new General(this, "galdino", "pirate", 4);
    galdino->addSkill(new CandleWax);
    addMetaObject<CandleWaxCard>();

    General *kureha = new General(this, "kureha", "citizen", 3, false);
    kureha->addSkill(new MedicalExpertise);
    kureha->addSkill(new WitheredFlower);
}

ADD_PACKAGE(Alabastan)
