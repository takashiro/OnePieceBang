#ifndef STANDARDGENERALS_H
#define STANDARDGENERALS_H

#include "package.h"
#include "card.h"
#include "skill.h"
#include "standard.h"
#include "carditem.h"
#include "engine.h"
#include "maneuvering.h"

class OuKiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE OuKiCard();
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const;
};

class LieCard: public SingleTargetTrick{
    Q_OBJECT

public:
    Q_INVOKABLE LieCard(Card::Suit suit, int number);
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class TripleSwordsCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE TripleSwordsCard();
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const;
};

#endif // TEST_H
