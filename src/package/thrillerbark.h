#ifndef THRILLERBARK_H
#define THRILLERBARK_H

#include "package.h"
#include "card.h"
#include "standard.h"

class ThrillerBarkPackage : public Package
{
public:
	ThrillerBarkPackage();
};

class AcheronCard: public SkillCard{
	Q_OBJECT

public:
	Q_INVOKABLE AcheronCard();
	bool targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const;
	bool targetsFeasible(const QList<const Player *> &targets, const Player *) const;
	void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

#endif // THRILLERBARK_H
