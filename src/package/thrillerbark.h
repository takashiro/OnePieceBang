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


class GhostRapCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE GhostRapCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#endif // THRILLERBARK_H
