#ifndef ROOMDRIVER_H
#define ROOMDRIVER_H

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

class RoomDriverController;

class RoomDriver: public QObject{
	Q_OBJECT

	friend class RoomDriverController;

public:
	explicit RoomDriver(Room *room);
	~RoomDriver();
	void constructTriggerTable();
	bool trigger(TriggerEvent event, ServerPlayer *target, QVariant &data);
	bool trigger(TriggerEvent event, ServerPlayer *target);

	void addPlayerSkills(ServerPlayer *player, bool invoke_game_start = false);

	void addTriggerSkill(const TriggerSkill *skill);
	void delay(unsigned long msecs = 1000);
	void run3v3();
	void action3v3(ServerPlayer *player);

	const QList<EventTriplet> *getEventStack() const;
	void start();

private slots:
	void quit();

private:
	Room *room;
	QString order;

	QThread thread;
	RoomDriverController *controller;

	QList<const TriggerSkill *> skill_table[NumOfEvents];
	QSet<const TriggerSkill *> skill_set;

	QList<EventTriplet> event_stack;

signals:
	void driver_start();
	void driver_finished();
};

class RoomDriverController: public QObject{
	Q_OBJECT

public:
	RoomDriverController(RoomDriver *driver);

public slots:
	void run();

private:
	RoomDriver *driver;
	Room *room;

signals:
	void game_finished();
};

#endif // ROOMTHREAD_H
