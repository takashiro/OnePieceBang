#ifndef SKYPIEA_H
#define SKYPIEA_H

#include "package.h"
#include "card.h"
#include "standard.h"

class SkypieaPackage: public Package
{
public:
	SkypieaPackage();
};

class ThunderBotCard: public SkillCard{
	Q_OBJECT

public:
	Q_INVOKABLE ThunderBotCard();
	bool targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const;
	bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
	void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class BallDragonCard: public SkillCard{
	Q_OBJECT

public:
	Q_INVOKABLE BallDragonCard();
	bool targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const;
	bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
	void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

#endif // SKYPIEA_H
