#include "standard.h"
#include "standard-equips.h"
#include "general.h"
#include "engine.h"
#include "client.h"
#include "room.h"
#include "carditem.h"

Slash::Slash(Suit suit, int number): BasicCard(suit, number)
{
    setObjectName("slash");
    nature = DamageStruct::Normal;
}

DamageStruct::Nature Slash::getNature() const{
    return nature;
}

void Slash::setNature(DamageStruct::Nature nature){
    this->nature = nature;
}

bool Slash::IsAvailable(const Player *player){
    if(player->hasFlag("slash_forbidden"))
        return false;

    return player->hasWeapon("crossbow") || player->canSlashWithoutCrossbow();
}

bool Slash::isAvailable(const Player *player) const{
    return IsAvailable(player) && BasicCard::isAvailable(player);;
}

QString Slash::getSubtype() const{
    return "attack_card";
}

void Slash::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    BasicCard::use(room, source, targets);

    if(source->hasFlag("drank")){
        LogMessage log;
        log.type = "#UnsetDrank";
        log.from = source;
        room->sendLog(log);
    }
}

void Slash::onEffect(const CardEffectStruct &card_effect) const{
    Room *room = card_effect.from->getRoom();
    if(card_effect.from->hasFlag("drank")){
        room->setCardFlag(this, "drank");
        room->setPlayerFlag(card_effect.from, "-drank");
    }

    SlashEffectStruct effect;
    effect.from = card_effect.from;
    effect.nature = nature;
    effect.slash = this;

    effect.to = card_effect.to;
    effect.drank = this->hasFlag("drank");

    room->slashEffect(effect);
}

bool Slash::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
    return !targets.isEmpty();
}

bool Slash::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    int slash_targets = 1 + Self->getMark("@#slash_extra_targets");
    if(targets.length() >= slash_targets)
        return false;

    bool distance_limit = !Self->hasFlag("@slash_distance_unlimited");

    return Self->canSlash(to_select, distance_limit);
}

Jink::Jink(Suit suit, int number):BasicCard(suit, number){
    setObjectName("jink");

    target_fixed = true;
}

QString Jink::getSubtype() const{
    return "defense_card";
}

bool Jink::isAvailable(const Player *) const{
    return false;
}

Vulnerary::Vulnerary(Suit suit, int number):BasicCard(suit, number){
    setObjectName("vulnerary");
    target_fixed = true;
}

QString Vulnerary::getSubtype() const{
    return "recover_card";
}

QString Vulnerary::getEffectPath(bool ) const{
    return Card::getEffectPath();
}

void Vulnerary::onUse(Room *room, const CardUseStruct &card_use) const{
    CardUseStruct use = card_use;
    if(use.to.isEmpty()){
        use.to << use.from;
    }
    BasicCard::onUse(room, use);
}

void Vulnerary::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    room->throwCard(this);

    foreach(ServerPlayer *tmp, targets)
        room->cardEffect(this, source, tmp);
}

void Vulnerary::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    // do animation
    room->broadcastInvoke("animate", QString("vulnerary:%1:%2")
                          .arg(effect.from->objectName())
                          .arg(effect.to->objectName()));

    // recover hp
    RecoverStruct recover;
    recover.card = this;
    recover.who = effect.from;

    room->recover(effect.to, recover);
}

bool Vulnerary::isAvailable(const Player *player) const{
    return player->isWounded();
}

Crossbow::Crossbow(Suit suit, int number)
    :Weapon(suit, number, 1)
{
    setObjectName("crossbow");
}

class DoubleSwordSkill: public WeaponSkill{
public:
    DoubleSwordSkill():WeaponSkill("double_sword"){
        events << SlashEffect;
    }

    virtual bool trigger(TriggerEvent, ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        Room *room = player->getRoom();

        if(effect.from->getGeneral()->isMale() != effect.to->getGeneral()->isMale()){
            if(effect.from->askForSkillInvoke(objectName())){
                bool draw_card = false;

                if(effect.to->isKongcheng())
                    draw_card = true;
                else{
                    QString prompt = "double-sword-card:" + effect.from->getGeneralName();
                    const Card *card = room->askForCard(effect.to, ".", prompt, QVariant(), CardDiscarded);
                    if(card){
                        room->throwCard(card);
                    }else
                        draw_card = true;
                }

                if(draw_card)
                    effect.from->drawCards(1);
            }
        }

        return false;
    }
};

DoubleSword::DoubleSword(Suit suit, int number)
    :Weapon(suit, number, 2)
{
    setObjectName("double_sword");
    skill = new DoubleSwordSkill;
}

class QinggangSwordSkill: public WeaponSkill{
public:
    QinggangSwordSkill():WeaponSkill("qinggang_sword"){
        events << SlashEffect;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        effect.to->addMark("qinggang");

        return false;
    }
};

QinggangSword::QinggangSword(Suit suit, int number)
    :Weapon(suit, number, 2)
{
    setObjectName("qinggang_sword");

    skill = new QinggangSwordSkill;
}

class BladeSkill : public WeaponSkill{
public:
    BladeSkill():WeaponSkill("blade"){
        events << SlashMissed;
    }

    virtual int getPriority() const{
        return -1;
    }

    virtual bool trigger(TriggerEvent, ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();

        if(effect.to->hasSkill("kongcheng") && effect.to->isKongcheng())
            return false;

        Room *room = player->getRoom();
        const Card *card = room->askForCard(player, "slash", "blade-slash:" + effect.to->objectName(), QVariant(), NonTrigger);
        if(card){
            // if player is drank, unset his flag
            if(player->hasFlag("drank"))
                room->setPlayerFlag(player, "-drank");

            CardUseStruct use;
            use.card = card;
            use.from = player;
            use.to << effect.to;
            room->useCard(use, false);
        }

        return false;
    }
};

Blade::Blade(Suit suit, int number)
    :Weapon(suit, number, 3)
{
    setObjectName("blade");
    skill = new BladeSkill;
}

class SpearSkill: public ViewAsSkill{
public:
    SpearSkill():ViewAsSkill("spear"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return  pattern == "slash";
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        return selected.length() < 2 && !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.length() != 2)
            return NULL;

        const Card *first = cards.at(0)->getFilteredCard();
        const Card *second = cards.at(1)->getFilteredCard();

        Card::Suit suit = Card::NoSuit;
        if(first->isBlack() && second->isBlack())
            suit = Card::Spade;
        else if(first->isRed() && second->isRed())
            suit = Card::Heart;

        Slash *slash = new Slash(suit, 0);
        slash->setSkillName(objectName());
        slash->addSubcard(first);
        slash->addSubcard(second);

        return slash;
    }
};

Spear::Spear(Suit suit, int number)
    :Weapon(suit, number, 3)
{
    setObjectName("spear");
    attach_skill = true;
}

class AxeViewAsSkill: public ViewAsSkill{
public:
    AxeViewAsSkill():ViewAsSkill("axe"){

    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "@axe";
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        if(selected.length() >= 2)
            return false;

        if(to_select->getCard() == Self->getWeapon())
            return false;

        return true;
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.length() != 2)
            return NULL;

        DummyCard *card = new DummyCard;
        card->setSkillName(objectName());
        card->addSubcards(cards);
        return card;
    }
};

class AxeSkill: public WeaponSkill{
public:
    AxeSkill():WeaponSkill("axe"){
        events << SlashMissed;
    }

    virtual bool trigger(TriggerEvent, ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();

        Room *room = player->getRoom();
        CardStar card = room->askForCard(player, "@axe", "@axe:" + effect.to->objectName(),data, CardDiscarded);
        if(card){
            QList<int> card_ids = card->getSubcards();
            foreach(int card_id, card_ids){
                LogMessage log;
                log.type = "$DiscardCard";
                log.from = player;
                log.card_str = QString::number(card_id);

                room->sendLog(log);
            }

            LogMessage log;
            log.type = "#AxeSkill";
            log.from = player;
            log.to << effect.to;
            log.arg = objectName();
            room->sendLog(log);

            room->slashResult(effect, NULL);
        }

        return false;
    }
};

Axe::Axe(Suit suit, int number)
    :Weapon(suit, number, 3)
{
    setObjectName("axe");
    skill = new AxeSkill;
    attach_skill = true;
}

Halberd::Halberd(Suit suit, int number)
    :Weapon(suit, number, 4)
{
    setObjectName("halberd");
}

class KylinBowSkill: public WeaponSkill{
public:
    KylinBowSkill():WeaponSkill("kylin_bow"){
        events << DamageProceed;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();

        QStringList horses;
    if(damage.card && damage.card->inherits("Slash") && !damage.chain){
        if(damage.to->getDefensiveHorse())
            horses << "dhorse";
        if(damage.to->getOffensiveHorse())
            horses << "ohorse";

        if(horses.isEmpty())
            return false;

        Room *room = player->getRoom();
        if(!player->askForSkillInvoke(objectName(), data))
            return false;

        QString horse_type;
        if(horses.length() == 2)
            horse_type = room->askForChoice(player, objectName(), horses.join("+"));
        else
            horse_type = horses.first();

        if(horse_type == "dhorse")
            room->throwCard(damage.to->getDefensiveHorse(), damage.to);
        else if(horse_type == "ohorse")
            room->throwCard(damage.to->getOffensiveHorse(), damage.to);
    }

        return false;
    }
};

KylinBow::KylinBow(Suit suit, int number)
    :Weapon(suit, number, 5)
{
    setObjectName("kylin_bow");
    skill = new KylinBowSkill;
}

class EightDiagramSkill: public ArmorSkill{
private:
    EightDiagramSkill():ArmorSkill("eight_diagram"){
        events << CardAsked;
    }

public:
    static EightDiagramSkill *GetInstance(){
        static EightDiagramSkill *instance = NULL;
        if(instance == NULL)
            instance = new EightDiagramSkill;

        return instance;
    }

    virtual int getPriority() const{
        return 2;
    }

    virtual bool trigger(TriggerEvent, ServerPlayer *player, QVariant &data) const{
        QString asked = data.toString();
        if(asked == "jink"){
            Room *room = player->getRoom();
                if(room->askForSkillInvoke(player, objectName())){
                JudgeStruct judge;
                judge.pattern = QRegExp("(.*):(heart|diamond):(.*)");
                judge.good = true;
                judge.reason = objectName();
                judge.who = player;

                room->judge(judge);
                if(judge.isGood()){
                    Jink *jink = new Jink(Card::NoSuit, 0);
                    jink->setSkillName(objectName());
                    room->provide(jink);
                    room->setEmotion(player, "good");

                    return true;
                }else
                    room->setEmotion(player, "bad");
            }
        }
        return false;
    }
};



EightDiagram::EightDiagram(Suit suit, int number)
    :Armor(suit, number){
    setObjectName("eight_diagram");
    skill = EightDiagramSkill::GetInstance();
}

AmazingGrace::AmazingGrace(Suit suit, int number)
    :GlobalEffect(suit, number)
{
    setObjectName("amazing_grace");
}

void AmazingGrace::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    room->throwCard(this);

    QList<ServerPlayer *> players = targets.isEmpty() ? room->getAllPlayers() : targets;
    QList<int> card_ids = room->getNCards(players.length());
    room->fillAG(card_ids);

    QVariantList ag_list;
    foreach(int card_id, card_ids){
        room->setCardFlag(card_id, "visible");
        ag_list << card_id;
    }
    room->setTag("AmazingGrace", ag_list);

    GlobalEffect::use(room, source, players);

    ag_list = room->getTag("AmazingGrace").toList();

    // throw the rest cards
    foreach(QVariant card_id, ag_list){
        room->takeAG(NULL, card_id.toInt());
    }

    room->broadcastInvoke("clearAG");
}

void AmazingGrace::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    QVariantList ag_list = room->getTag("AmazingGrace").toList();
    QList<int> card_ids;
    foreach(QVariant card_id, ag_list)
        card_ids << card_id.toInt();

    int card_id = room->askForAG(effect.to, card_ids, false, objectName());
    card_ids.removeOne(card_id);

    room->takeAG(effect.to, card_id);
    ag_list.removeOne(card_id);

    room->setTag("AmazingGrace", ag_list);
}

AllBlue::AllBlue(Suit suit, int number)
    :GlobalEffect(suit, number)
{
    setObjectName("all_blue");
}

bool AllBlue::isCancelable(const CardEffectStruct &effect) const{
    return effect.to->isWounded();
}

void AllBlue::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    RecoverStruct recover;
    recover.card = this;
    recover.who = effect.from;
    room->recover(effect.to, recover);
}

NeptunianAttack::NeptunianAttack(Suit suit, int number)
    :AOE(suit, number)
{
    setObjectName("neptunian_attack");
}

void NeptunianAttack::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    const Card *slash = room->askForCard(effect.to, "slash", "neptunian-attack-slash:" + effect.from->objectName());
    if(slash)
        room->setEmotion(effect.to, "killer");
    else{
        DamageStruct damage;
        damage.card = this;
        damage.damage = 1;
        damage.to = effect.to;
        damage.nature = DamageStruct::Normal;

        if(effect.from->isAlive())
            damage.from = effect.from;
        else
            damage.from = NULL;

        room->damage(damage);
    }
}

HaouHaki::HaouHaki(Card::Suit suit, int number)
    :AOE(suit, number)
{
    setObjectName("haou_haki");
}

void HaouHaki::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    const Card *jink = room->askForCard(effect.to, "jink", "haou-haki-jink:" + effect.from->objectName());
    if(jink)
        room->setEmotion(effect.to, "jink");
    else{
        DamageStruct damage;
        damage.card = this;
        damage.damage = 1;
        if(effect.from->isAlive())
            damage.from = effect.from;
        else
            damage.from = NULL;
        damage.to = effect.to;
        damage.nature = DamageStruct::Normal;

        room->damage(damage);
    }
}

void SingleTargetTrick::onUse(Room *room, const CardUseStruct &card_use) const{
    CardUseStruct use = card_use;
    if(use.to.isEmpty()){
        use.to << use.from;
    }
    TrickCard::onUse(room, use);
}

void SingleTargetTrick::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    room->throwCard(this);

    CardEffectStruct effect;
    effect.card = this;
    effect.from = source;
    if(!targets.isEmpty()){
        foreach(ServerPlayer *tmp, targets){
            effect.to = tmp;
            room->cardEffect(effect);
        }
    }
}

Collateral::Collateral(Card::Suit suit, int number)
    :SingleTargetTrick(suit, number, false)
{
    setObjectName("collateral");
}

bool Collateral::isAvailable(const Player *player) const{
    foreach(const Player *p, player->getSiblings()){
        if(p->getWeapon() && p->isAlive())
            return true;
    }

    return false;
}

bool Collateral::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
    return targets.length() == 2;
}

bool Collateral::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(targets.isEmpty()){
        if(to_select->hasSkill("weimu") && isBlack())
            return false;

        return to_select->getWeapon() && to_select != Self;
    }else if(targets.length() == 1){
        const Player *first = targets.first();
        return first != Self && first->getWeapon() && first->canSlash(to_select);
    }else
        return false;
}

void Collateral::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    room->throwCard(this);

    ServerPlayer *killer = targets.at(0);
    QList<ServerPlayer *> victims = targets;
    if(victims.length() > 1)
        victims.removeAt(0);
    const Weapon *weapon = killer->getWeapon();

    if(weapon == NULL)
        return;

    bool on_effect = room->cardEffect(this, source, killer);
    if(on_effect){
        QString prompt = QString("collateral-slash:%1:%2")
                .arg(source->objectName()).arg(victims.first()->objectName());
        const Card *slash = room->askForCard(killer, "slash", prompt, QVariant(), NonTrigger);
        if (victims.first()->isDead()){
            if (source->isDead()){
                if(killer->isAlive() && killer->getWeapon()){
                    int card_id = weapon->getId();
                    room->throwCard(card_id, source);
                }
            }
            else
            {
                if(killer->isAlive() && killer->getWeapon()){
                    source->obtainCard(weapon);
                }
            }
        }
        if (source->isDead()){
            if (killer->isAlive()){
                if(slash){
                    CardUseStruct use;
                    use.card = slash;
                    use.from = killer;
                    use.to = victims;
                    room->useCard(use);
                }
                else{
                    if(killer->getWeapon()){
                        int card_id = weapon->getId();
                        room->throwCard(card_id, source);
                    }
                }
            }
        }
        else{
            if(killer->isDead()) ;
            else{
                if(slash){
                    CardUseStruct use;
                    use.card = slash;
                    use.from = killer;
                    use.to = victims;
                    room->useCard(use);
                }
                else{
                    if(killer->getWeapon())
                        source->obtainCard(weapon);
                }
            }
        }
    }
}

Nullification::Nullification(Suit suit, int number)
    :SingleTargetTrick(suit, number, false)
{
    setObjectName("nullification");
}

void Nullification::use(Room *room, ServerPlayer *, const QList<ServerPlayer *> &) const{
    // does nothing, just throw it
    room->throwCard(this);
}

bool Nullification::isAvailable(const Player *) const{
    return false;
}

TreasureChest::TreasureChest(Suit suit, int number)
    :SingleTargetTrick(suit, number, false)
{
    setObjectName("treasure_chest");
    target_fixed = true;
}

void TreasureChest::onEffect(const CardEffectStruct &effect) const{
    effect.to->drawCards(2);
}

Duel::Duel(Suit suit, int number)
    :SingleTargetTrick(suit, number, true)
{
    setObjectName("duel");
}

bool Duel::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(to_select->hasSkill("kongcheng") && to_select->isKongcheng())
        return false;

    if(to_select == Self)
        return false;

    return targets.isEmpty();
}

void Duel::onEffect(const CardEffectStruct &effect) const{
    ServerPlayer *first = effect.to;
    ServerPlayer *second = effect.from;
    Room *room = first->getRoom();

    room->setEmotion(first, "duel-a");
    room->setEmotion(second, "duel-b");

    forever{
        if(second->hasSkill("wushuang")){
            room->playSkillEffect("wushuang");
            const Card *slash = room->askForCard(first, "slash", "@wushuang-slash-1:" + second->objectName());
            if(slash == NULL)
                break;

            slash = room->askForCard(first, "slash", "@wushuang-slash-2:" + second->objectName());
            if(slash == NULL)
                break;

        }else{
            const Card *slash = room->askForCard(first, "slash", "duel-slash:" + second->objectName());
            if(slash == NULL)
                break;
        }

        qSwap(first, second);
    }

    DamageStruct damage;
    damage.card = this;
    if(second->isAlive())
        damage.from = second;
    else
        damage.from = NULL;
    damage.to = first;

    room->damage(damage);
}

Snatch::Snatch(Suit suit, int number):SingleTargetTrick(suit, number, true) {
    setObjectName("snatch");
}

bool Snatch::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!targets.isEmpty())
        return false;

    if(to_select->isAllNude())
        return false;

    if(to_select == Self)
        return false;

    if(Self->distanceTo(to_select) > 1 && !Self->hasSkill("qicai"))
        return false;

    return true;
}

void Snatch::onEffect(const CardEffectStruct &effect) const{
    if(effect.from->isDead())
        return;
    if(effect.to->isAllNude())
        return;

    Room *room = effect.to->getRoom();
    int card_id = room->askForCardChosen(effect.from, effect.to, "hej", objectName());

    room->obtainCard(effect.from, card_id, room->getCardPlace(card_id) != Player::Hand);
}

Dismantlement::Dismantlement(Suit suit, int number)
    :SingleTargetTrick(suit, number, false) {
    setObjectName("dismantlement");
}

bool Dismantlement::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!targets.isEmpty())
        return false;

    if(to_select->isAllNude())
        return false;

    if(to_select == Self)
        return false;

    return true;
}

void Dismantlement::onEffect(const CardEffectStruct &effect) const{
    if(effect.from->isDead())
        return;
    if(effect.to->isAllNude())
        return;

    Room *room = effect.to->getRoom();
    int card_id = room->askForCardChosen(effect.from, effect.to, "hej", objectName());
    room->throwCard(card_id, room->getCardPlace(card_id) == Player::Judging ? NULL : effect.to);

    LogMessage log;
    log.type = "$Dismantlement";
    log.from = effect.to;
    log.card_str = QString::number(card_id);
    room->sendLog(log);
}

Indulgence::Indulgence(Suit suit, int number)
    :DelayedTrick(suit, number)
{
    setObjectName("indulgence");
    target_fixed = false;

    judge.pattern = QRegExp("(.*):(heart):(.*)");
    judge.good = true;
    judge.reason = objectName();
}

bool Indulgence::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if(!targets.isEmpty())
        return false;

    if(to_select->containsTrick(objectName()))
        return false;

    if(to_select == Self)
        return false;

    return true;
}

void Indulgence::takeEffect(ServerPlayer *target) const{
    target->clearHistory();
    target->skip(Player::Play);
}

Disaster::Disaster(Card::Suit suit, int number)
    :DelayedTrick(suit, number, true)
{
    target_fixed = true;
}

bool Disaster::isAvailable(const Player *player) const{
    if(player->containsTrick(objectName()))
        return false;

    return !player->isProhibited(player, this);
}

Lightning::Lightning(Suit suit, int number):Disaster(suit, number){
    setObjectName("lightning");

    judge.pattern = QRegExp("(.*):(spade):([2-9])");
    judge.good = false;
    judge.reason = objectName();
}

void Lightning::takeEffect(ServerPlayer *target) const{
    DamageStruct damage;
    damage.card = this;
    damage.damage = 3;
    damage.from = NULL;
    damage.to = target;
    damage.nature = DamageStruct::Thunder;

    target->getRoom()->damage(damage);
}

Rain::Rain(Suit suit, int number):Disaster(suit, number){
    setObjectName("rain");

    judge.pattern = QRegExp("(.*):(heart):([2-9])");
    judge.good = false;
    judge.reason = objectName();
}

void Rain::takeEffect(ServerPlayer *target) const{
    Room *room = target->getRoom();
    room->acquireSkill(target, "raineffect", false);
}

Tornado::Tornado(Suit suit, int number):Disaster(suit, number){
    setObjectName("tornado");

    judge.pattern = QRegExp("(.*):(diamond):([2-9])");
    judge.good = false;
    judge.reason = objectName();
}

void Tornado::takeEffect(ServerPlayer *target) const{
    target->throwAllEquips();
}

// EX cards

class IceSwordSkill: public WeaponSkill{
public:
    IceSwordSkill():WeaponSkill("ice_sword"){
        events << DamageProceed;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();

        Room *room = player->getRoom();

        if(damage.card && damage.card->inherits("Slash") && !damage.to->isNude()
                && !damage.chain && player->askForSkillInvoke("ice_sword", data)){
            int card_id = room->askForCardChosen(player, damage.to, "he", "ice_sword");
            room->throwCard(card_id, damage.to);

            if(!damage.to->isNude()){
                card_id = room->askForCardChosen(player, damage.to, "he", "ice_sword");
                room->throwCard(card_id, damage.to);
            }

            return true;
        }

        return false;
    }
};

IceSword::IceSword(Suit suit, int number)
    :Weapon(suit, number, 2)
{
    setObjectName("ice_sword");
    skill = new IceSwordSkill;
}

class RenwangShieldSkill: public ArmorSkill{
public:
    RenwangShieldSkill():ArmorSkill("renwang_shield"){
        events << SlashEffected;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if(effect.slash->isBlack()){
            LogMessage log;
            log.type = "#ArmorNullify";
            log.from = player;
            log.arg = objectName();
            log.arg2 = effect.slash->objectName();
            player->getRoom()->sendLog(log);

            return true;
        }else
            return false;
    }
};

RenwangShield::RenwangShield(Suit suit, int number)
    :Armor(suit, number)
{
    setObjectName("renwang_shield");
    skill = new RenwangShieldSkill;
}

class HorseSkill: public DistanceSkill{
public:
    HorseSkill():DistanceSkill("horse"){

    }

    virtual int getCorrect(const Player *from, const Player *to) const{
        int correct = 0;
        if(from->getOffensiveHorse())
            correct += from->getOffensiveHorse()->getCorrect();
        if(to->getDefensiveHorse())
            correct += to->getDefensiveHorse()->getCorrect();

        return correct;
    }
};

class RainEffect: public FilterSkill{
public:
    RainEffect():FilterSkill("raineffect"){

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getCard()->getSuit() == Card::Heart;
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getCard();
        Card *new_card = Card::Clone(card);
        if(new_card) {
            new_card->setSuit(Card::Spade);
            new_card->setSkillName(objectName());
            return new_card;
        }else
            return card;
    }
};

class RainEffectEx: public TriggerSkill{
public:
    RainEffectEx():TriggerSkill("#raineffect-extra"){
        frequency = Compulsory;
        events << FinishJudge << PhaseChange;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(event == FinishJudge){
            JudgeStar judge = data.value<JudgeStar>();
            if(judge->card->getSuit() == Card::Heart){
                Card *new_card = Card::Clone(judge->card);
                new_card->setSuit(Card::Spade);
                new_card->setSkillName("rain");
                judge->card = new_card;

                player->getRoom()->sendLog("#RainEffectRetrialResult", player);
            }
        }else if(event == PhaseChange){
            if(player->getPhase() == Player::NotActive){
                Room *room = player->getRoom();
                room->detachSkillFromPlayer(player, "raineffect");
            }
        }

        return false;
    }
};


NatureSlash::NatureSlash(Suit suit, int number, DamageStruct::Nature nature)
    :Slash(suit, number)
{
    this->nature = nature;
}

bool NatureSlash::match(const QString &pattern) const{
    if(pattern == "slash")
        return true;
    else
        return Slash::match(pattern);
}

ThunderSlash::ThunderSlash(Suit suit, int number)
    :NatureSlash(suit, number, DamageStruct::Thunder)
{
    setObjectName("thunder_slash");
}

FireSlash::FireSlash(Suit suit, int number)
    :NatureSlash(suit, number, DamageStruct::Fire)
{
    setObjectName("fire_slash");
    nature = DamageStruct::Fire;
}

BusouHaki::BusouHaki(Card::Suit suit, int number)
    :SingleTargetTrick(suit, number, false)
{
    setObjectName("busou_haki");
    target_fixed = true;
    once = true;
}

void BusouHaki::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    LogMessage log;
    log.type = "#Drank";
    log.from = effect.to;
    room->sendLog(log);

    room->setPlayerFlag(effect.to, "drank");
}

class FireFanSkill: public WeaponSkill{
public:
    FireFanSkill():WeaponSkill("fan"){
        events << SlashEffect;
    }
    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if(!effect.slash->getSkillName().isEmpty() && effect.slash->getSubcards().length() > 0)
            return false;
        if(effect.nature == DamageStruct::Normal){
            if(player->getRoom()->askForSkillInvoke(player, objectName(), data)){
                effect.nature = DamageStruct::Fire;
                data = QVariant::fromValue(effect);
            }
        }
        return false;
    }
};

class FanSkill: public OneCardViewAsSkill{
public:
    FanSkill():OneCardViewAsSkill("fan"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern == "slash";
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getFilteredCard()->objectName() == "slash";
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getCard();
        Card *acard = new FireSlash(card->getSuit(), card->getNumber());
        acard->addSubcard(card->getId());
        acard->setSkillName(objectName());
        return acard;
    }
};


Fan::Fan(Suit suit, int number):Weapon(suit, number, 4){
    setObjectName("fan");
    skill = new FireFanSkill;
    attach_skill = true;
}

class GudingBladeSkill: public WeaponSkill{
public:
    GudingBladeSkill():WeaponSkill("guding_blade"){
        events << DamageProceed;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.card && damage.card->inherits("Slash") &&
            damage.to->isKongcheng() && !damage.chain)
        {
            Room *room = damage.to->getRoom();

            LogMessage log;
            log.type = "#GudingBladeEffect";
            log.from = player;
            log.to << damage.to;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(damage.damage + 1);
            room->sendLog(log);

            damage.damage ++;
            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

GudingBlade::GudingBlade(Suit suit, int number):Weapon(suit, number, 2){
    setObjectName("guding_blade");
    skill = new GudingBladeSkill;
}

class VineSkill: public ArmorSkill{
public:
    VineSkill():ArmorSkill("vine"){
        events << Predamaged << SlashEffected << CardEffected;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(event == SlashEffected){
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if(effect.nature == DamageStruct::Normal){
                LogMessage log;
                log.from = player;
                log.type = "#ArmorNullify";
                log.arg = objectName();
                log.arg2 = effect.slash->objectName();
                player->getRoom()->sendLog(log);

                return true;
            }
        }else if(event == CardEffected){
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if(effect.card->inherits("AOE")){
                LogMessage log;
                log.from = player;
                log.type = "#ArmorNullify";
                log.arg = objectName();
                log.arg2 = effect.card->objectName();
                player->getRoom()->sendLog(log);

                return true;
            }
        }else if(event == Predamaged){
            DamageStruct damage = data.value<DamageStruct>();
            if(damage.nature == DamageStruct::Fire){
                LogMessage log;
                log.type = "#VineDamage";
                log.from = player;
                log.arg = QString::number(damage.damage);
                log.arg2 = QString::number(damage.damage + 1);
                player->getRoom()->sendLog(log);

                damage.damage ++;
                data = QVariant::fromValue(damage);
            }
        }

        return false;
    }
};

Vine::Vine(Suit suit, int number):Armor(suit, number){
    setObjectName("vine");
    skill = new VineSkill;
}

class SilverLionSkill: public ArmorSkill{
public:
    SilverLionSkill():ArmorSkill("silver_lion"){
        events << Predamaged;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.damage > 1){
            LogMessage log;
            log.type = "#SilverLion";
            log.from = player;
            log.arg = QString::number(damage.damage);
            log.arg2 = objectName();
            player->getRoom()->sendLog(log);

            damage.damage = 1;
            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

SilverLion::SilverLion(Suit suit, int number):Armor(suit, number){
    setObjectName("silver_lion");
    skill = new SilverLionSkill;
}

void SilverLion::onUninstall(ServerPlayer *player) const{
    if(player->isAlive() && player->getMark("qinggang") == 0){
        RecoverStruct recover;
        recover.card = this;
        player->getRoom()->recover(player, recover);
    }
}

FireAttack::FireAttack(Card::Suit suit, int number)
    :SingleTargetTrick(suit, number, true)
{
    setObjectName("fire_attack");
}

bool FireAttack::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!targets.isEmpty())
        return false;

    if(to_select->isKongcheng())
        return false;

    if(to_select == Self)
        return Self->getHandcardNum() >= 2;
    else
        return true;
}

void FireAttack::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    if(effect.to->isKongcheng())
        return;

    const Card *card = room->askForCardShow(effect.to, effect.from, objectName());
    room->showCard(effect.to, card->getEffectiveId());

    QString suit_str = card->getSuitString();
    QString pattern = QString(".%1").arg(suit_str.at(0).toUpper());
    QString prompt = QString("@fire-attack:%1::%2").arg(effect.to->getGeneralName()).arg(suit_str);
    if(room->askForCard(effect.from, pattern, prompt, QVariant(), CardDiscarded)){
        DamageStruct damage;
        damage.card = this;
        damage.from = effect.from;
        damage.to = effect.to;
        damage.nature = DamageStruct::Fire;

        room->damage(damage);
    }

    if(card->isVirtualCard())
        delete card;
}

IronChain::IronChain(Card::Suit suit, int number)
    :TrickCard(suit, number, false)
{
    setObjectName("iron_chain");
}

QString IronChain::getSubtype() const{
    return "damage_spread";
}

QString IronChain::getEffectPath(bool is_male) const{
    return QString();
}

bool IronChain::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(targets.length() >= 2)
        return false;

    return true;
}

bool IronChain::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    if(getSkillName() == "guhuo")
        return targets.length() == 1 || targets.length() == 2;
    else
        return targets.length() <= 2;
}

void IronChain::onUse(Room *room, const CardUseStruct &card_use) const{
    if(card_use.to.isEmpty()){
        room->throwCard(this);
        card_use.from->playCardEffect("@recast");
        card_use.from->drawCards(1);
    }else
        TrickCard::onUse(room, card_use);
}

void IronChain::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    room->throwCard(this);

    source->playCardEffect("@tiesuo");
    TrickCard::use(room, source, targets);
}

void IronChain::onEffect(const CardEffectStruct &effect) const{
    bool chained = ! effect.to->isChained();
    effect.to->setChained(chained);

    effect.to->getRoom()->broadcastProperty(effect.to, "chained");
    effect.to->getRoom()->setEmotion(effect.to, "chain");
}

SupplyShortage::SupplyShortage(Card::Suit suit, int number)
    :DelayedTrick(suit, number)
{
    setObjectName("supply_shortage");

    judge.pattern = QRegExp("(.*):(club):(.*)");
    judge.good = true;
    judge.reason = objectName();
}

bool SupplyShortage::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!targets.isEmpty())
        return false;

    if(to_select == Self)
        return false;

    if(to_select->containsTrick(objectName()))
        return false;

    if(Self->hasSkill("qicai"))
        return true;

    int distance = Self->distanceTo(to_select);
    if(Self->hasSkill("duanliang"))
        return distance <= 2;
    else
        return distance <= 1;
}

void SupplyShortage::takeEffect(ServerPlayer *target) const{
    target->skip(Player::Draw);
}

StandardCardPackage::StandardCardPackage()
    :Package("standard_cards")
{
    type = Package::CardPack;

    QList<Card*> cards;

    cards << new Slash(Card::Spade, 7)
          << new Slash(Card::Spade, 8)
          << new Slash(Card::Spade, 8)
          << new Slash(Card::Spade, 9)
          << new Slash(Card::Spade, 9)
          << new Slash(Card::Spade, 10)
          << new Slash(Card::Spade, 10)

          << new Slash(Card::Club, 2)
          << new Slash(Card::Club, 3)
          << new Slash(Card::Club, 4)
          << new Slash(Card::Club, 5)
          << new Slash(Card::Club, 6)
          << new Slash(Card::Club, 7)
          << new Slash(Card::Club, 8)
          << new Slash(Card::Club, 8)
          << new Slash(Card::Club, 9)
          << new Slash(Card::Club, 9)
          << new Slash(Card::Club, 10)
          << new Slash(Card::Club, 10)
          << new Slash(Card::Club, 11)
          << new Slash(Card::Club, 11)

          << new Slash(Card::Heart, 10)
          << new Slash(Card::Heart, 10)
          << new Slash(Card::Heart, 11)

          << new Slash(Card::Diamond, 6)
          << new Slash(Card::Diamond, 7)
          << new Slash(Card::Diamond, 8)
          << new Slash(Card::Diamond, 9)
          << new Slash(Card::Diamond, 10)
          << new Slash(Card::Diamond, 13)

          << new Jink(Card::Heart, 2)
          << new Jink(Card::Heart, 2)
          << new Jink(Card::Heart, 13)

          << new Jink(Card::Diamond, 2)
          << new Jink(Card::Diamond, 2)
          << new Jink(Card::Diamond, 3)
          << new Jink(Card::Diamond, 4)
          << new Jink(Card::Diamond, 5)
          << new Jink(Card::Diamond, 6)
          << new Jink(Card::Diamond, 7)
          << new Jink(Card::Diamond, 8)
          << new Jink(Card::Diamond, 9)
          << new Jink(Card::Diamond, 10)
          << new Jink(Card::Diamond, 11)
          << new Jink(Card::Diamond, 11)

          << new Vulnerary(Card::Heart, 3)
          << new Vulnerary(Card::Heart, 4)
          << new Vulnerary(Card::Heart, 6)
          << new Vulnerary(Card::Heart, 7)
          << new Vulnerary(Card::Heart, 8)
          << new Vulnerary(Card::Heart, 9)
          << new Vulnerary(Card::Heart, 12)

          << new Vulnerary(Card::Diamond, 12)

          << new Crossbow(Card::Club)
          << new Crossbow(Card::Diamond)
          << new DoubleSword
          << new QinggangSword
          << new Blade
          << new Spear
          << new Axe
          << new Halberd
          << new KylinBow

          << new EightDiagram(Card::Spade)
          << new EightDiagram(Card::Club);

    skills << EightDiagramSkill::GetInstance();

    {
        QList<Card *> horses;
        horses << new DefensiveHorse(Card::Spade, 5)
               << new DefensiveHorse(Card::Club, 5)
               << new DefensiveHorse(Card::Heart, 13)
               << new OffensiveHorse(Card::Heart, 5)
               << new OffensiveHorse(Card::Spade, 13)
               << new OffensiveHorse(Card::Diamond, 13);

        horses.at(0)->setObjectName("jueying");
        horses.at(1)->setObjectName("dilu");
        horses.at(2)->setObjectName("zhuahuangfeidian");
        horses.at(3)->setObjectName("chitu");
        horses.at(4)->setObjectName("dayuan");
        horses.at(5)->setObjectName("zixing");

        cards << horses;

        skills << new HorseSkill;
    }

    cards << new AmazingGrace(Card::Heart, 3)
          << new AmazingGrace(Card::Heart, 4)
          << new AllBlue
          << new NeptunianAttack(Card::Spade, 7)
          << new NeptunianAttack(Card::Spade, 13)
          << new NeptunianAttack(Card::Club, 7)
          << new HaouHaki(Card::Heart, 1)
          << new Duel(Card::Spade, 1)
          << new Duel(Card::Club, 1)
          << new Duel(Card::Diamond, 1)
          << new TreasureChest(Card::Heart, 7)
          << new TreasureChest(Card::Heart, 8)
          << new TreasureChest(Card::Heart, 9)
          << new TreasureChest(Card::Heart, 11)
          << new Snatch(Card::Spade, 3)
          << new Snatch(Card::Spade, 4)
          << new Snatch(Card::Spade, 11)
          << new Snatch(Card::Diamond, 3)
          << new Snatch(Card::Diamond, 4)
          << new Dismantlement(Card::Spade, 3)
          << new Dismantlement(Card::Spade, 4)
          << new Dismantlement(Card::Spade, 12)
          << new Dismantlement(Card::Club, 3)
          << new Dismantlement(Card::Club, 4)
          << new Dismantlement(Card::Heart, 12)
          << new Collateral(Card::Club, 12)
          << new Collateral(Card::Club, 13)
          << new Nullification(Card::Spade, 11)
          << new Nullification(Card::Club, 12)
          << new Nullification(Card::Club, 13)
          << new Indulgence(Card::Spade, 6)
          << new Indulgence(Card::Club, 6)
          << new Indulgence(Card::Heart, 6)
          << new Lightning(Card::Spade, 1)
          << new Lightning(Card::Heart, 12)
          << new Rain(Card::Heart, 1)
          << new Rain(Card::Club, 12)
          << new Tornado(Card::Club, 1)
          << new Tornado(Card::Diamond, 12);

    cards << new IceSword(Card::Spade, 2)
          << new RenwangShield(Card::Club, 2)
          << new Nullification(Card::Diamond, 12);

    // spade
    cards << new GudingBlade(Card::Spade, 1)
            << new Vine(Card::Spade, 2)
            << new BusouHaki(Card::Spade, 3)
            << new ThunderSlash(Card::Spade, 4)
            << new ThunderSlash(Card::Spade, 5)
            << new ThunderSlash(Card::Spade, 6)
            << new ThunderSlash(Card::Spade, 7)
            << new ThunderSlash(Card::Spade, 8)
            << new BusouHaki(Card::Spade, 9)
            << new SupplyShortage(Card::Spade,10)
            << new IronChain(Card::Spade, 11)
            << new IronChain(Card::Spade, 12)
            << new Nullification(Card::Spade, 13);

    // club
    cards << new SilverLion(Card::Club, 1)
            << new Vine(Card::Club, 2)
            << new BusouHaki(Card::Club, 3)
            << new SupplyShortage(Card::Club, 4)
            << new ThunderSlash(Card::Club, 5)
            << new ThunderSlash(Card::Club, 6)
            << new ThunderSlash(Card::Club, 7)
            << new ThunderSlash(Card::Club, 8)
            << new BusouHaki(Card::Club, 9)
            << new IronChain(Card::Club, 10)
            << new IronChain(Card::Club, 11)
            << new IronChain(Card::Club, 12)
            << new IronChain(Card::Club, 13);

    // heart
    cards << new Nullification(Card::Heart, 1)
            << new FireAttack(Card::Heart, 2)
            << new FireAttack(Card::Heart, 3)
            << new FireSlash(Card::Heart, 4)
            << new Vulnerary(Card::Heart, 5)
            << new Vulnerary(Card::Heart, 6)
            << new FireSlash(Card::Heart, 7)
            << new Jink(Card::Heart, 8)
            << new Jink(Card::Heart, 9)
            << new FireSlash(Card::Heart, 10)
            << new Jink(Card::Heart, 11)
            << new Jink(Card::Heart, 12)
            << new Nullification(Card::Heart, 13);

    // diamond
    cards << new Fan(Card::Diamond, 1)
            << new Vulnerary(Card::Diamond, 2)
            << new Vulnerary(Card::Diamond, 3)
            << new FireSlash(Card::Diamond, 4)
            << new FireSlash(Card::Diamond, 5)
            << new Jink(Card::Diamond, 6)
            << new Jink(Card::Diamond, 7)
            << new Jink(Card::Diamond, 8)
            << new BusouHaki(Card::Diamond, 9)
            << new Jink(Card::Diamond, 10)
            << new Jink(Card::Diamond, 11)
            << new FireAttack(Card::Diamond, 12);

    DefensiveHorse *hualiu = new DefensiveHorse(Card::Diamond, 13);
    hualiu->setObjectName("hualiu");

    cards << hualiu;

    foreach(Card *card, cards)
        card->setParent(this);

    skills << new SpearSkill << new AxeViewAsSkill << new RainEffect << new RainEffectEx << new FanSkill;
    related_skills.insertMulti("raineffect", "#raineffect-extra");
}

ADD_PACKAGE(StandardCard)
