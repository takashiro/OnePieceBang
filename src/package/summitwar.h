#ifndef SUMMITWAR_H
#define SUMMITWAR_H

#include "package.h"
#include "card.h"
#include "standard.h"

class SummitWarPackage : public Package
{
public:
    SummitWarPackage();
};

class MeteorVolcanoCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE MeteorVolcanoCard();
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

#endif // SUMMITWAR_H
