#include "thrillerbark.h"
#include "carditem.h"

class NegativeHorror: public OneCardViewAsSkill{
public:
    NegativeHorror(): OneCardViewAsSkill("negativehorror"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getCardCount(true) > 0;
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        const Card *card = to_select->getCard();
        return card->getSuit() == Card::Club && (card->getTypeId() == Card::Basic || card->getTypeId() == Card::Equip);
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *sub = card_item->getCard();
        Indulgence *indulgence = new Indulgence(sub->getSuit(), sub->getNumber());
        indulgence->addSubcard(sub);
        indulgence->setSkillName(objectName());
        return indulgence;
    }
};

GhostRapCard::GhostRapCard(){
    owner_discarded = true;
}

bool GhostRapCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!targets.isEmpty()){
        return false;
    }

    foreach(const Card *card, to_select->getJudgingArea()){
        const DelayedTrick *trick = DelayedTrick::CastFrom(card);
        if(trick->inherits("Indulgence")){
            return true;
        }
    }

    return false;
}

bool GhostRapCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() == 1;
}

void GhostRapCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    DamageStruct damage = effect.from->tag.value("GhostRapDamage").value<DamageStruct>();
    damage.to = effect.to;
    damage.chain = true;
    room->damage(damage);
}

class GhostRapViewAsSkill: public OneCardViewAsSkill{
public:
    GhostRapViewAsSkill(): OneCardViewAsSkill("#ghostrapviewasskill"){

    }

    virtual bool viewFilter(const CardItem *) const{
        return true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "@@ghostrap";
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        GhostRapCard *card = new GhostRapCard;
        card->addSubcard(card_item->getFilteredCard());
        return card;
    }
};

class GhostRap: public TriggerSkill{
public:
    GhostRap(): TriggerSkill("ghostrap"){
        view_as_skill = new GhostRapViewAsSkill;
        events << Predamaged;
    }

    virtual int getPriority() const{
        return 2;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->getCardCount(true) > 0;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(player == NULL){
            return false;
        }

        Room *room = player->getRoom();
        player->tag["GhostRapDamage"] = data;
        return room->askForUseCard(player, "@@ghostrap", "@ghostrap-card");
    }
};

ThrillerBarkPackage::ThrillerBarkPackage():Package("ThrillerBark")
{
    General *perona = new General(this, "perona", "pirate", 3, false);
    perona->addSkill(new NegativeHorror);
    perona->addSkill(new GhostRap);
    addMetaObject<GhostRapCard>();
}

ADD_PACKAGE(ThrillerBark)
