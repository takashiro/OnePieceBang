#ifndef ROOMDRIVER1V1_H
#define ROOMDRIVER1V1_H

#include <QThread>
#include <QStringList>

class Room;
class ServerPlayer;

class RoomDriver1v1 : public QThread
{
	Q_OBJECT

public:
	explicit RoomDriver1v1(Room *room);
	void takeGeneral(ServerPlayer *player, const QString &name);
	void arrange(ServerPlayer *player, const QStringList &arranged);

protected:
	void run();

private:
	Room *room;
	QStringList general_names;
	QStringList unknown_list;

	void askForTakeGeneral(ServerPlayer *player);
	void startArrange(ServerPlayer *player);
};

#endif // ROOMTHREAD1V1_H
