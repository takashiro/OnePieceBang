#ifndef TEST_H
#define TEST_H

#include "package.h"
#include "card.h"
#include "skill.h"
#include "standard.h"
#include "carditem.h"
#include "engine.h"

class TestPackage : public Package
{
public:
	TestPackage();
};

class OperatingRoomCard: public SkillCard{
	Q_OBJECT

public:
	Q_INVOKABLE OperatingRoomCard();
	virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const;
};

#endif // TEST_H
