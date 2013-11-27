#ifndef STANDARDGENERALS_H
#define STANDARDGENERALS_H

#include "package.h"
#include "card.h"
#include "skill.h"
#include "standard.h"
#include "carditem.h"
#include "engine.h"

class RubberPistolCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE RubberPistolCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;

private:
    Slash *slash;
};

class OuKiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE OuKiCard();
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const;
};

class ForecastCard: public SkillCard{
	Q_OBJECT

public:
	Q_INVOKABLE ForecastCard();
};

class LieCard: public SkillCard{
	Q_OBJECT

public:
	Q_INVOKABLE LieCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
	virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
	virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class WaltzCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE WaltzCard();
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const;
};

#endif // TEST_H
