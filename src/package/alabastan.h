#ifndef ALABASTAN_H
#define ALABASTAN_H

#include "package.h"
#include "card.h"
#include "standard.h"

class AlabastanPackage : public Package
{
public:
	AlabastanPackage();
};

class FleurCard: public SkillCard{
	Q_OBJECT

public:
	Q_INVOKABLE FleurCard();
	virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
	virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
	virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class CandleWaxCard: public SkillCard{
	Q_OBJECT

public:
	Q_INVOKABLE CandleWaxCard();

	virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
	virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
	virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

#endif // ALABASTAN_H
