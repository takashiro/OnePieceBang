#ifndef ROOMTHREAD_H
#define ROOMTHREAD_H

#include <QThread>
#include <QSemaphore>
#include <QVariant>

#include <csetjmp>

#include "structs.h"

struct LogMessage{
	LogMessage();
	QString toString() const;

	QString type;
	ServerPlayer *from;
	QList<ServerPlayer *> to;
	QString card_str;
	QString arg;
	QString arg2;
};

class EventTriplet{
public:
	EventTriplet(TriggerEvent event, ServerPlayer *target, QVariant *data)
		:event(event), target(target), data(data){}
	QString toString() const;

private:
	TriggerEvent event;
	ServerPlayer *target;
	QVariant *data;
};

class RoomThread : public QThread{
	Q_OBJECT

public:
	explicit RoomThread(Room *room);
	~RoomThread();
	void constructTriggerTable();
	bool trigger(TriggerEvent event, ServerPlayer *target, QVariant &data);
	bool trigger(TriggerEvent event, ServerPlayer *target);

	void addPlayerSkills(ServerPlayer *player, bool invoke_game_start = false);

	void addTriggerSkill(const TriggerSkill *skill);
	void delay(unsigned long msecs = 1000);
	void run3v3();
	void action3v3(ServerPlayer *player);

	const QList<EventTriplet> *getEventStack() const;

protected:
	virtual void run();

private:
	Room *room;
	QString order;
	GameRule *game_rule;
	GameRule *basara_mode;

	QList<const TriggerSkill *> skill_table[NumOfEvents];
	QSet<const TriggerSkill *> skillSet;

	QList<EventTriplet> event_stack;
};

#endif // ROOMTHREAD_H
