#ifndef SERVER_H
#define SERVER_H

class Room;
class QGroupBox;
class QLabel;
class QRadioButton;

#include "socket.h"
#include "detector.h"
#include "clientstruct.h"

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QButtonGroup>
#include <QComboBox>
#include <QLayoutItem>
#include <QListWidget>
#include <QSplitter>
#include <QTabWidget>
#include <QMultiHash>

class Package;
class GameRule;

class Select3v3GeneralDialog: public QDialog{
	Q_OBJECT

public:
	Select3v3GeneralDialog(QDialog *parent);

private:
	QTabWidget *tab_widget;
	QSet<QString> ex_generals;

	void fillTabWidget();
	void fillListWidget(QListWidget *list, const Package *pack);

private slots:
	void save3v3Generals();
	void toggleCheck();
};

class BanlistDialog: public QDialog{
	Q_OBJECT

public:
	BanlistDialog(QWidget *parent, bool view = false);

private:
	QList<QListWidget *>lists;
	QListWidget * list;
	int item;
	QStringList ban_list;
	QPushButton* add2nd;

private slots:
	void addGeneral(const QString &name);
	void add2ndGeneral(const QString &name);
	void addPair(const QString &first, const QString& second);
	void doAdd2ndButton();
	void doAddButton();
	void doRemoveButton();
	void save();
	void saveAll();
	void switchTo(int item);
};

class ServerDialog: public QDialog{
	Q_OBJECT

public:
	ServerDialog(QWidget *parent);
	void ensureEnableAI();
	bool config();

private:
	QWidget *createBasicTab();
	QWidget *createPackageTab();
	QWidget *createAdvancedTab();
	QWidget *createAITab();
	QLayout *createButtonLayout();

	QGroupBox *createGameModeBox();
	QGroupBox *create3v3Box();

	QLineEdit *server_name_edit;
	QSpinBox *timeout_spinbox;
	QCheckBox *nolimit_checkbox;
	QCheckBox *contest_mode_checkbox;
	QCheckBox *free_choose_checkbox;
	QCheckBox *free_assign_checkbox;
	QCheckBox *free_assign_self_checkbox;
	QSpinBox *maxchoice_spinbox;
	QCheckBox *forbid_same_ip_checkbox;
	QCheckBox *disable_chat_checkbox;
	QCheckBox *second_general_checkbox;
	QCheckBox *scene_checkbox;	//changjing
	QCheckBox *same_checkbox;
	QCheckBox *basara_checkbox;
	QCheckBox *hegemony_checkbox;
	QLabel *max_hp_label;
	QComboBox *max_hp_scheme_combobox;
	QCheckBox *announce_ip_checkbox;
	QComboBox *scenario_combobox;
	QComboBox *mini_scene_combobox;
	QPushButton *mini_scene_button;
	QLineEdit *address_edit;
	QLineEdit *port_edit;
	QCheckBox *ai_enable_checkbox;
	QCheckBox *role_predictable_checkbox;
	QCheckBox *ai_chat_checkbox;
	QSpinBox *ai_delay_spinbox;
	QRadioButton *standard_3v3_radiobutton;
	QRadioButton *new_3v3_radiobutton;
	QComboBox *role_choose_combobox;
	QCheckBox *exclude_disaster_checkbox;

	QButtonGroup *extension_group;
	QButtonGroup *mode_group;

private slots:
	void onOkButtonClicked();
	void onDetectButtonClicked();
	void onHttpDone();
	void select3v3Generals();
	void edit1v1Banlist();
	void updateButtonEnablility(QAbstractButton* button);

	void doCustomAssign();
	void setMiniCheckBox();
};

class Scenario;
class ServerPlayer;

class Server : public QObject{
	Q_OBJECT

public:
	explicit Server(QObject *parent);

	void broadcast(const QString &msg);
	bool listen();
	void daemonize();
	Room *createNewRoom();
	void signupPlayer(ServerPlayer *player);
	void gamesOver();
	GameRule *getGameRule();
	TriggerSkill *getAbortGameRule();
	GameRule *getSceneRule();
	GameRule *getHulaoPassMode();
	GameRule *getBasaraMode();
	bool isReadyToClose() const;
	void deleteOnReadyToClose();

public slots:
	void stop();

private:
	ServerSocket *server;
	Room *current;
	QSet<Room *> rooms;
	QHash<QString, ServerPlayer*> players;
	QSet<QString> addresses;
	QMultiHash<QString, QString> name2objname;
	GameRule *game_rule;
	TriggerSkill *abort_rule;
	GameRule *hulaopass_mode;
	GameRule *basara_mode;
	GameRule *scene_rule;

	void processRequest(ClientSocket *socket, const QByteArray &request);

private slots:
	void processNewConnection(ClientSocket *socket);
	void processRequest();
	void cleanup();
	void gameOver();
	void closeRoom();

signals:
	void server_message(const QString &);
	void about_to_stop();
	void ready_to_close();
	void unready_to_close();
};

#endif // SERVER_H
