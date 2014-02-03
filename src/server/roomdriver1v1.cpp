#include "roomdriver1v1.h"
#include "room.h"
#include "engine.h"
#include "settings.h"
#include "generalselector.h"

#include <QDateTime>

RoomDriver1v1::RoomDriver1v1(Room *room)
	:room(room)
{
	//setParent(room);
}

void RoomDriver1v1::run(){

	// initialize the random seed for this thread
	qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));

	QSet<QString> banset = Config.value("Banlist/1v1").toStringList().toSet();
	general_names = Bang->getRandomGenerals(10, banset);
	unknown_list = general_names.mid(6, 4);

	QJsonArray choice_list;
	for(int i = 0;i < 6 && i < general_names.length(); i++){
		choice_list.append(general_names.at(i));
	}

	for(int i = 0; i < 4; i++){
		general_names[i + 6] = QString("x%1").arg(i);
		choice_list.append(general_names[i + 6]);
	}

	room->broadcastNotification(BP::FillGenerals, choice_list);

	ServerPlayer *first = room->getPlayers().at(0), *next = room->getPlayers().at(1);
	askForTakeGeneral(first);

	while(general_names.length() > 1){
		qSwap(first, next);

		askForTakeGeneral(first);
		askForTakeGeneral(first);
	}

	askForTakeGeneral(next);

	startArrange(first);
	startArrange(next);

	room->sem->acquire(2);
}

void RoomDriver1v1::askForTakeGeneral(ServerPlayer *player){
	QString name;
	if(general_names.length() == 1)
		name = general_names.first();
	else if(player->getState() != "online"){
		GeneralSelector *selector = GeneralSelector::GetInstance();
		name = selector->select1v1(general_names);
	}

	if(name.isNull()){
		player->notify(BP::AskForGeneral1v1);
	}else{
		QThread::currentThread()->msleep(1000);
		takeGeneral(player, name);
	}

	room->sem->acquire();
}

void RoomDriver1v1::takeGeneral(ServerPlayer *player, const QString &name){
	QString group = player->isLord() ? "warm" : "cool";
	QJsonArray group_data = BP::toJsonArray(group, name);
	room->broadcastNotification(BP::TakeGeneral, group_data, player);

	QRegExp unknown_rx("x(\\d)");
	QString general_name = name;
	if(unknown_rx.exactMatch(name)){
		int index = unknown_rx.capturedTexts().at(1).toInt();
		general_name = unknown_list.at(index);

		QJsonArray recover;
		recover.append(index);
		recover.append(general_name);
		player->notify(BP::RecoverGeneral, recover);
	}

	player->notify(BP::TakeGeneral, group_data);

	general_names.removeOne(name);
	player->addToSelected(general_name);

	room->sem->release();
}

void RoomDriver1v1::startArrange(ServerPlayer *player){
	if(player->getState() != "online"){
		GeneralSelector *selector = GeneralSelector::GetInstance();
		arrange(player, selector->arrange1v1(player));
	}else{
		player->notify(BP::StartArrange);
	}
}

void RoomDriver1v1::arrange(ServerPlayer *player, const QStringList &arranged){
	Q_ASSERT(arranged.length() == 3);

	QStringList left = arranged.mid(1, 2);
	player->tag["1v1Arrange"] = QVariant::fromValue(left);
	player->setGeneralName(arranged.first());

	foreach(QString general, arranged)
		player->notify(BP::RevealGeneral, BP::toJsonArray(player->objectName(), general));

	room->sem->release();
}
