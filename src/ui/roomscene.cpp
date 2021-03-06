#include "roomscene.h"
#include "settings.h"
#include "carditem.h"
#include "engine.h"
#include "cardoverview.h"
#include "distanceviewdialog.h"
#include "playercarddialog.h"
#include "choosegeneraldialog.h"
#include "window.h"
#include "button.h"
#include "cardcontainer.h"
#include "recorder.h"
#include "indicatoritem.h"
#include "pixmapanimation.h"
#include "audio.h"

#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QGraphicsSceneMouseEvent>
#include <QMessageBox>
#include <QListWidget>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QCheckBox>
#include <QGraphicsLinearLayout>
#include <QMenu>
#include <QGroupBox>
#include <QLineEdit>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QFileDialog>
#include <QDesktopServices>
#include <QRadioButton>
#include <QApplication>
#include <QTimer>
#include <QCommandLinkButton>
#include <QFormLayout>
#include <QStatusBar>
#include <qmath.h>

#ifdef Q_OS_WIN32
#include <QAxObject>
#endif

#ifdef JOYSTICK_SUPPORT

#include "joystick.h"

#endif


struct RoomLayout {
	int m_scenePadding;
	int m_roleBoxHeight;
	int m_chatTextBoxHeight;
	int m_discardPileMinWidth;
	int m_discardPilePadding;
	double m_logBoxHeightPercentage;
	double m_chatBoxHeightPercentage;
	double m_infoPlaneWidthPercentage;
	double m_photoRoomPadding;
	double m_photoPhotoPadding;
	QSize m_minimumSceneSize;
};

struct CircularRoomLayout : public RoomLayout{
	CircularRoomLayout(){
		m_scenePadding = 0;
		m_roleBoxHeight = 60;
		m_chatTextBoxHeight = 30;
		m_logBoxHeightPercentage = 0.6;
		m_chatBoxHeightPercentage = 0.4;
		m_infoPlaneWidthPercentage = 0.22;
		m_photoRoomPadding = 10;
		m_photoPhotoPadding = 5;
		m_discardPileMinWidth = CardItem::S_NORMAL_CARD_WIDTH * 5;
		m_discardPilePadding = 50;
		m_minimumSceneSize = QSize(900, 650);
	}
};

static RoomLayout *GetRoomLayout(){
	static CircularRoomLayout circular;
	return &circular;
}

RoomScene *RoomSceneInstance;

void RoomScene::resetPiles(){
	discard_pile->clearCards();
}

#include "irregularbutton.h"

RoomScene::RoomScene(QMainWindow *main_window)
	:QGraphicsScene(main_window),focused(NULL), special_card(NULL),
	  main_window(main_window),game_started(false)
{
	choice_dialog = NULL;
	RoomSceneInstance = this;
	_m_last_front_item = NULL;
	last_front_z_value = 0;
	int player_count = Bang->getPlayerCount(ServerInfo.GameMode);

	room_layout = GetRoomLayout();

	bool circular = Config.value("CircularView", false).toBool();

	// create photos
	for(int i = 0; i < player_count - 1;i++){
		Photo *photo = new Photo;
		photos << photo;
		addItem(photo);
		photo->setZValue(-0.5);
	}

	{
		createControlButtons();
		QGraphicsItem *button_widget = NULL;
		if(ClientInstance->getReplayer() == NULL){
			QString path = "image/system/button/irregular/background.png";
			button_widget = new QGraphicsPixmapItem(QPixmap(path));

			ok_button->setParentItem(button_widget);
			cancel_button->setParentItem(button_widget);
			discard_button->setParentItem(button_widget);
			trust_button->setParentItem(button_widget);
		}

		// create draw pile
		draw_pile = new DrawPile;
		addItem(draw_pile);

		// create discard pile
		discard_pile = new DiscardPile;
		addItem(discard_pile);

		// create handling area
		handling_area = discard_pile;

		// create dashboard
		dashboard = new Dashboard(button_widget);
		dashboard->setObjectName("dashboard");
		dashboard->setZValue(0.8);
		addItem(dashboard);

		dashboard->setPlayer(Self);
		connect(Self, SIGNAL(general_changed()), dashboard, SLOT(updateAvatar()));
		connect(Self, SIGNAL(general2_changed()), dashboard, SLOT(updateSmallAvatar()));
		connect(ClientInstance, SIGNAL(do_filter()), dashboard, SLOT(doFilter()));
		connect(dashboard, SIGNAL(card_selected(const Card*)), this, SLOT(enableTargets(const Card*)));
		connect(dashboard, SIGNAL(card_to_use()), this, SLOT(doOkButton()));

		sort_combobox = new QComboBox;

		sort_combobox->addItem(tr("No sort"));
		sort_combobox->addItem(tr("Sort by color"));
		sort_combobox->addItem(tr("Sort by suit"));
		sort_combobox->addItem(tr("Sort by type"));
		sort_combobox->addItem(tr("Sort by availability"));

		connect(sort_combobox, SIGNAL(currentIndexChanged(int)), dashboard, SLOT(sortCards(int)));
	}

	connect(Self, SIGNAL(pile_changed(QString)), this, SLOT(updatePileButton(QString)));

	// add role combobox
	role_combobox = new QComboBox;
	role_combobox->addItem(tr("Your role"));
	role_combobox->addItem(tr("Unknown"));
	connect(Self, SIGNAL(role_changed(QString)), this, SLOT(updateRoleComboBox(QString)));

	createExtraButtons();
	if(ClientInstance->getReplayer())
		createReplayControlBar();

	response_skill = new ResponseSkill;
	discard_skill = new DiscardSkill;
	yiji_skill = new YijiViewAsSkill;
	choose_skill = new ChoosePlayerSkill;

	known_cards_menu = new QMenu(main_window);

	{
		change_general_menu = new QMenu(main_window);
		QAction *action = change_general_menu->addAction(tr("Change general ..."));
		FreeChooseDialog *general_changer = new FreeChooseDialog(main_window);
		connect(action, SIGNAL(triggered()), general_changer, SLOT(exec()));
		connect(general_changer, SIGNAL(general_chosen(QString)), this, SLOT(changeGeneral(QString)));
		to_change = NULL;
	}

	// get dashboard's avatar
	avatar = dashboard->getAvatar();

	// do signal-slot connections
	connect(ClientInstance, SIGNAL(player_added(ClientPlayer*)), SLOT(addPlayer(ClientPlayer*)));
	connect(ClientInstance, SIGNAL(player_removed(QString)), SLOT(removePlayer(QString)));
	connect(ClientInstance, SIGNAL(generals_got(QStringList)), this, SLOT(chooseGeneral(QStringList)));
	connect(ClientInstance, SIGNAL(suits_got(QStringList)), this, SLOT(chooseSuit(QStringList)));
	connect(ClientInstance, SIGNAL(options_got(QString, QStringList)), this, SLOT(chooseOption(QString, QStringList)));
	connect(ClientInstance, SIGNAL(cards_got(const ClientPlayer*, QString, QString)), this, SLOT(chooseCard(const ClientPlayer*, QString, QString)));
	connect(ClientInstance, SIGNAL(roles_got(QString, QStringList)), this, SLOT(chooseRole(QString, QStringList)));
	connect(ClientInstance, SIGNAL(directions_got()), this, SLOT(chooseDirection()));
	connect(ClientInstance, SIGNAL(orders_got(BP::Game3v3ChooseOrderCommand)), this, SLOT(chooseOrder(BP::Game3v3ChooseOrderCommand)));
	connect(ClientInstance, SIGNAL(kingdoms_got(QStringList)), this, SLOT(chooseKingdom(QStringList)));
	connect(ClientInstance, SIGNAL(seats_arranged(QList<const ClientPlayer*>)), SLOT(arrangeSeats(QList<const ClientPlayer*>)));
	connect(ClientInstance, SIGNAL(status_changed(Client::Status, Client::Status)), this, SLOT(updateStatus(Client::Status, Client::Status)));
	connect(ClientInstance, SIGNAL(avatars_hiden()), this, SLOT(hideAvatars()));
	connect(ClientInstance, SIGNAL(hp_changed(QString,int,DamageStruct::Nature,bool)), SLOT(changeHp(QString,int,DamageStruct::Nature,bool)));
	connect(ClientInstance, SIGNAL(pile_reset()), this, SLOT(resetPiles()));
	connect(ClientInstance, SIGNAL(player_killed(QString)), this, SLOT(killPlayer(QString)));
	connect(ClientInstance, SIGNAL(player_revived(QString)), this, SLOT(revivePlayer(QString)));
	connect(ClientInstance, SIGNAL(card_shown(QString,int)), this, SLOT(showCard(QString,int)));
	connect(ClientInstance, SIGNAL(gongxin(QList<int>, bool)), this, SLOT(doGongxin(QList<int>, bool)));
	connect(ClientInstance, SIGNAL(focus_moved(QString, BP::Countdown)), this, SLOT(moveFocus(QString, BP::Countdown)));
	connect(ClientInstance, SIGNAL(emotion_set(QString,QString)), this, SLOT(setEmotion(QString,QString)));
	connect(ClientInstance, SIGNAL(skill_invoked(QString,QString)), this, SLOT(showSkillInvocation(QString,QString)));
	connect(ClientInstance, SIGNAL(skill_acquired(const ClientPlayer*,QString)), this, SLOT(acquireSkill(const ClientPlayer*,QString)));
	connect(ClientInstance, SIGNAL(animated(QString,QStringList)), this, SLOT(doAnimation(QString,QStringList)));
	connect(ClientInstance, SIGNAL(judge_result(QString,bool)), this, SLOT(showJudgeResult(QString,bool)));
	connect(ClientInstance, SIGNAL(role_state_changed(QString)),this, SLOT(updateRoles(QString)));

	connect(ClientInstance, SIGNAL(game_started()), this, SLOT(onGameStart()));
	connect(ClientInstance, SIGNAL(game_over()), this, SLOT(onGameOver()));
	connect(ClientInstance, SIGNAL(standoff()), this, SLOT(onStandoff()));

	connect(ClientInstance, SIGNAL(move_cards_lost(int, QList<CardsMoveStruct>)), this, SLOT(loseCards(int, QList<CardsMoveStruct>)));
	connect(ClientInstance, SIGNAL(move_cards_got(int, QList<CardsMoveStruct>)), this, SLOT(getCards(int, QList<CardsMoveStruct>)));

	connect(ClientInstance, SIGNAL(assign_asked()), this, SLOT(startAssign()));
	connect(ClientInstance, SIGNAL(start_in_xs()), this, SLOT(startInXSeconds()));

	{
		guanxing_box = new GuanxingBox;
		guanxing_box->hide();
		addItem(guanxing_box);
		guanxing_box->setZValue(9.0);

		connect(ClientInstance, SIGNAL(guanxing(QList<int>,bool)), guanxing_box, SLOT(doGuanxing(QList<int>,bool)));

		if(circular)
			guanxing_box->moveBy(-120, 0);
	}

	{
		card_container = new CardContainer();
		card_container->hide();
		addItem(card_container);
		// card_container->shift();
		card_container->setZValue(guanxing_box->zValue());

		connect(card_container, SIGNAL(item_chosen(int)), ClientInstance, SLOT(onPlayerChooseAG(int)));
		connect(card_container, SIGNAL(item_gongxined(int)), ClientInstance, SLOT(onPlayerReplyGongxin(int)));

		connect(ClientInstance, SIGNAL(ag_filled(QList<int>)), this, SLOT(fillCards(QList<int>)));
		connect(ClientInstance, SIGNAL(ag_taken(ClientPlayer*,int)), this, SLOT(takeAmazingGrace(ClientPlayer*,int)));
		connect(ClientInstance, SIGNAL(ag_cleared()), card_container, SLOT(clear()));

		if(circular)
			card_container->moveBy(-120, 0);
	}

	connect(ClientInstance, SIGNAL(skill_attached(QString, bool)), this, SLOT(attachSkill(QString,bool)));
	connect(ClientInstance, SIGNAL(skill_detached(QString)), this, SLOT(detachSkill(QString)));

	enemy_box = NULL;
	self_box = NULL;

	if(ServerInfo.GameMode == "06_3v3" || ServerInfo.GameMode == "02_1v1"){
		// 1v1 & 3v3 mode
		connect(ClientInstance, SIGNAL(generals_filled(QStringList)), this, SLOT(fillGenerals(QStringList)));
		connect(ClientInstance, SIGNAL(general_asked()), this, SLOT(startGeneralSelection()));
		connect(ClientInstance, SIGNAL(general_taken(QString,QString)), this, SLOT(takeGeneral(QString,QString)));
		connect(ClientInstance, SIGNAL(arrange_started()), this, SLOT(startArrange()));
		connect(ClientInstance, SIGNAL(general_recovered(int,QString)), this, SLOT(recoverGeneral(int,QString)));

		arrange_button = NULL;

		if(ServerInfo.GameMode == "02_1v1"){
			enemy_box = new KOFOrderBox(false, this);
			self_box = new KOFOrderBox(true, this);

			enemy_box->hide();
			self_box->hide();

			connect(ClientInstance, SIGNAL(general_revealed(bool,QString)), this, SLOT(revealGeneral(bool,QString)));
		}
	}

	{
		// chat box
		chat_box = new QTextEdit;
		chat_box->setObjectName("chat_box");
		chat_box_widget = addWidget(chat_box);
		chat_box_widget->setZValue(-2.0);
		chat_box_widget->setObjectName("chat_box_widget");
		chat_box->setReadOnly(true);
		chat_box->setTextColor(Config.TextEditColor);
		connect(ClientInstance, SIGNAL(line_spoken(QString)), this, SLOT(appendChatBox(QString)));

		// chat edit
		chat_edit = new QLineEdit;
		chat_edit->setObjectName("chat_edit");
		chat_edit_widget = addWidget(chat_edit);
		chat_edit_widget->setObjectName("chat_edit_widget");
		chat_edit_widget->setZValue(-2.0);
		connect(chat_edit, SIGNAL(returnPressed()), this, SLOT(speak()));
#if QT_VERSION >= 0x040700
		chat_edit->setPlaceholderText(tr("Please enter text to chat ... "));
#endif
		
		chat_widget = new ChatWidget();
		chat_widget->setZValue(-0.2);
		addItem(chat_widget);
		connect(chat_widget,SIGNAL(return_button_click()),this, SLOT(speak()));
		connect(chat_widget,SIGNAL(chat_widget_msg(QString)),this, SLOT(appendChatEdit(QString)));

		if(ServerInfo.DisableChat)
			chat_edit_widget->hide();
	}

	{
		// log box
		log_box = new ClientLogBox;
		log_box->setTextColor(Config.TextEditColor);
		log_box->setObjectName("log_box");

		log_box_widget = addWidget(log_box);
		log_box_widget->setObjectName("log_box_widget");
		connect(ClientInstance, SIGNAL(log_received(QString)), log_box, SLOT(appendLog(QString)));
		
		log_box_widget->setFlag(QGraphicsItem::ItemIsMovable);
	}

	{
		prompt_box = new Window(tr("OnePieceBang"), QSize(480, 200));
		prompt_box->setOpacity(0);
		prompt_box->setFlag(QGraphicsItem::ItemIsMovable);
		prompt_box->shift();
		prompt_box->setZValue(10);
		prompt_box->keepWhenDisappear();

		prompt_box_widget = new QGraphicsTextItem(prompt_box);
		prompt_box_widget->setParent(prompt_box);
		prompt_box_widget->setPos(40, 45);
		prompt_box_widget->setDefaultTextColor(Qt::white);

		QTextDocument *prompt_doc = ClientInstance->getPromptDoc();
		prompt_doc->setTextWidth(prompt_box->boundingRect().width() - 80);
		prompt_box_widget->setDocument(prompt_doc);

		QFont qf = Config.SmallFont;
		qf.setPixelSize(18);
		qf.setStyleStrategy(QFont::PreferAntialias);
		//qf.setBold(true);
		prompt_box_widget->setFont(qf);

		QGraphicsDropShadowEffect *drp = new QGraphicsDropShadowEffect;
		drp->setOffset(0);
		drp->setColor(Qt::white);
		drp->setBlurRadius(5);
		//m_pileCardNumInfoTextBox->setGraphicsEffect(drp);

		connect(prompt_doc,SIGNAL(contentsChanged()),this,SLOT(adjustPrompt()));

		addItem(prompt_box);
	}

#ifdef AUDIO_SUPPORT
	memory = new QSharedMemory("OnePieceBang", this);
#endif

	timer_id = 0;
	tick = 0;

#ifdef JOYSTICK_SUPPORT

	if(Config.value("JoystickEnabled", false).toBool()){
		Joystick *js = new Joystick(this);
		connect(js, SIGNAL(button_clicked(int)), this, SLOT(onJoyButtonClicked(int)));
		connect(js, SIGNAL(direction_clicked(int)), this, SLOT(onJoyDirectionClicked(int)));

		js->start();
	}

#endif

	QHBoxLayout* skill_dock_layout = new QHBoxLayout;
	QMargins margins = skill_dock_layout->contentsMargins();
	margins.setTop(0);
	margins.setBottom(5);
	skill_dock_layout->setContentsMargins(margins);
	skill_dock_layout->addStretch();

	main_window->statusBar()->setObjectName("skill_bar_container");
	//main_window->statusBar()->setLayout(skill_dock_layout);
	addWidgetToSkillDock(sort_combobox, true);
	main_window->statusBar()->show();

	roles_box_background.load("image/system/state.png");
	roles_box = new QGraphicsPixmapItem;
	addItem(roles_box);
	QString roles = Bang->getRoles(ServerInfo.GameMode);
	pile_card_num_info_text_box = addText("");
	pile_card_num_info_text_box->setParentItem(roles_box);
	pile_card_num_info_text_box->setDocument(ClientInstance->getLinesDoc());
	pile_card_num_info_text_box->setDefaultTextColor(Qt::white);
	updateRoles(roles);

	add_robot = NULL;
	fill_robots = NULL;
	if(ServerInfo.EnableAI){
		control_panel = addRect(0, 0, 500, 150, Qt::NoPen);
		control_panel->hide();

		add_robot = new Button(tr("Add a robot"));
		add_robot->setParentItem(control_panel);
		add_robot->setTransform(QTransform::fromTranslate(-add_robot->boundingRect().width() / 2, -add_robot->boundingRect().height() / 2), true);
		add_robot->setPos(0, -add_robot->boundingRect().height() - 10);

		fill_robots = new Button(tr("Fill robots"));
		fill_robots->setParentItem(control_panel);
		fill_robots->setTransform(QTransform::fromTranslate(-fill_robots->boundingRect().width() / 2, -fill_robots->boundingRect().height() / 2), true);
		add_robot->setPos(0, add_robot->boundingRect().height() + 10);

		connect(add_robot, SIGNAL(clicked()), ClientInstance, SLOT(addRobot()));
		connect(fill_robots, SIGNAL(clicked()), ClientInstance, SLOT(fillRobots()));
		connect(Self, SIGNAL(owner_changed(bool)), this, SLOT(showOwnerButtons(bool)));
	}else
		control_panel = NULL;
	animations = new EffectAnimation();
}

void RoomScene::createControlButtons(){
	ok_button = new IrregularButton("ok");
	ok_button->setPos(5, 1);

	cancel_button = new IrregularButton("cancel");
	cancel_button->setPos(5, 64);

	discard_button = new IrregularButton("discard");
	discard_button->setPos(57, 18);

	connect(ok_button, SIGNAL(clicked()), this, SLOT(doOkButton()));
	connect(cancel_button, SIGNAL(clicked()), this, SLOT(doCancelButton()));
	connect(discard_button, SIGNAL(clicked()), this, SLOT(doDiscardButton()));

	trust_button = new TrustButton;
	trust_button->setPos(57, 102);
	connect(trust_button, SIGNAL(clicked()), ClientInstance, SLOT(trust()));
	connect(Self, SIGNAL(state_changed()), this, SLOT(updateTrustButton()));

	// set them all disabled
	ok_button->setEnabled(false);
	cancel_button->setEnabled(false);
	discard_button->setEnabled(false);
	trust_button->setEnabled(false);
}

void RoomScene::createExtraButtons(){
	reverse_selection_button = dashboard->createButton("reverse-select");
	reverse_selection_button->setEnabled(true);

	dashboard->addWidget(reverse_selection_button, room_layout->m_scenePadding
		+ room_layout->m_photoRoomPadding * 3 + Photo::S_NORMAL_PHOTO_WIDTH, true);
	connect(reverse_selection_button, SIGNAL(clicked()), dashboard, SLOT(reverseSelection()));

	free_discard_button = NULL;
}

ReplayerControlBar::ReplayerControlBar(Dashboard *dashboard){
	QHBoxLayout *layout = new QHBoxLayout;

	QPushButton *play, *uniform, *slow_down, *speed_up;

	uniform = dashboard->createButton("uniform");
	slow_down = dashboard->createButton("slow-down");
	play = dashboard->createButton("pause");
	speed_up = dashboard->createButton("speed-up");

	time_label = new QLabel;
	QPalette palette;
	palette.setColor(QPalette::WindowText, Config.TextEditColor);
	time_label->setPalette(palette);

	QWidgetList widgets;
	widgets << uniform << slow_down << play << speed_up << time_label;

	foreach(QWidget *widget, widgets){
		widget->setEnabled(true);
		layout->addWidget(widget);
	}

	Replayer *replayer = ClientInstance->getReplayer();
	connect(play, SIGNAL(clicked()), replayer, SLOT(toggle()));
	connect(play, SIGNAL(clicked()), this, SLOT(toggle()));
	connect(uniform, SIGNAL(clicked()), replayer, SLOT(uniform()));
	connect(slow_down, SIGNAL(clicked()), replayer, SLOT(slowDown()));
	connect(speed_up, SIGNAL(clicked()), replayer, SLOT(speedUp()));
	connect(replayer, SIGNAL(elasped(int)), this, SLOT(setTime(int)));
	connect(replayer, SIGNAL(speed_changed(qreal)), this, SLOT(setSpeed(qreal)));

	speed = replayer->getSpeed();

	QWidget *widget = new QWidget;
	widget->setAttribute(Qt::WA_TranslucentBackground);
	widget->setLayout(layout);
	setWidget(widget);

	setParentItem(dashboard);
	setPos(0,-35);

	duration_str = FormatTime(replayer->getDuration());
}

QString ReplayerControlBar::FormatTime(int secs){
	int minutes = secs / 60;
	int remainder  = secs % 60;
	return QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(remainder, 2, 10, QChar('0'));
}

void ReplayerControlBar::toggle(){
	QPushButton *button = qobject_cast<QPushButton *>(sender());

	if(button){
		QString new_name = button->objectName() == "pause" ? "play" : "pause";
		button->setObjectName(new_name);
		button->setIcon(QIcon(QString("image/system/button/%1.png").arg(new_name)));
	}
}

void ReplayerControlBar::setSpeed(qreal speed){
	this->speed = speed;
}

void ReplayerControlBar::setTime(int secs){
	time_label->setText(QString("<b>x%1 </b> [%2/%3]")
						.arg(speed)
						.arg(FormatTime(secs))
						.arg(duration_str));
}

void RoomScene::createReplayControlBar(){
	// hide all buttons
	reverse_selection_button->hide();

	new ReplayerControlBar(dashboard);
}

void RoomScene::adjustItems(){
	QRectF displayRegion = sceneRect();
	if(displayRegion.left() != 0 || displayRegion.top() != 0 ||
		displayRegion.bottom() < room_layout->m_minimumSceneSize.height() ||
		displayRegion.right() < room_layout->m_minimumSceneSize.width())
	{
		displayRegion.setLeft(0); displayRegion.setTop(0);
		double sy = room_layout->m_minimumSceneSize.height() / displayRegion.height();
		double sx = room_layout->m_minimumSceneSize.width() / displayRegion.width();
		double scale = qMax(sx, sy);
		displayRegion.setBottom(scale * displayRegion.height());
		displayRegion.setRight(scale * displayRegion.width());
		setSceneRect(displayRegion);
	}
	int padding = room_layout->m_scenePadding;
	displayRegion.moveLeft(displayRegion.x() + padding);
	displayRegion.moveTop(displayRegion.y() + padding);
	displayRegion.setWidth(displayRegion.width() - padding * 2);
	displayRegion.setHeight(displayRegion.height() - padding * 2);
	
	// set dashboard
	dashboard->setX(displayRegion.x());
	dashboard->setWidth(displayRegion.width());
	dashboard->setY(displayRegion.height() - dashboard->boundingRect().height());
	
	// set infoplane
	QRectF infoPlane;
	infoPlane.setWidth(displayRegion.width() * room_layout->m_infoPlaneWidthPercentage);
	infoPlane.moveRight(displayRegion.right());
	infoPlane.setTop(displayRegion.top() + room_layout->m_roleBoxHeight);
	infoPlane.setBottom(dashboard->y() - room_layout->m_chatTextBoxHeight);
	roles_box_background = roles_box_background.scaled(infoPlane.width(), room_layout->m_roleBoxHeight);
	roles_box->setPixmap(roles_box_background);
	roles_box->setPos(infoPlane.left(), displayRegion.top());

	log_box_widget->setPos(infoPlane.topLeft());
	log_box->resize(infoPlane.width(), infoPlane.height() * room_layout->m_logBoxHeightPercentage);
	chat_box_widget->setPos(infoPlane.left(), infoPlane.bottom() - infoPlane.height() * room_layout->m_chatBoxHeightPercentage);
	chat_box->resize(infoPlane.width(), infoPlane.bottom() - chat_box_widget->y());
	chat_edit_widget->setPos(infoPlane.left(), infoPlane.bottom());
	chat_edit->resize(infoPlane.width() - chat_widget->boundingRect().width(), room_layout->m_chatTextBoxHeight);
	chat_widget->setPos(infoPlane.right() - chat_widget->boundingRect().width(),
		chat_edit_widget->y() + (room_layout->m_chatTextBoxHeight - chat_widget->boundingRect().height()) / 2);
	
	 if(self_box)
		 self_box->setPos(infoPlane.left() - padding - self_box->boundingRect().width(),
			sceneRect().height() - padding * 3 - self_box->boundingRect().height() - dashboard->boundingRect().height() - reverse_selection_button->height());
	 if(enemy_box)
		 enemy_box->setPos(padding * 2, padding * 2);
	
	updateTable();
	updateRolesBox();
}

void RoomScene::_dispersePhotos(QList<Photo*> &photos, QRectF fillRegion, int minDistanceBetweenPhotos, Qt::Alignment align)
{
	int numPhotos = photos.size();
	if(numPhotos == 0) return;
	if(align == Qt::AlignHCenter){
		double maxWidth = fillRegion.width();
		double photoWidth = Photo::S_NORMAL_PHOTO_WIDTH;
		double step = qMax(photoWidth  + minDistanceBetweenPhotos, maxWidth / numPhotos);
		for(int i = 0; i < numPhotos; i++){
			Photo* photo = photos[i];
			double newX = fillRegion.center().x() + step * (i - (numPhotos - 1) / 2.0);
			QPointF newPos = QPointF(newX, fillRegion.center().y());
			photo->setPos(newPos);
		}
	}
	else if(align == Qt::AlignVCenter){
		double maxHeight = fillRegion.height();
		double photoHeight = Photo::S_NORMAL_PHOTO_HEIGHT;
		double step = qMax(photoHeight + minDistanceBetweenPhotos, maxHeight / numPhotos);
		for(int i = 0; i < numPhotos; i++){
			Photo* photo = photos[i];
			double newY = fillRegion.center().y() + step * (i - (numPhotos - 1) / 2.0);
			QPointF newPos = QPointF(fillRegion.center().x(), newY);
			photo->setPos(newPos);
		}
	}
	else if(align == Qt::AlignBottom){
		double photoHeight = Photo::S_NORMAL_PHOTO_HEIGHT;
		double step = minDistanceBetweenPhotos + photoHeight;
		for(int i = 0; i < numPhotos; i++){
			Photo* photo = photos[i];
			double newY = fillRegion.bottom() - photoHeight / 2 - step * (numPhotos - i - 1);
			QPointF newPos = QPointF(fillRegion.center().x(), newY);
			photo->setPos(newPos);
		}
	}
}

void RoomScene::updateTable()
{
	int &pad = room_layout->m_scenePadding;
	int tablew = log_box_widget->x() - pad;
	int tableh = sceneRect().height() - pad * 2 - dashboard->boundingRect().height() - room_layout->m_photoRoomPadding;

	// Layout:
	//    col1           col2
	// _______________________
	// |_2_|______1_______|_0_| row1
	// |   |              |   |
	// | 4 |    table     | 3 |
	// |___|______________|___|
	// |      dashboard       |
	// ------------------------
	// region 5 = 0 + 3, region 6 = 2 + 4, region 7 = 0 + 1 + 2
	// region 8 = heightened 3, region 9 = heightened 4
	static int regularSeatIndex[][9] =
	{
		{1},    // 2 players
		{5, 6}, // 3 players
		{5, 1, 6},
		{3, 1, 1, 4},
		{3, 1, 1, 1, 4},
		{5, 5, 1, 1, 6, 6},
		{8, 8, 1, 1, 1, 9, 9}, // 8 players
		{3, 3, 7, 7, 7, 7, 4, 4}, // 9 players
		{3, 3, 7, 7, 7, 7, 7, 4, 4} // 10 players
	};
	static int hulaoSeatIndex[][3] =
	{
		{1, 1, 1}, // if self is lubu{3, 3, 1},
		{3, 1, 4},
		{1, 4, 4}
	};
	static int kof3v3SeatIndex[][5] =
	{
		{3, 1, 1, 1, 4}, // lord
		{1, 1, 1, 4, 4}, // rebel (left), same with loyalist (left)
		{3, 3, 1, 1, 1} // loyalist (right), same with rebel (right)
	};
	
	double col1 = Photo::S_NORMAL_PHOTO_WIDTH + 2 * room_layout->m_photoRoomPadding;
	double col2 = tablew - col1;
	double row1 = Photo::S_NORMAL_PHOTO_HEIGHT + 2 * room_layout->m_photoRoomPadding;
	double row2 = tableh;

	const int C_NUM_REGIONS = 10;
	QRectF seatRegions[] =
	{
		QRectF(col2, 0, col1, row1),
		QRectF(col1, 0, col2 - col1, row1),
		QRectF(0, 0, col1, row1),
		QRectF(col2, row1, col1, row2 - row1),
		QRectF(0, row1, col1, row2 - row1),
		QRectF(col2, 0, col1, row2),
		QRectF(0, 0, col1, row2),
		QRectF(0, 0, col1 + col2, row1),
		QRectF(col2, row1 - Photo::S_NORMAL_PHOTO_HEIGHT * 0.2, col1, row2 - row1 + Photo::S_NORMAL_PHOTO_HEIGHT * 0.2),
		QRectF(0, row1 - Photo::S_NORMAL_PHOTO_HEIGHT * 0.2, col1, row2 - row1 + Photo::S_NORMAL_PHOTO_HEIGHT * 0.2)
	};

	QRectF tableRect(col1, row1, col2 - col1, row2 - row1);
	table_center_pos = tableRect.center();
	control_panel->setPos(table_center_pos);
	draw_pile->setPos(table_center_pos);
	discard_pile->setPos(table_center_pos);
	discard_pile->setSize(qMax((int)tableRect.width() - room_layout->m_discardPilePadding * 2, room_layout->m_discardPileMinWidth), CardItem::S_NORMAL_CARD_HEIGHT);
	card_container->setPos(table_center_pos);
	guanxing_box->setPos(table_center_pos);
	prompt_box->setPos(table_center_pos);

	int* seatToRegion;
	bool pkMode = false;
	if(ServerInfo.GameMode == "04_1v3" && game_started){
		seatToRegion = hulaoSeatIndex[Self->getSeat() - 1];
		pkMode = true;
	}
	else if(ServerInfo.GameMode == "06_3v3" && game_started){
		seatToRegion = kof3v3SeatIndex[(Self->getSeat() - 1) % 3];
		pkMode = true;
	}
	else
	{
		seatToRegion = regularSeatIndex[photos.length() - 1];
	}
	QList<Photo*> photosInRegion[C_NUM_REGIONS];
	int n = photos.length();
	for(int i = 0; i < n; i++){
		int regionIndex = seatToRegion[i];
		if(regionIndex == 4 || regionIndex == 6 || regionIndex == 9)
			photosInRegion[regionIndex].append(photos[i]);
		else
			photosInRegion[regionIndex].prepend(photos[i]);
	}
	for(int i = 0; i < C_NUM_REGIONS; i++){
		if(photosInRegion[i].isEmpty()) continue;
		Qt::Alignment align;
		if(i < 3 || i == 7) align = Qt::AlignHCenter;
		// else if(pkMode) align = Qt::AlignBottom;
		else align = Qt::AlignVCenter;
		_dispersePhotos(photosInRegion[i], seatRegions[i], room_layout->m_photoPhotoPadding, align);
	}

}

void RoomScene::addPlayer(ClientPlayer *player){
	int i;
	for(i=0; i<photos.length(); i++){
		Photo *photo = photos[i];
		if(photo->getPlayer() == NULL){
			photo->setPlayer(player);
			name2photo[player->objectName()] = photo;

			if(!Self->hasFlag("marshalling"))
				Bang->playAudio("add-player");

			return;
		}
	}
}

QPointF RoomScene::getTableCenter() const{
	return table_center_pos;
}

void RoomScene::removePlayer(const QString &player_name){
	Photo *photo = name2photo[player_name];
	if(photo){
		photo->setPlayer(NULL);
		name2photo.remove(player_name);

		Bang->playAudio("remove-player");
	}
}

void RoomScene::arrangeSeats(const QList<const ClientPlayer*> &seats){
	// rearrange the photos
	Q_ASSERT(seats.length() == photos.length());

	for(int i = 0; i < seats.length(); i++){
		const Player *player = seats.at(i);
		for(int j = i; j < photos.length(); j++){
			if(photos.at(j)->getPlayer() == player){
				photos.swap(i, j);
				break;
			}
		}
	}
	game_started = true;
	QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
	updateTable();

	for(int i = 0; i < photos.length(); i++){
		Photo *photo = photos.at(i);
		photo->setOrder(photo->getPlayer()->getSeat());
		photo->createRoleCombobox();
	}
		
	group->start(QAbstractAnimation::DeleteWhenStopped);

	// set item to player mapping
	if(item2player.isEmpty()){
		item2player.insert(avatar, Self);
		connect(avatar, SIGNAL(selected_changed()), this, SLOT(updateSelectedTargets()));
		connect(avatar, SIGNAL(selected_changed()), this, SLOT(onSelectChange()));
		foreach(Photo *photo, photos){
			item2player.insert(photo, photo->getPlayer());
			connect(photo, SIGNAL(selected_changed()), this, SLOT(updateSelectedTargets()));
			connect(photo, SIGNAL(selected_changed()), this, SLOT(onSelectChange()));
			connect(photo, SIGNAL(enable_changed()), this, SLOT(onEnabledChange()));
		}
	}
}

void RoomScene::mousePressEvent(QGraphicsSceneMouseEvent *event){
	foreach(Photo *photo, photos){
		if(photo->isUnderMouse() && photo->isEnabled() && photo->flags() & QGraphicsItem::ItemIsSelectable){
			photo->setSelected(!photo->isSelected());
			return;
		}
	}

	if(avatar->isUnderMouse() && avatar->isEnabled() && avatar->flags() & QGraphicsItem::ItemIsSelectable){
		avatar->setSelected(!avatar->isSelected());
		return;
	}

	QGraphicsScene::mousePressEvent(event);
}

void RoomScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event){
	QGraphicsScene::mouseMoveEvent(event);

	QGraphicsObject *obj = static_cast<QGraphicsObject*>(focusItem());
	CardItem *card_item = qobject_cast<CardItem*>(obj);
	if(!card_item || !card_item->isUnderMouse())
		return;

	foreach(Photo *photo, photos){
		if(photo->isUnderMouse()){
			photo->setSelected(true);
			break;
		}
	}

	if(avatar->isUnderMouse()){
		avatar->setSelected(true);
	}
}

void RoomScene::keyReleaseEvent(QKeyEvent *event){
	if(!Config.EnableHotKey)
		return;

	if(chat_edit->hasFocus())
		return;

	bool control_is_down = event->modifiers() & Qt::ControlModifier;

	switch(event->key()){
	case Qt::Key_F1: break;
	case Qt::Key_F2: chooseSkillButton(); break;
	case Qt::Key_F3: sort_combobox->showPopup(); break;

	case Qt::Key_S: dashboard->selectCard("slash");  break;
	case Qt::Key_J: dashboard->selectCard("jink"); break;
	case Qt::Key_P: dashboard->selectCard("wine"); break;

	case Qt::Key_E: dashboard->selectCard("equip"); break;
	case Qt::Key_W: dashboard->selectCard("weapon"); break;
	case Qt::Key_H: dashboard->selectCard("horse"); break;

	case Qt::Key_T: dashboard->selectCard("trick"); break;
	case Qt::Key_A: dashboard->selectCard("aoe"); break;
	case Qt::Key_N: dashboard->selectCard("nullification"); break;
	case Qt::Key_Q: dashboard->selectCard("snatch"); break;
	case Qt::Key_C: dashboard->selectCard("dismantlement"); break;
	case Qt::Key_U: dashboard->selectCard("duel"); break;
	case Qt::Key_L: dashboard->selectCard("lightning"); break;
	case Qt::Key_I: dashboard->selectCard("negative_soul"); break;
	case Qt::Key_R: dashboard->selectCard("collateral"); break;
	case Qt::Key_Y: dashboard->selectCard("all_blue"); break;

	case Qt::Key_Left: dashboard->selectCard(".", false); break;
	case Qt::Key_Right:
	case Qt::Key_Space:  dashboard->selectCard("."); break; // iterate all cards
	case Qt::Key_F:  break; // fix the selected

	case Qt::Key_G: selectNextTarget(control_is_down); break; // iterate generals

	case Qt::Key_Return : {
			if(ok_button->isEnabled())
				doOkButton();
			break;
		}

	case Qt::Key_Escape : {
			if(ClientInstance->getStatus() == Client::Playing){
				dashboard->unselectAll();
				enableTargets(NULL);
			}
			break;
		}

	case Qt::Key_0:
	case Qt::Key_1:
	case Qt::Key_2:
	case Qt::Key_3:
	case Qt::Key_4:
	case Qt::Key_5:
	case Qt::Key_6:
	case Qt::Key_7:
	case Qt::Key_8:
	case Qt::Key_9:{
			int seat = event->key() - Qt::Key_0 + 1;
			int i;
			for(i=0; i<photos.length(); i++){
				if(photos.at(i)->getPlayer()->getSeat() == seat){
					selectTarget(i, control_is_down);
					break;
				}
			}
			break;
		}

	case Qt::Key_D:{
			// for debugging use

			if(chat_box_widget){
				QString msg = QString("chat_box_widget (%1, %2)")
							  .arg(chat_box_widget->x()).arg(chat_box_widget->y());

				QMessageBox::information(main_window, "", msg);
			}
		}
	}
}

void RoomScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event){
	QGraphicsScene::contextMenuEvent(event);

	QGraphicsItem *item = itemAt(event->scenePos(), QTransform());
	if(!item)
		return;

	const ClientPlayer *player = item2player.value(item, NULL);
	if(player){
		if(player == Self)
			return;

		QList<const Card *> cards = player->getCards();
		QMenu *menu = known_cards_menu;
		menu->clear();
		menu->setTitle(player->objectName());

		QAction *view = menu->addAction(tr("View in popup window ..."));
		view->setData(player->objectName());
		connect(view, SIGNAL(triggered()), this, SLOT(showPlayerCards()));

		menu->addSeparator();

		if(cards.isEmpty()){
			menu->addAction(tr("There is no known cards"))->setEnabled(false);
		}else{
			foreach(const Card *card, cards)
				menu->addAction(card->getSuitIcon(), card->getFullName());
		}

		// acquired skills
		QSet<QString> skill_names = player->getAcquiredSkills();
		QList<const Skill *> skills;
		foreach(QString skill_name, skill_names){
			const Skill *skill = Bang->getSkill(skill_name);
			if(skill && !skill->inherits("WeaponSkill") && !skill->inherits("ArmorSkill"))
				skills << skill;
		}

		if(!skills.isEmpty()){
			menu->addSeparator();
			foreach(const Skill *skill, skills){
				QString tooltip = skill->getDescription();
				menu->addAction(Bang->translate(skill->objectName()))->setToolTip(tooltip);
			}
		}

		menu->popup(event->screenPos());
	}else if(ServerInfo.FreeChoose && arrange_button){
		QGraphicsObject *obj = item->toGraphicsObject();
		if(obj && Bang->getGeneral(obj->objectName())){
			to_change = qobject_cast<CardItem *>(obj);
			change_general_menu->popup(event->screenPos());
		}
	}
}

void RoomScene::chooseGeneral(const QStringList &generals){
	QApplication::alert(main_window);
	if(!main_window->isActiveWindow())
		Bang->playAudio("prelude");
	QDialog *dialog;
	
	if(generals.isEmpty())
		dialog = new FreeChooseDialog(main_window);
	else
		dialog = new ChooseGeneralDialog(generals, main_window);
	
	delete choice_dialog;
	choice_dialog = dialog;
}

void RoomScene::chooseSuit(const QStringList &suits)
{
	QDialog *dialog = new QDialog;
	QVBoxLayout *layout = new QVBoxLayout;

	foreach(QString suit, suits){
		QCommandLinkButton *button = new QCommandLinkButton;
		button->setIcon(QIcon(QString("image/system/suit/%1.png").arg(suit)));
		button->setText(Bang->translate(suit));
		button->setObjectName(suit);

		layout->addWidget(button);

		connect(button, SIGNAL(clicked()), ClientInstance, SLOT(onPlayerChooseSuit()));
		connect(button, SIGNAL(clicked()), dialog, SLOT(accept()));
	}

	connect(dialog, SIGNAL(rejected()), ClientInstance, SLOT(onPlayerChooseSuit()));

	dialog->setObjectName(".");
	dialog->setWindowTitle(tr("Please choose a suit"));
	dialog->setLayout(layout);
	delete choice_dialog;
	choice_dialog = dialog;
}

void RoomScene::chooseKingdom(const QStringList &kingdoms)
{
	QDialog *dialog = new QDialog;
	QVBoxLayout *layout = new QVBoxLayout;

	foreach(QString kingdom, kingdoms){
		QCommandLinkButton *button = new QCommandLinkButton;
		QPixmap kingdom_pixmap(QString("image/kingdom/icon/%1.png").arg(kingdom));
		QIcon kingdom_icon(kingdom_pixmap);

		button->setIcon(kingdom_icon);
		button->setIconSize(kingdom_pixmap.size());
		button->setText(Bang->translate(kingdom));
		button->setObjectName(kingdom);

		layout->addWidget(button);

		connect(button, SIGNAL(clicked()), ClientInstance, SLOT(onPlayerChooseKingdom()));
		connect(button, SIGNAL(clicked()), dialog, SLOT(accept()));
	}

	dialog->setObjectName(".");
	connect(dialog, SIGNAL(rejected()), ClientInstance, SLOT(onPlayerChooseKingdom()));

	dialog->setObjectName(".");
	dialog->setWindowTitle(tr("Please choose a kingdom"));
	dialog->setLayout(layout);
	delete choice_dialog;
	choice_dialog = dialog;
}

void RoomScene::chooseOption(const QString &skillName, const QStringList &options)
{
	QDialog *dialog = new QDialog;
	QVBoxLayout *layout = new QVBoxLayout;
	dialog->setWindowTitle(Bang->translate(skillName));
	layout->addWidget(new QLabel(tr("Please choose:")));

	foreach(QString option, options){
		QCommandLinkButton *button = new QCommandLinkButton;
		QString text = QString("%1:%2").arg(skillName).arg(option);
		QString translated = Bang->translate(text);
		if(text == translated)
			translated = Bang->translate(option);

		button->setObjectName(option);
		button->setText(translated);

		connect(button, SIGNAL(clicked()), dialog, SLOT(accept()));
		connect(button, SIGNAL(clicked()), ClientInstance, SLOT(onPlayerMakeChoice()));

		layout->addWidget(button);
	}

	dialog->setObjectName(options.first());
	connect(dialog, SIGNAL(rejected()), ClientInstance, SLOT(onPlayerMakeChoice()));

	dialog->setLayout(layout);
	Bang->playAudio("pop-up");
	delete choice_dialog;
	choice_dialog = dialog;
}

void RoomScene::chooseCard(const ClientPlayer *player, const QString &flags, const QString &reason)
{
	PlayerCardDialog *dialog = new PlayerCardDialog(player, flags);
	dialog->setWindowTitle(Bang->translate(reason));
	connect(dialog, SIGNAL(card_id_chosen(int)), ClientInstance, SLOT(onPlayerChooseCard(int)));
	connect(dialog, SIGNAL(rejected()), ClientInstance, SLOT(onPlayerChooseCard()));
	delete choice_dialog;
	choice_dialog = dialog;
}

void RoomScene::chooseOrder(BP::Game3v3ChooseOrderCommand reason)
{
	QDialog *dialog = new QDialog;
	if(reason == BP::ChooseOrderSelect)
		dialog->setWindowTitle(tr("The order who first choose general"));
	else if(reason == BP::ChooseOrderTurn)
		dialog->setWindowTitle(tr("The order who first in turn"));

	QLabel *prompt = new QLabel(tr("Please select the order"));
	OptionButton *warm_button = new OptionButton("image/system/3v3/warm.png", tr("Warm"));
	warm_button->setObjectName("warm");
	OptionButton *cool_button = new OptionButton("image/system/3v3/cool.png", tr("Cool"));
	cool_button->setObjectName("cool");

	QHBoxLayout *hlayout = new QHBoxLayout;
	hlayout->addWidget(warm_button);
	hlayout->addWidget(cool_button);

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(prompt);
	layout->addLayout(hlayout);
	dialog->setLayout(layout);

	connect(warm_button, SIGNAL(clicked()), ClientInstance, SLOT(onPlayerChooseOrder()));
	connect(cool_button, SIGNAL(clicked()), ClientInstance, SLOT(onPlayerChooseOrder()));
	connect(warm_button, SIGNAL(clicked()), dialog, SLOT(accept()));
	connect(cool_button, SIGNAL(clicked()), dialog, SLOT(accept()));
	connect(dialog, SIGNAL(rejected()), ClientInstance, SLOT(onPlayerChooseOrder()));
	delete choice_dialog;
	choice_dialog = dialog;
}

void RoomScene::chooseRole(const QString &scheme, const QStringList &roles)
{
	QDialog *dialog = new QDialog;
	dialog->setWindowTitle(tr("Select role in 3v3 mode"));

	QLabel *prompt = new QLabel(tr("Please select a role"));
	QVBoxLayout *layout = new QVBoxLayout;

	layout->addWidget(prompt);

	static QMap<QString, QString> jargon;
	if(jargon.isEmpty()){
		jargon["lord"] = tr("Warm leader");
		jargon["loyalist"] = tr("Warm guard");
		jargon["renegade"] = tr("Cool leader");
		jargon["rebel"] = tr("Cool guard");

		jargon["leader1"] = tr("Leader of Team 1");
		jargon["guard1"] = tr("Guard of Team 1");
		jargon["leader2"] = tr("Leader of Team 2");
		jargon["guard2"] = tr("Guard of Team 2");
	}
	
	QStringList possibleRoles;
	if(scheme == "AllRoles")
		possibleRoles << "lord" << "loyalist" << "renegade" << "rebel";
	else
		possibleRoles << "leader1" << "guard1" << "leader2" << "guard2";

	foreach(QString role, possibleRoles){
		QCommandLinkButton *button = new QCommandLinkButton(jargon[role]);
		if(scheme == "AllRoles")
			button->setIcon(QIcon(QString("image/system/roles/%1.png").arg(role)));
		layout->addWidget(button);
		button->setObjectName(role);
		connect(button, SIGNAL(clicked()), ClientInstance, SLOT(onPlayerChooseRole3v3()));
		connect(button, SIGNAL(clicked()), dialog, SLOT(accept()));
	}

	QCommandLinkButton *abstain_button = new QCommandLinkButton(tr("Abstain"));
	connect(abstain_button, SIGNAL(clicked()), dialog, SLOT(reject()));
	layout->addWidget(abstain_button);

	dialog->setObjectName("abstain");
	connect(dialog, SIGNAL(rejected()), ClientInstance, SLOT(onPlayerChooseRole3v3()));

	dialog->setLayout(layout);
	delete choice_dialog;
	choice_dialog = dialog;
}

void RoomScene::chooseDirection()
{
	QDialog *dialog = new QDialog;
	dialog->setWindowTitle(tr("Please select the direction"));

	QLabel *prompt = new QLabel(dialog->windowTitle());

	OptionButton *cw_button = new OptionButton("image/system/3v3/cw.png", tr("CW"));
	cw_button->setObjectName("cw");

	OptionButton *ccw_button = new OptionButton("image/system/3v3/ccw.png", tr("CCW"));
	ccw_button->setObjectName("ccw");

	QHBoxLayout *hlayout = new QHBoxLayout;
	hlayout->addWidget(cw_button);
	hlayout->addWidget(ccw_button);

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(prompt);
	layout->addLayout(hlayout);
	dialog->setLayout(layout);

	dialog->setObjectName("ccw");
	connect(ccw_button, SIGNAL(clicked()), ClientInstance, SLOT(onPlayerMakeChoice()));
	connect(ccw_button, SIGNAL(clicked()), dialog, SLOT(accept()));
	connect(cw_button, SIGNAL(clicked()), ClientInstance, SLOT(onPlayerMakeChoice()));
	connect(cw_button, SIGNAL(clicked()), dialog, SLOT(accept()));
	connect(dialog, SIGNAL(rejected()), ClientInstance, SLOT(onPlayerMakeChoice()));
	delete choice_dialog;
	choice_dialog = dialog;
}

void RoomScene::toggleDiscards(){
	CardOverview *overview = new CardOverview;
	overview->loadFromList(ClientInstance->discarded_list);
	overview->show();
}

PlayerCardContainer *RoomScene::_getPlayerCardContainer(Player::Place place, Player *player){
	if(place == Player::DiscardPile)
		return discard_pile;
	else if(place == Player::DrawPile)
		return draw_pile;
	else if(place == Player::HandlingArea)
		return handling_area;
	else if(player == NULL && place == Player::SpecialArea)
		return card_container;
	else if(player == Self)
		return dashboard;
	else
		return name2photo.value(player->objectName(), NULL);
}

void RoomScene::loseCards(int moveId, QList<CardsMoveStruct> card_moves){
	for(int i = 0; i < card_moves.size(); i++){
		CardsMoveStruct &move = card_moves[i];
		if((move.from_place == Player::HandlingArea && move.to_place == Player::DiscardPile) || (move.from_place == Player::DiscardPile && move.to_place == Player::HandlingArea)){
			continue;
		}

		card_container->m_currentPlayer = (ClientPlayer*) move.to;
		PlayerCardContainer* from_container = _getPlayerCardContainer(move.from_place, move.from);
		QList<CardItem*> cards = from_container->removeCardItems(move.card_ids, move.from_place);
		foreach(CardItem* card, cards){
			card->setHomePos(from_container->pos() + card->homePos());
			card->setPos(from_container->pos() + card->pos());
			card->goBack(true);
			card->setParentItem(NULL);
		}
		cards_move_stash[moveId].append(cards);
		keepLoseCardLog(move);
	}
}

void RoomScene::getCards(int moveId, QList<CardsMoveStruct> card_moves){
	bool doAdjust = false;
	for(int i = 0; i < card_moves.size(); i++){
		CardsMoveStruct &movement = card_moves[i];
		if((movement.from_place == Player::HandlingArea && movement.to_place == Player::DiscardPile) || (movement.from_place == Player::DiscardPile && movement.to_place == Player::HandlingArea)){
			continue;
		}

		card_container->m_currentPlayer = (ClientPlayer*) movement.to;
		PlayerCardContainer* to_container = _getPlayerCardContainer(movement.to_place, movement.to);
		QList<CardItem*> cards = cards_move_stash[moveId][i];
		for(int j = 0; j < cards.size(); j++){
			CardItem *card = cards[j];
			int card_id = card->getId();
			if(!card_moves[i].card_ids.contains(card_id)){
				cards.removeAt(j);
				j--;
			}else{
				card->setEnabled(true);
			}
		}
		bringToFront(to_container);
		to_container->addCardItems(cards, movement.to_place);
		keepGetCardLog(movement);
		if(movement.from == Self || movement.to == Self)
			doAdjust = true;
	}
	if(doAdjust)
		dashboard->adjustCards();
	cards_move_stash[moveId].clear();
}

void RoomScene::keepLoseCardLog(const CardsMoveStruct &move)
{
}

void RoomScene::keepGetCardLog(const CardsMoveStruct &move)
{
	if(move.card_ids.isEmpty()) return;
	//DrawNCards
	if(move.from_place == Player::DrawPile && move.to_place == Player::HandArea){
		QString to_general = move.to->getGeneralName();
		log_box->appendLog("#DrawNCards", to_general, QStringList(), QString(),
							QString::number(move.card_ids.length()));
	}
	if(move.from_place == Player::DiscardPile && move.to_place == Player::HandArea){
		QString to_general = move.to->getGeneralName();
		foreach(int card_id, move.card_ids)
			log_box->appendLog("$RecycleCard", to_general, QStringList(), QString::number(card_id));
	}
	if(move.from && move.from_place != Player::HandArea && move.to && move.from != move.to){
		QString from_general = move.from->getGeneralName();
		QStringList tos;
		tos << move.to->getGeneralName();
		int hide = 0;
		foreach(int card_id, move.card_ids){
			if(card_id != Card::S_UNKNOWN_CARD_ID)
				log_box->appendLog("$MoveCard", from_general, tos, QString::number(card_id));
			else
				hide++;
		}
		if(hide > 0)
			log_box->appendLog("#MoveNCards", from_general, tos, QString(),
			QString::number(hide));
	}
	if(move.from_place == Player::HandArea && move.to_place == Player::HandArea){
		QString from_general = move.from->getGeneralName();
		QStringList tos;
		tos << move.to->getGeneralName();
		bool hiden = false;
		foreach(int card_id, move.card_ids)
			if(card_id == Card::S_UNKNOWN_CARD_ID)
				hiden = true;
		if(hiden)
			log_box->appendLog("#MoveNCards", from_general, tos, QString(),
							   QString::number(move.card_ids.length()));
		else
		{
			foreach(int card_id, move.card_ids)
			log_box->appendLog("$MoveCard", from_general, tos, QString::number(card_id));
		}
	}
	if(move.from && move.to){
		// both src and dest are player
		QString type;
		if(move.to_place == Player::JudgingArea){
			const Card *trick = Bang->getCard(move.card_ids.first());
			if(trick->objectName() == "lightning")
				type = "$LightningMove";
			else
				type = "$PasteCard";
		}
		if(!type.isNull()){
			QString from_general = move.from->objectName();
			QStringList tos;
			tos << move.to->objectName();
			log_box->appendLog(type, from_general, tos, QString::number(move.card_ids.first()));
		}
	}
	if(move.from && move.to_place == Player::DrawPile){
		QString type = "$PutCard";
		QString from_general = move.from->getGeneralName();
		log_box->appendLog(type, from_general, QStringList(), QString::number(move.card_ids.first()));
	}
	if(move.from && move.to && move.from_place == Player::EquipArea && move.to_place == Player::EquipArea){
		QString type = "$Install";
		QString to_general = move.to->getGeneralName();
		foreach(int card_id, move.card_ids)
			log_box->appendLog(type, to_general, QStringList(), QString::number(card_id));
	}
			/*if(movement.from_place == Player::Special){
			CardItem *card_item = card_container->take(NULL, card_id);
			if(card_item != NULL);
			else if(movement.from == Self){
				card_item = new CardItem(Bang->getCard(card_id));
				card_item->setPos(avatar->scenePos());
			}
			else
			{
				card_item = special_card;
				card_item->hideFrame();
				card_item->showAvatar(NULL);
				special_card = NULL;
			}
		}*/
		/*if(movement.to_place == Player::Special){
			special_card = card_item;
			dstPos = avatar->scenePos();
		}*/
		   /*
	if(src)
		card_item->setOpacity(src == Self ? 1.0 : 0.0);

	if(card_item->scene() == NULL)
		addItem(card_item);

	if(src != NULL && src_place != Player::Judging){
		QString from_general;
		from_general= src->getGeneralName();
		from_general = Bang->translate(from_general);
	}
	else{
		if(src_place == Player::DiscardPile || dest_place == Player::Hand){
			card_item->deleteCardDescription();
		}
	}   */
	
}

inline uint qHash(const QPointF p) { return qHash((int)p.x()+(int)p.y()); }

void RoomScene::addSkillButton(const Skill *skill, bool from_left){
	if(ClientInstance->getReplayer())
		return;

	// check duplication
	foreach(QAbstractButton *button, skill_buttons){
		if(button->objectName() == skill->objectName())
			return;
	}

	QAbstractButton *button = NULL;

	if(skill->inherits("TriggerSkill")){
		const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(skill);
		switch(trigger_skill->getFrequency()){
		case Skill::Frequent:{
				QCheckBox *checkbox = new QCheckBox();
				checkbox->setObjectName(skill->objectName());
				checkbox->setChecked(true);

				button = checkbox;
				break;
		}
		case Skill::Limited:
		case Skill::NotFrequent:{
				const ViewAsSkill *view_as_skill = trigger_skill->getViewAsSkill();
				button = new QPushButton();
				if(view_as_skill){
					button2skill.insert(button, view_as_skill);
					connect(button, SIGNAL(clicked()), this, SLOT(doSkillButton()));
				}

				break;
		}

		case Skill::Wake:
		case Skill::Compulsory: button = new QPushButton(); break;
		}
	}else if(skill->inherits("FilterSkill")){
		const FilterSkill *filter = qobject_cast<const FilterSkill *>(skill);
		if(filter && dashboard->getFilter() == NULL)
			dashboard->setFilter(filter);
		button = new QPushButton();

	}else if(skill->inherits("ViewAsSkill")){
		button = new QPushButton();
		button2skill.insert(button, qobject_cast<const ViewAsSkill *>(skill));
		connect(button, SIGNAL(clicked()), this, SLOT(doSkillButton()));
	}else{
		button = new QPushButton;
	}

	QDialog *dialog = skill->getDialog();
	if(dialog){
		dialog->setParent(main_window, Qt::Dialog);
		connect(button, SIGNAL(clicked()), dialog, SLOT(popup()));
	}

	button->setObjectName(skill->objectName());
	button->setText(skill->getText());
	button->setToolTip(skill->getDescription());
	button->setDisabled(skill->getFrequency() == Skill::Compulsory);
	//button->setStyleSheet(Config.value("style/button").toString());

	if(skill->isLordSkill())
		button->setIcon(QIcon("image/system/roles/lord.png"));

	skill_buttons << button;
	addWidgetToSkillDock(button, from_left);
}

void RoomScene::addWidgetToSkillDock(QWidget *widget, bool from_left){
	if(widget->inherits("QComboBox"))widget->setFixedHeight(20);
	else widget->setFixedHeight(26);

	if(!from_left)
		main_window->statusBar()->addPermanentWidget(widget);
	else
		main_window->statusBar()->addWidget(widget);
}

void RoomScene::removeWidgetFromSkillDock(QWidget *widget){
	QStatusBar * bar = main_window->statusBar();
	bar->removeWidget(widget);
}

void RoomScene::acquireSkill(const ClientPlayer *player, const QString &skill_name){
	if(skill_name.startsWith("#")) return;

	QGraphicsObject *dest = getAnimationObject(player->objectName());
	QGraphicsTextItem *item = new QGraphicsTextItem(Bang->translate(skill_name), NULL);
	this->addItem(item);
	item->setFont(Config.BigFont);

	QGraphicsDropShadowEffect *drop = new QGraphicsDropShadowEffect;
	drop->setBlurRadius(5);
	drop->setOffset(0);
	drop->setColor(Qt::yellow);
	item->setGraphicsEffect(drop);

	QPropertyAnimation *move = new QPropertyAnimation(item, "pos");
	move->setStartValue(table_center_pos);
	move->setEndValue(dest->scenePos());
	move->setDuration(Config.S_REGULAR_ANIMATION_SLOW_DURAION);

	move->start(QAbstractAnimation::DeleteWhenStopped);
	connect(move, SIGNAL(finished()), item, SLOT(deleteLater()));

	QString type = "#AcquireSkill";
	QString from_general = player->getGeneralName();
	QString arg = skill_name;

	log_box->appendLog(type, from_general, QStringList(), QString(), arg);

	if(player == Self){
		addSkillButton(Bang->getSkill(skill_name));
	}
}

void RoomScene::updateSkillButtons(){
	foreach(const Skill* skill, Self->getVisibleSkillList()){
		if(skill->isLordSkill()){
			if(Self->getRole() != "lord" || ServerInfo.GameMode == "06_3v3")
				continue;
		}

		addSkillButton(skill);
	}

	addWidgetToSkillDock(role_combobox);

	// disable all skill buttons
	foreach(QAbstractButton *button, skill_buttons)
		button->setDisabled(true);
}

void RoomScene::updateRoleComboBox(const QString &new_role){
	QMap<QString, QString> normal_mode, threeV3_mode, hegemony_mode;
	normal_mode["lord"] = tr("Lord");
	normal_mode["loyalist"] = tr("Loyalist");
	normal_mode["rebel"] = tr("Rebel");
	normal_mode["renegade"] = tr("Renegade");

	threeV3_mode["lord"] = threeV3_mode["renegade"] = tr("Marshal");
	threeV3_mode["loyalist"] = threeV3_mode["rebel"] = tr("Vanguard");

	hegemony_mode["lord"] = tr("Wei");
	hegemony_mode["loyalist"] = tr("Shu");
	hegemony_mode["rebel"] = tr("Wu");
	hegemony_mode["renegade"] = tr("Qun");

	QMap<QString, QString> *map = NULL;
	switch(Bang->getRoleIndex()){
	case 4: map = &threeV3_mode; break;
	case 5: map = &hegemony_mode; break;
	default:
		map = &normal_mode;
	}

	if(ServerInfo.EnableHegemony){
		QMap<QString, QString> hegemony_roles;

		hegemony_roles["lord"] = "wei";
		hegemony_roles["loyalist"] = "shu";
		hegemony_roles["rebel"] = "wu";
		hegemony_roles["renegade"] = "qun";

		role_combobox->setItemText(1, map->value(new_role));
		role_combobox->setItemIcon(1, QIcon(QString("image/kingdom/icon/%1.png").arg(hegemony_roles[new_role])));
		role_combobox->setCurrentIndex(5);
	}
	else{
		role_combobox->setItemText(1, map->value(new_role));
		role_combobox->setItemIcon(1, QIcon(QString("image/system/roles/%1.png").arg(new_role)));
		role_combobox->setCurrentIndex(1);
	}
}

void RoomScene::enableTargets(const Card *card){
	if(card && (Self->isJilei(card) || Self->isLocked(card))){
		ok_button->setEnabled(false);
		return;
	}

	selected_targets.clear();

	// unset avatar and all photo
	foreach(QGraphicsItem *item, item2player.keys()){
		item->setSelected(false);
	}

	if(card == NULL){
		foreach(QGraphicsItem *item, item2player.keys()){
			animations->effectOut(item);
			item->setFlag(QGraphicsItem::ItemIsSelectable, false);
			item->setEnabled(true);
		}

		ok_button->setEnabled(false);
		return;
	}

	if(card->targetFixed() || ClientInstance->hasNoTargetResponsing()){
		foreach(QGraphicsItem *item, item2player.keys()){
			animations->effectOut(item);
			item->setFlag(QGraphicsItem::ItemIsSelectable, false);
		}

		ok_button->setEnabled(true);
		return;
	}

	updateTargetsEnablity(card);

	if(Config.EnableAutoTarget)
		selectNextTarget(false);

	ok_button->setEnabled(card->targetsFeasible(selected_targets, Self));
}

void RoomScene::updateTargetsEnablity(const Card *card){
	QMapIterator<QGraphicsItem *, const ClientPlayer *> itor(item2player);
	while(itor.hasNext()){
		itor.next();

		QGraphicsItem *item = itor.key();
		const ClientPlayer *player = itor.value();

		if(item->isSelected())
			continue;

		bool enabled;
		if(card) enabled = card->targetFilter(selected_targets, player, Self);
		else enabled = true;

		if(enabled)animations->effectOut(item);
		else
		{
			if(item->graphicsEffect() &&
					item->graphicsEffect()->inherits("SentbackEffect"));
			else animations->sendBack(item);
		}

		if(card)item->setFlag(QGraphicsItem::ItemIsSelectable, enabled);
	}
}

void RoomScene::updateSelectedTargets(){
	Pixmap *item = qobject_cast<Pixmap *>(sender());

	if(item == NULL)
		return;

	const Card *card = dashboard->getSelected();
	if(card){
		const ClientPlayer *player = item2player[item];
		if(item->isSelected()){
			selected_targets.append(player);
		}else{
			selected_targets.removeOne(player);
		}


		ok_button->setEnabled(card->targetsFeasible(selected_targets, Self));
	}else{
		selected_targets.clear();
	}

	updateTargetsEnablity(card);
}

void RoomScene::useSelectedCard(){
	switch(ClientInstance->getStatus()){
	case Client::Playing:{
			const Card *card = dashboard->getSelected();
			if(card)
				useCard(card);
			break;
		}
	case Client::Responsing:{
			const Card *card = dashboard->getSelected();
			if(card){
				if(ClientInstance->hasNoTargetResponsing())
					ClientInstance->onPlayerResponseCard(card);
				else
					ClientInstance->onPlayerUseCard(card, selected_targets);
				prompt_box->disappear();
			}

			dashboard->unselectAll();
			break;
		}

	case Client::Discarding: {
			const Card *card = dashboard->pendingCard();
			if(card){
				ClientInstance->onPlayerDiscardCards(card);
				dashboard->stopPending();
				prompt_box->disappear();
			}
			break;
		}

	case Client::NotActive: {
			QMessageBox::warning(main_window, tr("Warning"),
								 tr("The OK button should be disabled when client is not active!"));
			return;
		}
	case Client::AskForAG:{
		ClientInstance->onPlayerChooseAG(-1);
			return;
		}

	case Client::ExecDialog:{
			QMessageBox::warning(main_window, tr("Warning"),
								 tr("The OK button should be disabled when client is in executing dialog"));
			return;
		}

	case Client::AskForSkillInvoke:{
			prompt_box->disappear();
			ClientInstance->onPlayerInvokeSkill(true);
			break;
		}

	case Client::AskForPlayerChoose:{
			ClientInstance->onPlayerChoosePlayer(selected_targets.first());
			prompt_box->disappear();

			break;
		}

	case Client::AskForYiji:{
			const Card *card = dashboard->pendingCard();
			if(card){
				ClientInstance->onPlayerReplyYiji(card, selected_targets.first());
				dashboard->stopPending();
				prompt_box->disappear();
			}

			break;
		}

	case Client::AskForGuanxing:{
			guanxing_box->reply();

			break;
		}

	case Client::AskForGongxin:{
			ClientInstance->onPlayerReplyGongxin();
			card_container->clear();

			break;
		}
	}

	const ViewAsSkill *skill = dashboard->currentSkill();
	if(skill)
		dashboard->stopPending();
}

void RoomScene::onSelectChange()
{
	/*
	QGraphicsItem * photo = qobject_cast<QGraphicsItem*>(sender());
	if(!photo)return;
	if(photo->isSelected())animations->emphasize(photo);
	else animations->effectOut(photo);
	*/
}
void RoomScene::onEnabledChange()
{
	QGraphicsItem * photo = qobject_cast<QGraphicsItem*>(sender());
	if(!photo)return;
	if(photo->isEnabled())animations->effectOut(photo);
	else animations->sendBack(photo);
}


void RoomScene::useCard(const Card *card){
	if(card->targetFixed() || card->targetsFeasible(selected_targets, Self))
		ClientInstance->onPlayerUseCard(card, selected_targets);
	
	selected_targets.clear();
	foreach(QGraphicsItem *item, item2player.keys()){
		item->setSelected(false);
		animations->effectOut(item);
	}
	// enableTargets(NULL);
}

void RoomScene::callViewAsSkill(){
	const Card *card = dashboard->pendingCard();

	if(card == NULL)
		return;

	if(card->isAvailable(Self)){
		// use card
		dashboard->stopPending();
		useCard(card);
	}
}

void RoomScene::cancelViewAsSkill(){
	//const ViewAsSkill *skill = dashboard->currentSkill();
	dashboard->stopPending();
	//QAbstractButton *button = button2skill.key(skill, NULL);

	//if(button)
	Client::Status status = ClientInstance->getStatus();
	updateStatus(status, status);
}

#ifdef JOYSTICK_SUPPORT

void RoomScene::onJoyButtonClicked(int bit){
	QWidget *active_window = QApplication::activeWindow();

	if(active_window == main_window){
		switch(bit){
		case 1: doOkButton(); break;
		case 2: doCancelButton(); break;
		case 3: doDiscardButton(); break;
		case 4: chooseSkillButton(); break;
		}
	}else{
		switch(bit){
		case 1: {
				QList<QAbstractButton *> buttons = active_window->findChildren<QAbstractButton *>();
				foreach(QAbstractButton *button, buttons){
					if(button->underMouse() && button->isEnabled()){
						button->click();
						break;
					}
				}

				break;
			}
		case 2: {
				QDialog *dialog = qobject_cast<QDialog *>(active_window);
				if(dialog)
					dialog->reject();
				break;
			}
		}
	}
}

void RoomScene::onJoyDirectionClicked(int direction){
	QWidget *active_window = QApplication::activeWindow();

	if(active_window == main_window){
		switch(direction){
		case Joystick::Left: dashboard->selectCard(".", false); break;
		case Joystick::Right: dashboard->selectCard(".", true); break;
		case Joystick::Up: selectNextTarget(true); break;
		case Joystick::Down: selectNextTarget(false); break;
		}
	}else{
		bool next = (direction == Joystick::Right || direction == Joystick::Down);
		int index = -1;
		QList<QAbstractButton *> list = active_window->findChildren<QAbstractButton *>();

		QMutableListIterator<QAbstractButton *> itor(list);
		while(itor.hasNext()){
			QAbstractButton *button = itor.next();
			if(!button->isEnabled())
				itor.remove();
		}

		if(list.isEmpty())
			return;

		int i, n = list.length();
		for(i=0; i<n; i++){
			QAbstractButton *button = list.at(i);
			if(button->underMouse() && button->isEnabled()){
				index = i;
				break;
			}
		}

		QAbstractButton *dest = NULL;
		if(index == -1){
			dest = list.first();
		}else{
			n = list.length();
			if(!next){
				index--;
				if(index == -1)
					index += n;
			}else{
				index++;
				if(index >= n)
					index -= n;
			}

			dest = list.at(index);
		}

		QPoint center(dest->width()/2, dest->height()/2);
		QCursor::setPos(dest->mapToGlobal(center));
	}
}

#endif

void RoomScene::selectTarget(int order, bool multiple){
	QGraphicsItem *to_select = NULL;

	if(order == 0)
		to_select = avatar;
	else if(order > 0 && order <= photos.length())
		to_select = photos.at(order - 1);

	if(!multiple)
		unselectAllTargets(to_select);

	if(to_select)
		to_select->setSelected(! to_select->isSelected());
}

void RoomScene::selectNextTarget(bool multiple){
	if(!multiple)
		unselectAllTargets();

	QList<QGraphicsItem *> targets;
	foreach(Photo *photo, photos){
		if(photo->flags() & QGraphicsItem::ItemIsSelectable)
			targets << photo;
	}

	if(avatar->flags() & QGraphicsItem::ItemIsSelectable)
		targets << avatar;

	int i, j;
	for(i=0; i<targets.length(); i++){
		if(targets.at(i)->isSelected()){
			for(j=i+1; j<targets.length(); j++){
				if(!targets.at(j)->isSelected()){
					targets.at(j)->setSelected(true);
					return;
				}
			}
		}
	}

	foreach(QGraphicsItem *target, targets){
		if(!target->isSelected()){
			target->setSelected(true);
			break;
		}
	}
}

void RoomScene::unselectAllTargets(const QGraphicsItem *except){
	if(avatar != except)
		avatar->setSelected(false);

	foreach(Photo *photo, photos){
		if(photo != except)
			photo->setSelected(false);
	}
}

void RoomScene::doTimeout(){
	switch(ClientInstance->getStatus())
	{
	case Client::Playing:{
		discard_button->click();
		break;}
	case Client::Responsing:
	case Client::Discarding:
	case Client::ExecDialog:{
		doCancelButton();
		break;}
	case Client::AskForPlayerChoose:{
		ClientInstance->onPlayerChoosePlayer(NULL);
		dashboard->stopPending();
		prompt_box->disappear();
		break;}
	case Client::AskForAG:{
		int card_id = card_container->getFirstEnabled();
		if(card_id != -1)
			ClientInstance->onPlayerChooseAG(card_id);
		break;}
	case Client::AskForSkillInvoke:
	case Client::AskForYiji:{
		cancel_button->click();
		break;}
	case Client::AskForGuanxing:
	case Client::AskForGongxin:{
		ok_button->click();
		break;}
	default:
		break;
	}
}

void RoomScene::showPromptBox()
{
	bringToFront(prompt_box);
	prompt_box->appear();
}

void RoomScene::updateStatus(Client::Status oldStatus, Client::Status newStatus){
	switch(newStatus){
	case Client::NotActive:{
			if(oldStatus == Client::ExecDialog){
				if(choice_dialog != NULL && choice_dialog->isVisible()){
					choice_dialog->hide();
				}
			}
			else if(oldStatus == Client::AskForGuanxing ||
					 oldStatus == Client::AskForGongxin)
			{
				guanxing_box->hide();
				card_container->hide();
			}
			prompt_box->disappear();
			ClientInstance->getPromptDoc()->clear();

			dashboard->disableAllCards();
			selected_targets.clear();

			ok_button->setEnabled(false);
			cancel_button->setEnabled(false);
			discard_button->setEnabled(false);

			if(dashboard->currentSkill())
				dashboard->stopPending();

			foreach(Photo *photo, photos){
				photo->setOpacity(photo->getPlayer()->isAlive() ? 1.0 : 0.7);
			}

			dashboard->hideProgressBar();

			break;
		}

	case Client::Responsing: {
			showPromptBox();

			ok_button->setEnabled(false);
			cancel_button->setEnabled(ClientInstance->is_discard_action_refusable);
			discard_button->setEnabled(false);

			QString pattern = ClientInstance->getPattern();
			QRegExp rx("@@?(\\w+)!?");
			if(rx.exactMatch(pattern)){
				QString skill_name = rx.capturedTexts().at(1);
				const ViewAsSkill *skill = Bang->getViewAsSkill(skill_name);
				if(skill)
					dashboard->startPending(skill);
			}else{
				response_skill->setPattern(pattern);
				dashboard->startPending(response_skill);
			}

			break;
		}

	case Client::Playing:{
			dashboard->enableCards();
			bringToFront(dashboard);
			ok_button->setEnabled(false);
			cancel_button->setEnabled(false);
			discard_button->setEnabled(true);
			break;
		}

	case Client::Discarding:{
			showPromptBox();

			ok_button->setEnabled(false);
			cancel_button->setEnabled(ClientInstance->is_discard_action_refusable);
			discard_button->setEnabled(false);

			discard_skill->setNum(ClientInstance->discard_num);
			discard_skill->setMinNum(ClientInstance->min_num);
			discard_skill->setIncludeEquip(ClientInstance->can_discard_equip);
			dashboard->startPending(discard_skill);
			break;
		}

	case Client::ExecDialog:{
			if(choice_dialog != NULL){
				choice_dialog->setParent(main_window, Qt::Dialog);
				choice_dialog->show();
				ok_button->setEnabled(false);
				cancel_button->setEnabled(true);
				discard_button->setEnabled(false);
			}
			break;
		}

	case Client::AskForSkillInvoke:{
			QString skill_name = ClientInstance->getSkillNameToInvoke();
			foreach(QAbstractButton *button, skill_buttons){
				if(button->objectName() == skill_name){
					QCheckBox *check_box = qobject_cast<QCheckBox *>(button);
					if(check_box && check_box->isChecked()){
						ClientInstance->onPlayerInvokeSkill(true);
						return;
					}
				}
			}
			showPromptBox();
			ok_button->setEnabled(true);
			cancel_button->setEnabled(true);
			discard_button->setEnabled(false);
			break;
		}

	case Client::AskForPlayerChoose:{
			showPromptBox();

			ok_button->setEnabled(false);
			cancel_button->setEnabled(false);
			discard_button->setEnabled(false);

			QString description;
			const Skill *skill = Bang->getSkill(ClientInstance->skill_name);
			if(skill)
				description = skill->getDescription();
			else
				description = Bang->translate(ClientInstance->skill_name);

			if(!description.isEmpty() && description != ClientInstance->skill_name)
				ClientInstance->getPromptDoc()->setHtml(tr("Please choose a player<br/> <b>Source</b>: %1<br/>").arg(description));
			else
				ClientInstance->getPromptDoc()->setHtml(tr("Please choose a player"));


			choose_skill->setPlayerNames(ClientInstance->players_to_choose);
			dashboard->startPending(choose_skill);

			break;
		}

	case Client::AskForAG:{
			dashboard->disableAllCards();

			ok_button->setEnabled(ClientInstance->is_discard_action_refusable);
			cancel_button->setEnabled(false);
			discard_button->setEnabled(false);

			card_container->startChoose();

			break;
		}

	case Client::AskForYiji:{
			ok_button->setEnabled(false);
			cancel_button->setEnabled(true);
			discard_button->setEnabled(false);

			yiji_skill->setCards(ClientInstance->getPattern());
			dashboard->startPending(yiji_skill);

			showPromptBox();

			break;
		}
	case Client::AskForGuanxing:{
			ok_button->setEnabled(true);
			cancel_button->setEnabled(false);
			discard_button->setEnabled(false);

			break;
		}

	case Client::AskForGongxin:{
			ok_button->setEnabled(true);
			cancel_button->setEnabled(false);
			discard_button->setEnabled(false);

			break;
		}
	}


	foreach(QAbstractButton *button, skill_buttons){
		const ViewAsSkill *skill = button2skill.value(button, NULL);
		if(skill)
			button->setEnabled(skill->isAvailable());
		else
			button->setEnabled(true);
	}

	if(ServerInfo.OperationTimeout == 0)
		return;

	// do timeout
	if(newStatus != Client::NotActive){
		if(focused) focused->hideProgressBar();
		QApplication::alert(main_window);
		connect(dashboard, SIGNAL(progressBarTimedOut()), this, SLOT(doTimeout()));
		dashboard->showProgressBar(ClientInstance->getCountdown());
	}
}

void RoomScene::doSkillButton(){
	const ViewAsSkill *current = dashboard->currentSkill();
	if(current){
		dashboard->stopPending();
		QAbstractButton *button = button2skill.key(current);
		if(button)
			button->setEnabled(true);
	}

	QPushButton *button = qobject_cast<QPushButton *>(sender());
	const ViewAsSkill *skill = button2skill.value(button, NULL);

	if(skill){
		dashboard->startPending(skill);

		button->setEnabled(false);
		ok_button->setEnabled(false);
		cancel_button->setEnabled(true);

		const Card *card = dashboard->pendingCard();
		if(card && card->targetFixed()){
			useSelectedCard();
		}
	}
}

void RoomScene::updateTrustButton(){
	if(!ClientInstance->getReplayer()){
		bool trusting = Self->getState() == "trust";
		trust_button->update();
		dashboard->setTrust(trusting);
	}
}

static bool CompareByNumber(const Card *card1, const Card *card2){
	return card1->getNumber() < card2->getNumber();
}

void RoomScene::updatePileButton(const QString &pile_name){
	QPushButton *button = NULL;
	foreach(QAbstractButton *pile_button, skill_buttons){
		if(pile_button->objectName() == pile_name){
			button = qobject_cast<QPushButton *>(pile_button);
			break;
		}
	}

	QMenu *menu = NULL;
	if(button == NULL){
		QPushButton *push_button = new QPushButton;
		push_button->setObjectName(pile_name);

		skill_buttons << push_button;
		addWidgetToSkillDock(push_button);

		menu = new QMenu(push_button);
		push_button->setMenu(menu);
		button = push_button;
	}else{
		QPushButton *push_button = qobject_cast<QPushButton *>(button);
		menu = push_button->menu();
		if(menu == NULL){
			menu = new QMenu(push_button);
			push_button->setMenu(menu);
		}
	}

	QList<int> pile = Self->getPile(pile_name);
	if(pile.isEmpty())
		button->setText(Bang->translate(pile_name));
	else
		button->setText(QString("%1 (%2)").arg(Bang->translate(pile_name)).arg(pile.length()));

	menu->clear();

	QList<const Card *> cards;
	foreach(int card_id, pile){
		const Card *card = Bang->getCard(card_id);
		cards << card;
	}

	qSort(cards.begin(), cards.end(), CompareByNumber);
	foreach(const Card *card, cards){
		menu->addAction(card->getSuitIcon(), card->getFullName());
	}
}

void RoomScene::doOkButton(){
	if(!ok_button->isEnabled())
		return;

	useSelectedCard();
	enableTargets(NULL);
}

void RoomScene::doCancelButton(){
	switch(ClientInstance->getStatus()){
	case Client::Playing:{
			const ViewAsSkill *skill = dashboard->currentSkill();
			if(skill)
				cancelViewAsSkill();
			else
				dashboard->unselectAll();
			dashboard->stopPending();
			dashboard->enableCards();
			break;
		}

	case Client::Responsing:{
			QString pattern = ClientInstance->getPattern();
			if(!pattern.startsWith("@")){
				const ViewAsSkill *skill = dashboard->currentSkill();
				if(!skill->inherits("ResponseSkill")){
					cancelViewAsSkill();
					break;
				}
			}

			if(ClientInstance->hasNoTargetResponsing())
				ClientInstance->onPlayerResponseCard(NULL);
			else
				ClientInstance->onPlayerUseCard(NULL);
			prompt_box->disappear();
			dashboard->stopPending();
			break;
		}

	case Client::Discarding:{
			dashboard->stopPending();
			ClientInstance->onPlayerDiscardCards(NULL);
			prompt_box->disappear();
			break;
		}

	case Client::ExecDialog:{
			choice_dialog->reject();
			break;
		}

	case Client::AskForSkillInvoke:{
			ClientInstance->onPlayerInvokeSkill(false);
			prompt_box->disappear();
			break;
		}

	case Client::AskForYiji:{
			dashboard->stopPending();
			ClientInstance->onPlayerReplyYiji(NULL, NULL);
			prompt_box->disappear();
			break;
		}

	default:{
			break;
		}
	}
}

void RoomScene::doDiscardButton(){
	dashboard->stopPending();
	dashboard->unselectAll();

	if(ClientInstance->getStatus() == Client::Playing){
		ClientInstance->onPlayerUseCard(NULL);
	}
}

void RoomScene::hideAvatars(){
	if(control_panel)
		control_panel->hide();

	foreach(Photo *photo, photos)
		photo->hideAvatar();

	dashboard->hideAvatar();
}

void RoomScene::startInXSeconds(){
	if(add_robot) add_robot->hide();
	if(fill_robots) fill_robots->hide();
}

void RoomScene::changeHp(const QString &who, int delta, DamageStruct::Nature nature, bool losthp){
	// update
	Photo *photo = name2photo.value(who, NULL);
	if(photo)
		photo->update();
	else
		dashboard->update();

	QStringList list = QString("%1:%2").arg(who).arg(delta).split(":");
	doAnimation("hpChange",list);

	if(delta < 0){
		if(losthp){
			Bang->playAudio("hplost");
			return;
		}

		QString damage_effect;
		switch(delta){
		case -1: {
				ClientPlayer *player = ClientInstance->getPlayer(who);
				int r = qrand() % 3 + 1;
				if(player->getGeneral()->isMale())
					damage_effect = QString("injure1-male%1").arg(r);
				else
					damage_effect = QString("injure1-female%1").arg(r);
				break;
			}

		case -2:{
				damage_effect = "injure2";
				break;
			}

		case -3:
		default:{
				damage_effect = "injure3";
				break;
			}
		}

		Bang->playAudio(damage_effect);

		if(photo){
			//photo->setEmotion("damage");
			setEmotion(who,"damage");
			photo->tremble();
		}

		if(nature == DamageStruct::Fire)
			doAnimation("fire", QStringList() << who);
		else if(nature == DamageStruct::Thunder)
			doAnimation("lightning", QStringList() << who);

	}else if(delta > 0){
		QString type = "#Recover";
		QString from_general = ClientInstance->getPlayer(who)->getGeneralName();
		QString n = QString::number(delta);

		log_box->appendLog(type, from_general, QStringList(), QString(), n);
	}
}

void RoomScene::onStandoff(){
	freeze();

#ifdef AUDIO_SUPPORT
	Bang->playAudio("standoff");
#endif

	QDialog *dialog = new QDialog(main_window);
	dialog->resize(500, 600);
	dialog->setWindowTitle(tr("Standoff"));

	QVBoxLayout *layout = new QVBoxLayout;

	QTableWidget *table = new QTableWidget;
	fillTable(table, ClientInstance->getPlayers());

	layout->addWidget(table);
	dialog->setLayout(layout);

	addRestartButton(dialog);

	dialog->exec();
}

void RoomScene::onGameOver(){
	room_mutex.lock();
	freeze();

	bool victory = Self->property("win").toBool();

#ifdef AUDIO_SUPPORT
	QString win_effect;
	if(victory){
		win_effect = "win";
		foreach(const Player *player, ClientInstance->getPlayers()){
			if(player->property("win").toBool() && player->isCaoCao()){
				Audio::stop();

				win_effect = "win-cc";
				break;
			}
		}
	}else
		win_effect = "lose";

	Bang->playAudio(win_effect);
#endif

	QDialog *dialog = new QDialog(main_window);
	dialog->resize(500, 600);
	dialog->setWindowTitle(victory ? tr("Victory") : tr("Failure"));

	QGroupBox *winner_box = new QGroupBox(tr("Winner(s)"));
	QGroupBox *loser_box = new QGroupBox(tr("Loser(s)"));

	QTableWidget *winner_table = new QTableWidget;
	QTableWidget *loser_table = new QTableWidget;

	QVBoxLayout *winner_layout = new QVBoxLayout;
	winner_layout->addWidget(winner_table);
	winner_box->setLayout(winner_layout);

	QVBoxLayout *loser_layout = new QVBoxLayout;
	loser_layout->addWidget(loser_table);
	loser_box->setLayout(loser_layout);

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(winner_box);
	layout->addWidget(loser_box);
	dialog->setLayout(layout);

	QList<const ClientPlayer *> winner_list, loser_list;
	foreach(const ClientPlayer *player, ClientInstance->getPlayers()){
		bool win = player->property("win").toBool();
		if(win)
			winner_list << player;
		else
			loser_list << player;

		if(player != Self){
			setEmotion(player->objectName(),win ? "good" : "bad",true);
		}
	}

	fillTable(winner_table, winner_list);
	fillTable(loser_table, loser_list);

	addRestartButton(dialog);
	room_mutex.unlock();
	dialog->exec();
	
}

void RoomScene::addRestartButton(QDialog *dialog){
	dialog->resize(main_window->width()/2, dialog->height());

	bool goto_next =false;
	if(Config.GameMode.contains("_mini_") && Self->property("win").toBool()){
		QString id = Config.GameMode;
		id.replace("_mini_","");
		int stage = Config.value("MiniSceneStage",1).toInt();
		int current = id.toInt();
		if((stage == current) && stage<20)
			goto_next = true;
	}

	QPushButton *restart_button;
	  restart_button = new QPushButton(goto_next ? tr("Next Stage") : tr("Restart Game"));
	QPushButton *return_button = new QPushButton(tr("Return to main menu"));
	QHBoxLayout *hlayout = new QHBoxLayout;
	hlayout->addStretch();
	hlayout->addWidget(restart_button);

	QPushButton *save_button = new QPushButton(tr("Save record"));
	hlayout->addWidget(save_button);
	hlayout->addWidget(return_button);

	QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(dialog->layout());
	if(layout)
		layout->addLayout(hlayout);

	connect(restart_button, SIGNAL(clicked()), dialog, SLOT(accept()));
	connect(return_button, SIGNAL(clicked()), dialog, SLOT(accept()));
	connect(save_button, SIGNAL(clicked()), this, SLOT(saveReplayRecord()));
	connect(dialog, SIGNAL(accepted()), this, SIGNAL(restart()));
	connect(return_button, SIGNAL(clicked()), this, SIGNAL(return_to_start()));
}

void RoomScene::saveReplayRecord(){
	QString location = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
	QString filename = QFileDialog::getSaveFileName(main_window,
													tr("Save replay record"),
													location,
													tr("Pure text replay file (*.txt);; Image replay file (*.png)"));

	if(!filename.isEmpty()){
		ClientInstance->save(filename);
	}
}

ScriptExecutor::ScriptExecutor(QWidget *parent)
	:QDialog(parent)
{
	setWindowTitle(tr("Script execution"));

	QVBoxLayout *vlayout = new QVBoxLayout;

	vlayout->addWidget(new QLabel(tr("Please input the script that should be executed at server side:\n P = you, R = your room")));

	QTextEdit *box = new QTextEdit;
	box->setObjectName("scriptBox");
	vlayout->addWidget(box);

	QHBoxLayout *hlayout = new QHBoxLayout;
	hlayout->addStretch();

	QPushButton *ok_button = new QPushButton(tr("OK"));
	hlayout->addWidget(ok_button);

	vlayout->addLayout(hlayout);

	connect(ok_button, SIGNAL(clicked()), this, SLOT(accept()));
	connect(this, SIGNAL(accepted()), this, SLOT(doScript()));

	setLayout(vlayout);
}

void ScriptExecutor::doScript(){
	QTextEdit *box = findChild<QTextEdit *>("scriptBox");
	if(box == NULL)
		return;

	QString script = box->toPlainText();
	script = QString::fromLatin1(qCompress(script.toUtf8()));

	ClientInstance->requestCheatRunScript(script);
}

DeathNoteDialog::DeathNoteDialog(QWidget *parent)
	:QDialog(parent)
{
	setWindowTitle(tr("Death note"));

	killer = new QComboBox;
	RoomScene::FillPlayerNames(killer, true);

	victim = new QComboBox;
	RoomScene::FillPlayerNames(victim, false);

	QPushButton *ok_button = new QPushButton(tr("OK"));
	connect(ok_button, SIGNAL(clicked()), this, SLOT(accept()));

	QFormLayout *layout = new QFormLayout;
	layout->addRow(tr("Killer"), killer);
	layout->addRow(tr("Victim"), victim);

	QHBoxLayout *hlayout = new QHBoxLayout;
	hlayout->addStretch();
	hlayout->addWidget(ok_button);
	layout->addRow(hlayout);

	setLayout(layout);
}

void DeathNoteDialog::accept(){
	QDialog::accept();
	ClientInstance->requestCheatKill(killer->itemData(killer->currentIndex()).toString(),
							victim->itemData(victim->currentIndex()).toString());
}

DamageMakerDialog::DamageMakerDialog(QWidget *parent)
	:QDialog(parent)
{
	setWindowTitle(tr("Damage maker"));

	damage_source = new QComboBox;
	RoomScene::FillPlayerNames(damage_source, true);

	damage_target = new QComboBox;
	RoomScene::FillPlayerNames(damage_target, false);

	damage_nature = new QComboBox;
	damage_nature->addItem(tr("Normal"), BP::Cheat::NormalDamage);
	damage_nature->addItem(tr("Thunder"), BP::Cheat::ThunderDamage);
	damage_nature->addItem(tr("Fire"), BP::Cheat::FireDamage);
	damage_nature->addItem(tr("HP recover"), BP::Cheat::HpRecover);
	damage_nature->addItem(tr("Lose HP"), BP::Cheat::HpLose);

	damage_point = new QSpinBox;
	damage_point->setRange(1, 1000);
	damage_point->setValue(1);

	QPushButton *ok_button = new QPushButton(tr("OK"));
	connect(ok_button, SIGNAL(clicked()), this, SLOT(accept()));
	QHBoxLayout *hlayout = new QHBoxLayout;
	hlayout->addStretch();
	hlayout->addWidget(ok_button);

	QFormLayout *layout = new QFormLayout;

	layout->addRow(tr("Damage source"), damage_source);
	layout->addRow(tr("Damage target"), damage_target);
	layout->addRow(tr("Damage nature"), damage_nature);
	layout->addRow(tr("Damage point"), damage_point);
	layout->addRow(hlayout);

	setLayout(layout);

	connect(damage_nature, SIGNAL(currentIndexChanged(int)), this, SLOT(disableSource()));
}

void DamageMakerDialog::disableSource(){
	QString nature = damage_nature->itemData(damage_nature->currentIndex()).toString();
	damage_source->setEnabled(nature != "L");
}

void RoomScene::FillPlayerNames(QComboBox *combobox, bool add_none){
	if(add_none)
		combobox->addItem(tr("None"), ".");

	combobox->setIconSize(General::TinyIconSize);

	foreach(const ClientPlayer *player, ClientInstance->getPlayers()){
		QString general_name = Bang->translate(player->getGeneralName());
		if(!player->getGeneral()) continue;
		combobox->addItem(QIcon(player->getGeneral()->getPixmapPath("tiny")),
						  QString("%1 [%2]").arg(general_name).arg(player->screenName()),
						  player->objectName());
	}
}

void DamageMakerDialog::accept(){
	QDialog::accept();

	ClientInstance->requestCheatDamage(damage_source->itemData(damage_source->currentIndex()).toString(),
							damage_target->itemData(damage_target->currentIndex()).toString(),
							(DamageStruct::Nature)damage_nature->itemData(damage_nature->currentIndex()).toInt(),
							damage_point->value());
}

void RoomScene::makeDamage(){
	if(Self->getPhase() != Player::Play){
		QMessageBox::warning(main_window, tr("Warning"), tr("This function is only allowed at your play phase!"));
		return;
	}

	DamageMakerDialog *damage_maker = new DamageMakerDialog(main_window);
	damage_maker->exec();
}

void RoomScene::makeKilling(){
	if(Self->getPhase() != Player::Play){
		QMessageBox::warning(main_window, tr("Warning"), tr("This function is only allowed at your play phase!"));
		return;
	}

	DeathNoteDialog *dialog = new DeathNoteDialog(main_window);
	dialog->exec();
}

void RoomScene::makeReviving(){
	if(Self->getPhase() != Player::Play){
		QMessageBox::warning(main_window, tr("Warning"), tr("This function is only allowed at your play phase!"));
		return;
	}

	QStringList items;
	QList<const ClientPlayer*> victims;;
	foreach(const ClientPlayer *player, ClientInstance->getPlayers()){
		if(player->isDead()){
			QString general_name = Bang->translate(player->getGeneralName());
			items << QString("%1 [%2]").arg(player->screenName()).arg(general_name);
			victims << player;
		}
	}

	if(items.isEmpty()){
		QMessageBox::warning(main_window, tr("Warning"), tr("No victims now!"));
		return;
	}

	bool ok;
	QString item = QInputDialog::getItem(main_window, tr("Reviving wand"),
										 tr("Please select a player to revive"), items, 0, false, &ok);
	if(ok){
		int index = items.indexOf(item);
		ClientInstance->requestCheatRevive(victims.at(index)->objectName());
	}
}

void RoomScene::doScript(){
	ScriptExecutor *dialog = new ScriptExecutor(main_window);
	dialog->exec();
}

void RoomScene::fillTable(QTableWidget *table, const QList<const ClientPlayer *> &players){
   // table->setColumnCount(9);
	table->setColumnCount(4);
	table->setRowCount(players.length());
	table->setEditTriggers(QAbstractItemView::NoEditTriggers);

	static QStringList labels;
	if(labels.isEmpty()){
		labels << tr("General") << tr("Name") << tr("Alive");
		if(ServerInfo.EnableHegemony)
			labels << tr("Nationality");
		else
			labels << tr("Role");

	}
	table->setHorizontalHeaderLabels(labels);

	table->setSelectionBehavior(QTableWidget::SelectRows);
		
	for(int i = 0; i < players.length(); i++){
		const ClientPlayer *player = players.at(i);

		QTableWidgetItem *item = new QTableWidgetItem;
		item->setText(Bang->translate(player->getGeneralName()));
		table->setItem(i, 0, item);

		item = new QTableWidgetItem;
		item->setText(player->screenName());
		table->setItem(i, 1, item);

		item = new QTableWidgetItem;
		if(player->isAlive())
			item->setText(tr("Alive"));
		else
			item->setText(tr("Dead"));
		table->setItem(i, 2, item);

		item = new QTableWidgetItem;

		if(ServerInfo.EnableHegemony){
			QIcon icon(QString("image/kingdom/icon/%1.png").arg(player->getKingdom()));
			item->setIcon(icon);
			item->setText(Bang->translate(player->getKingdom()));
		}else{
			QIcon icon(QString("image/system/roles/%1.png").arg(player->getRole()));
			item->setIcon(icon);
			item->setText(Bang->translate(player->getRole()));
		}
		if(!player->isAlive())
			item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
		table->setItem(i, 3, item);
	}
}

void RoomScene::killPlayer(const QString &who){
	const General *general = NULL;
	room_mutex.lock();
	if(who == Self->objectName()){
		dashboard->killPlayer();
		general = Self->getGeneral();
		item2player.remove(avatar);

		if(ServerInfo.GameMode == "02_1v1")
			self_box->killPlayer(Self->getGeneralName());
	}else{
		Photo *photo = name2photo[who];
		photo->killPlayer();
		photo->setFrame(Photo::NoFrame);
		photo->setOpacity(0.7);
		photo->update();
		item2player.remove(photo);

		general = photo->getPlayer()->getGeneral();

		if(ServerInfo.GameMode == "02_1v1")
			enemy_box->killPlayer(general->objectName());
	}

	if(Config.EnableLastWord && !Self->hasFlag("marshalling"))
		general->lastWord();
	room_mutex.unlock();
}

void RoomScene::revivePlayer(const QString &who){
	if(who == Self->objectName()){
		dashboard->revivePlayer();
		item2player.insert(avatar, Self);
		updateSkillButtons();
	}else{
		Photo *photo = name2photo[who];
		photo->revivePlayer();

		item2player.insert(photo, photo->getPlayer());
	}
}

void RoomScene::takeAmazingGrace(ClientPlayer *taker, int card_id){
	QList<int> card_ids;
	card_ids.append(card_id);
	card_container->m_currentPlayer = taker;
	CardItem *copy = card_container->removeCardItems(card_ids, Player::HandArea).first();
	if(copy == NULL)
		return;

	addItem(copy);
	QList<CardItem*> items;
	items << copy;

	if(taker){
		QString type = "$TakeAG";
		QString from_general = taker->getGeneralName();
		QString card_str = QString::number(card_id);
		log_box->appendLog(type, from_general, QStringList(), card_str);
		PlayerCardContainer* container = _getPlayerCardContainer(Player::HandArea, taker);
		bringToFront(container);
		container->addCardItems(items, Player::HandArea);
	}else{
		discard_pile->addCardItems(items, Player::DiscardPile);
	}
}

void RoomScene::showCard(const QString &player_name, int card_id){
	QString card_str = QString::number(card_id);

	if(player_name == Self->objectName())
		log_box->appendLog("$ShowCard", Self->getGeneralName(), QStringList(), card_str);
	else{
		Photo *photo = name2photo.value(player_name, NULL);
		photo->showCard(card_id);

		log_box->appendLog("$ShowCard", photo->getPlayer()->getGeneralName(),
						   QStringList(), card_str);
	}
}

void RoomScene::chooseSkillButton(){
	QList<QAbstractButton *> enabled_buttons;
	foreach(QAbstractButton *skill_button, skill_buttons){
		if(skill_button->isEnabled())
			enabled_buttons << skill_button;
	}

	if(enabled_buttons.isEmpty())
		return;

	QDialog *dialog = new QDialog(main_window);
	dialog->setWindowTitle(tr("Select skill"));

	QVBoxLayout *layout = new QVBoxLayout;

	foreach(QAbstractButton *skill_button, enabled_buttons){
		QCommandLinkButton *button = new QCommandLinkButton(skill_button->text());
		connect(button, SIGNAL(clicked()), skill_button, SIGNAL(clicked()));
		connect(button, SIGNAL(clicked()), dialog, SLOT(accept()));
		layout->addWidget(button);
	}

	dialog->setLayout(layout);
	dialog->exec();
}

void RoomScene::attachSkill(const QString &skill_name, bool from_left){
	const Skill *skill = Bang->getSkill(skill_name);
	if(skill)
		addSkillButton(skill, from_left);
}

void RoomScene::detachSkill(const QString &skill_name){
	QMutableListIterator<QAbstractButton *> itor(skill_buttons);

	while(itor.hasNext()){
		itor.next();

		QAbstractButton *button = itor.value();
		if(button->objectName() == skill_name){
			removeWidgetFromSkillDock(button);
			button2skill.remove(button);
			button->deleteLater();
			itor.remove();

			break;
		}
	}

	if(dashboard->getFilter() == Bang->getSkill(skill_name)){
		dashboard->setFilter(NULL);
	}
}

void RoomScene::viewDistance(){
	DistanceViewDialog *dialog = new DistanceViewDialog(main_window);
	dialog->show();
}

void RoomScene::speak(){
	if(game_started && ServerInfo.DisableChat)
		chat_box->append(tr("This room does not allow chatting!"));
	else
		ClientInstance->speakToServer(chat_edit->text());
	chat_edit->clear();
}

void RoomScene::fillCards(const QList<int> &card_ids)
{
	bringToFront(card_container);
	card_container->fillCards(card_ids);
	card_container->show();
}

void RoomScene::doGongxin(const QList<int> &card_ids, bool enable_heart){
	fillCards(card_ids);
	if(enable_heart)
		card_container->startGongxin();
	else
		card_container->addCloseButton();
	
}


void RoomScene::showOwnerButtons(bool owner){
	if(control_panel && !game_started)
		control_panel->setVisible(owner);
}

void RoomScene::showJudgeResult(const QString &who, bool is_good){
	if(special_card){
		const ClientPlayer *player = ClientInstance->getPlayer(who);
		QString desc = QString(tr("%1's judge")).arg(Bang->translate(player->getGeneralName()));
		special_card->writeCardDesc(desc);

		special_card->setFrame(is_good ? "good" : "bad");
	}
}

void RoomScene::showPlayerCards(){
	QAction *action = qobject_cast<QAction *>(sender());

	if(action){
		QString name = action->data().toString();
		const ClientPlayer *player = ClientInstance->getPlayer(name);

		CardContainer *viewer = new CardContainer();
		viewer->addCloseButton(true);
		addItem(viewer);
		// viewer->shift();
		viewer->view(player);
		viewer->setZValue(card_container->zValue());
	}
}

KOFOrderBox::KOFOrderBox(bool self, QGraphicsScene *scene)
{
	QString basename = self ? "self" : "enemy";
	QString path = QString("image/system/1v1/%1.png").arg(basename);
	setPixmap(QPixmap(path));

	scene->addItem(this);

	int i;
	for(i=0; i<3; i++){
		Pixmap *avatar = new Pixmap("image/system/1v1/unknown.png");
		avatar->setParentItem(this);
		avatar->setPos(5, 23 + 62 *i);
		avatar->setObjectName("unknown");

		avatars[i] = avatar;
	}

	revealed = 0;
}

void KOFOrderBox::revealGeneral(const QString &name){
	if(revealed < 3){
		const General *general = Bang->getGeneral(name);
		if(general){
			Pixmap *avatar = avatars[revealed ++];
			avatar->load(general->getPixmapPath("small"));
			avatar->setObjectName(name);
		}
	}
}

void KOFOrderBox::killPlayer(const QString &general_name){
	int i;
	for(i = 0; i < revealed; i++){
		Pixmap *avatar = avatars[i];
		if(avatar->isEnabled() && avatar->objectName() == general_name){
			QPixmap pixmap("image/system/death/unknown.png");
			QGraphicsPixmapItem *death = new QGraphicsPixmapItem(pixmap, avatar);
			death->moveBy(10, 0);

			avatar->setOpacity(0.7);
			avatar->makeGray();
			avatar->setEnabled(false);

			return;
		}
	}
}

#ifdef CHAT_VOICE

SpeakThread::SpeakThread()
	:voice_obj(NULL)
{

}

void SpeakThread::run(){
	voice_obj = new QAxObject("SAPI.SpVoice", this);

	while(true){
		sem.acquire();

		if(to_speak.isEmpty())
			return;

		const QMetaObject *meta = voice_obj->metaObject();
		meta->invokeMethod(voice_obj, "Speak", Q_ARG(QString, to_speak));
	}
}

void SpeakThread::finish(){
	to_speak.clear();

	sem.release();
}

void SpeakThread::speak(const QString &text){
	to_speak = text;

	sem.release();
}

#endif

void RoomScene::onGameStart(){
#ifdef CHAT_VOICE

	if(Config.value("EnableVoice", false).toBool()){
		SpeakThread *thread = new SpeakThread;
		connect(ClientInstance, SIGNAL(text_spoken(QString)), thread, SLOT(speak(QString)));
		connect(this, SIGNAL(destroyed()), thread, SLOT(finish()));

		thread->start();
	}

#endif

	if(ServerInfo.GameMode == "06_3v3" || ServerInfo.GameMode == "02_1v1"){
		selector_box->deleteLater();
		selector_box = NULL;

		chat_widget->show();
		log_box->show();

		if(self_box && enemy_box){
			self_box->show();
			enemy_box->show();
		}
	}

	updateSkillButtons();

	if(control_panel)
		control_panel->hide();

	log_box->append(tr("<font color='white'>------- Game Start --------</font>"));

	// add free discard button
	if(ServerInfo.FreeChoose && !ClientInstance->getReplayer()){
		free_discard_button = dashboard->addButton("free-discard",
			reverse_selection_button->pos().x() + reverse_selection_button->width()
			+ room_layout->m_scenePadding, true);
		free_discard_button->setToolTip(tr("Discard cards freely"));
		FreeDiscardSkill *discard_skill = new FreeDiscardSkill(this);
		button2skill.insert(free_discard_button, discard_skill);
		connect(free_discard_button, SIGNAL(clicked()), this, SLOT(doSkillButton()));

		skill_buttons << free_discard_button;
	}

	// updateStatus(ClientInstance->getStatus(), ClientInstance->getStatus());

	QList<const ClientPlayer *> players = ClientInstance->getPlayers();
	foreach(const ClientPlayer *player, players){
		connect(player, SIGNAL(phase_changed()), log_box, SLOT(appendSeparator()));
		connect(player, SIGNAL(phase_changed()), this, SLOT(clearDiscardPile()));
	}

	trust_button->setEnabled(true);


#ifdef AUDIO_SUPPORT

	if(!Config.EnableBgMusic)
		return;

	bool play_music = false;
	if(memory->isAttached() || memory->attach()){
		memory->lock();

		char *username = static_cast<char *>(memory->data());
		const char *my_username = Config.UserName.toLatin1();
		play_music = qstrcmp(username, my_username) == 0;

		memory->unlock();
	}else if(memory->create(255)){
		memory->lock();

		void *data = memory->data();
		const char *username = Config.UserName.toLatin1();
		memcpy(data, username, qstrlen(username));

		play_music = true;

		memory->unlock();
	}

	if(!play_music)
		return;

	// start playing background music
	QString bgmusic_path = Config.value("BackgroundMusic", "audio/system/bg_playing.mp3").toString();

	Audio::playBGM(bgmusic_path);
	Audio::setBGMVolume(Config.BGMVolume);

#endif

	game_started = true;
}

void RoomScene::freeze(){
	dashboard->setEnabled(false);
	avatar->setEnabled(false);
	foreach(Photo *photo, photos){
		photo->hideProgressBar();
		photo->setEnabled(false);
	}
	item2player.clear();

	chat_edit->setEnabled(false);

#ifdef AUDIO_SUPPORT

	Audio::stopBGM();

#endif

	dashboard->hideProgressBar();

	main_window->setStatusBar(NULL);
}

void RoomScene::moveFocus(const QString &who, BP::Countdown countdown){
	if(who == Self->objectName() && focused != NULL){
		focused->hideProgressBar();
		if(focused->getPlayer()->getPhase() == Player::NotActive)
			focused->setFrame(Photo::NoFrame);
	}
	Photo *photo = name2photo[who];
	if(photo){
		if(focused != photo && focused){
			focused->hideProgressBar();
			if(focused->getPlayer()->getPhase() == Player::NotActive)
				focused->setFrame(Photo::NoFrame);
		}

		focused = photo;
		focused->showProgressBar(countdown);
		if(focused->getPlayer()->getPhase() == Player::NotActive)
			focused->setFrame(Photo::Responsing);
	}
}

void RoomScene::setEmotion(const QString &who, const QString &emotion ,bool permanent){
	Photo *photo = name2photo[who];
	if(photo){
		photo->setEmotion(emotion,permanent);
		return;
	}
	PixmapAnimation * pma = PixmapAnimation::GetPixmapAnimation(dashboard, emotion);
	if(pma){
		pma->moveBy(0,- dashboard->boundingRect().height()/2);
		pma->setZValue(8.0);
	}
}

void RoomScene::showSkillInvocation(const QString &who, const QString &skill_name){
	const ClientPlayer *player = ClientInstance->findChild<const ClientPlayer *>(who);
	if(player != Self){
		Photo *photo = name2photo.value(who);
		photo->showSkillName(skill_name);
	}
}

void RoomScene::removeLightBox(){
	QPropertyAnimation *animation = qobject_cast<QPropertyAnimation *>(sender());
	QGraphicsTextItem *line = qobject_cast<QGraphicsTextItem *>(animation->targetObject());
	removeItem(line->parentItem());
}

QGraphicsObject *RoomScene::getAnimationObject(const QString &name) const{
	if(name == Self->objectName())
		return avatar;
	else
		return name2photo.value(name);
}

void RoomScene::doMovingAnimation(const QString &name, const QStringList &args){
	Pixmap *item = new Pixmap(QString("image/system/animation/%1.png").arg(name));
	addItem(item);

	QPointF from = getAnimationObject(args.at(0))->scenePos();
	QPointF to = getAnimationObject(args.at(1))->scenePos();

	QSequentialAnimationGroup *group = new QSequentialAnimationGroup;

	QPropertyAnimation *move = new QPropertyAnimation(item, "pos");
	move->setStartValue(from);
	move->setEndValue(to);
	move->setDuration(1000);

	QPropertyAnimation *disappear = new QPropertyAnimation(item, "opacity");
	disappear->setEndValue(0.0);
	disappear->setDuration(1000);

	group->addAnimation(move);
	group->addAnimation(disappear);

	group->start(QAbstractAnimation::DeleteWhenStopped);
	connect(group, SIGNAL(finished()), item, SLOT(deleteLater()));
}

#include "playercarddialog.h"

void RoomScene::animateHpChange(const QString &, const QStringList &args)
{
	QString who = args.at(0);
	const ClientPlayer *player = ClientInstance->getPlayer(who);
	int delta = - args.at(1).toInt();
	int hp = qMax(0, player->getHp() + delta);
	int index = 5;
	if(player->getHp() + delta < player->getMaxHp())
		index = qBound(0, hp, 5);

	if(player == Self){
		int max_hp = Self->getMaxHp();

		qreal width = max_hp > 6 ? 14 : 22;
		qreal total_width = width*max_hp;
		qreal skip = (121 - total_width)/(max_hp+1);
		qreal start_x = dashboard->getRightPosition();

		for(int i = 0;i < delta;i++){
			Pixmap *aniMaga = new Pixmap;
			QPixmap *qpixmap = max_hp > 6 ? MagatamaWidget::GetSmallMagatama(index) : MagatamaWidget::GetMagatama(index);
			aniMaga->setPixmap(*qpixmap);
			addItem(aniMaga);
			aniMaga->show();
			i+=hp-delta;

			QPoint pos = QPoint(start_x + skip * (i+1) + i * width,5);
			pos.rx() += dashboard->scenePos().x();
			pos.ry() += dashboard->scenePos().y();
			aniMaga->setPos(pos);

			QPropertyAnimation *fade = new QPropertyAnimation(aniMaga,"opacity");
			fade->setEndValue(0);
			QPropertyAnimation *grow = new QPropertyAnimation(aniMaga,"scale");
			grow->setEndValue(4);

			connect(fade,SIGNAL(finished()),aniMaga,SLOT(deleteLater()));

			QParallelAnimationGroup *group = new QParallelAnimationGroup;
			group->addAnimation(fade);
			group->addAnimation(grow);

			group->start(QAbstractAnimation::DeleteWhenStopped);

			i-=hp-delta;
		}

		return;
	}

	Photo *photo = name2photo[who];
	for(int i=0;i<delta;i++){
		i+=player->getHp();
		Pixmap *aniMaga = new Pixmap(QString("image/system/magatamas/small-%1.png").arg(index));
		addItem(aniMaga);

		QPoint pos = i>=5 ? QPoint(42,69):QPoint(26,86);
		pos.rx() += (i%5)*16;
		pos.rx() += photo->scenePos().x();
		pos.ry() += photo->scenePos().y();
		aniMaga->setPos(pos);

		QPropertyAnimation *fade = new QPropertyAnimation(aniMaga,"opacity");
		fade->setEndValue(0);
		QPropertyAnimation *grow = new QPropertyAnimation(aniMaga,"scale");
		grow->setEndValue(4);

		connect(fade,SIGNAL(finished()),aniMaga,SLOT(deleteLater()));

		QParallelAnimationGroup *group = new QParallelAnimationGroup;
		group->addAnimation(fade);
		group->addAnimation(grow);

		group->start(QAbstractAnimation::DeleteWhenStopped);

		aniMaga->show();

		i-=player->getHp();
	}
}

void RoomScene::animatePopup(const QString &name, const QStringList &args)
{
	QPointF pos = getAnimationObject(args.at(0))->scenePos();

	QPixmap *item = new QPixmap(QString("image/system/animation/%1.png").arg(name));
	pos.rx()+=item->width()/2;
	pos.ry()+=item->height()/2;

	Sprite *sprite = new Sprite();
	sprite->setParent(this);
	sprite->setPixmapAtMid(*item);
	Sprite *glare = new Sprite();
	glare->setPixmapAtMid(*item);

	sprite->setResetTime(200);
	sprite->addKeyFrame(0,"opacity",0);
	sprite->addKeyFrame(400,"opacity",1);
	sprite->addKeyFrame(600,"opacity",1);
	//sprite->addKeyFrame(1000,"opacity",0);
	sprite->addKeyFrame(0,"scale",0.2,QEasingCurve::OutQuad);
	sprite->addKeyFrame(400,"scale",1);
	sprite->addKeyFrame(600,"scale",1.2);

	sprite->start();

	addItem(sprite);
	sprite->setPos(pos);
}

void RoomScene::doAppearingAnimation(const QString &name, const QStringList &args){

	if(name == "busou_haki" || name == "wine"){
		setEmotion(args.at(0),name);
		return;
	}
	Pixmap *item = new Pixmap(QString("image/system/animation/%1.png").arg(name));
	addItem(item);

	QPointF from = getAnimationObject(args.at(0))->scenePos();
	item->setPos(from);

	QPropertyAnimation *disappear = new QPropertyAnimation(item, "opacity");
	disappear->setEndValue(0.0);
	disappear->setDuration(1000);

	disappear->start(QAbstractAnimation::DeleteWhenStopped);
	connect(disappear, SIGNAL(finished()), item, SLOT(deleteLater()));
}

void RoomScene::doLightboxAnimation(const QString &, const QStringList &args){
	// hide discarded card
	QString word = args.first();
	word = Bang->translate(word);

	QGraphicsRectItem *lightbox = addRect(main_window->rect());

	lightbox->setBrush(QColor(0x20, 0x20, 0x20));
	lightbox->moveBy(-main_window->width()/2, -main_window->height()/2);

	QGraphicsTextItem *line = addText(word, Config.BigFont);
	line->setDefaultTextColor(Qt::white);
	QRectF line_rect = line->boundingRect();
	line->setPos(-line_rect.width()/2, -line_rect.height());

	line->setParentItem(lightbox);
	line->setPos(lightbox->mapFromScene(line->x(), line->y()));

	QPropertyAnimation *appear = new QPropertyAnimation(line, "opacity");
	appear->setStartValue(0.0);
	appear->setKeyValueAt(0.8, 1.0);
	appear->setEndValue(1.0);

	int duration = args.value(1, "2000").toInt();
	appear->setDuration(duration);

	appear->start();

	connect(appear, SIGNAL(finished()), this, SLOT(removeLightBox()));
}

void RoomScene::doHuashen(const QString &, const QStringList &args){
	QVariantList huashen_list = Self->tag["Huashens"].toList();
	QList<CardItem*> generals;
	foreach(QString arg, args){
		huashen_list << arg;
		CardItem *item = new CardItem(arg);
		item->scaleSmoothly(0.5);
		generals.append(item);
	}
	dashboard->addCardItems(generals, Player::SpecialArea);

	Self->tag["Huashens"] = huashen_list;
}

void RoomScene::showIndicator(const QString &from, const QString &to){
	if(Config.value("NoIndicator", false).toBool())
		return;

	QGraphicsObject *obj1 = getAnimationObject(from);
	QGraphicsObject *obj2 = getAnimationObject(to);

	if(obj1 == NULL || obj2 == NULL || obj1 == obj2)
		return;

	if(obj1 == avatar)
		obj1 = dashboard;

	if(obj2 == avatar)
		obj2 = dashboard;

	QPointF start = obj1->sceneBoundingRect().center();
	QPointF finish = obj2->sceneBoundingRect().center();

	IndicatorItem *indicator = new IndicatorItem(start,
												 finish,
												 ClientInstance->getPlayer(from));

	qreal x = qMin(start.x(), finish.x());
	qreal y = qMin(start.y(), finish.y());
	indicator->setPos(x, y);
	indicator->setZValue(9.0);

	addItem(indicator);

	indicator->doAnimation();
}

void RoomScene::doIndicate(const QString &, const QStringList &args){
	showIndicator(args.first(), args.last());
}

void RoomScene::doAnimation(const QString &name, const QStringList &args){
	static QMap<QString, AnimationFunc> map;
	if(map.isEmpty()){
		map["wine"] = &RoomScene::doAppearingAnimation;
		map["jink"] = &RoomScene::animatePopup;
		map["nullification"] = &RoomScene::doMovingAnimation;

		map["busou_haki"] = &RoomScene::doAppearingAnimation;
		map["fire"] = &RoomScene::doAppearingAnimation;
		map["lightning"] = &RoomScene::doAppearingAnimation;
		map["tornado"] = &RoomScene::doAppearingAnimation;

		map["lightbox"] = &RoomScene::doLightboxAnimation;
		map["huashen"] = &RoomScene::doHuashen;
		map["indicate"] = &RoomScene::doIndicate;

		map["hpChange"] = &RoomScene::animateHpChange;
	}

	AnimationFunc func = map.value(name, NULL);
	if(func)
		(this->*func)(name, args);
}

void RoomScene::showServerInformation(){
	QDialog *dialog = new QDialog(main_window);
	dialog->setWindowTitle(tr("Server information"));

	QHBoxLayout *layout = new QHBoxLayout;
	ServerInfoWidget *widget = new ServerInfoWidget;
	widget->fill(ServerInfo, Config.HostAddress);
	layout->addWidget(widget);
	dialog->setLayout(layout);

	dialog->show();
}

void RoomScene::kick(){
	if(Self->getRole() != "lord"){
		QMessageBox::warning(main_window, tr("Warning"), tr("Only the lord can kick!"));
		return;
	}

	if(!Config.Password.isEmpty()){
		QMessageBox::warning(main_window, tr("Warning"), tr("This function is disabled in contest mode"));
		return;
	}

	QStringList items;
	QList<const ClientPlayer *> players = ClientInstance->getPlayers();
	if(players.isEmpty())
		return;

	foreach(const ClientPlayer *player, players){
		QString general_name = Bang->translate(player->getGeneralName());
		items << QString("%1 [%2]").arg(player->screenName()).arg(general_name);
	}

	bool ok;
	QString item = QInputDialog::getItem(main_window, tr("Kick"),
										 tr("Please select the player to kick"), items, 0, false, &ok);
	if(ok){
		int index = items.indexOf(item);
		ClientInstance->kick(players.at(index)->objectName());
	}
}

void RoomScene::surrender(){
	
	 if(Self->getPhase() != Player::Play){
		QMessageBox::warning(main_window, tr("Warning"), tr("You can only initiate a surrender poll at your play phase!"));
		return;
	}

	QMessageBox::StandardButton button;
	button = QMessageBox::question(main_window, tr("Surrender"), tr("Are you sure to surrender ?"));
	if(button == QMessageBox::Ok){
		ClientInstance->requestSurrender();
	}
}

void RoomScene::fillGenerals1v1(const QStringList &names){
	selector_box = new Pixmap("image/system/1v1/select.png", true);
	selector_box->setPos(table_center_pos);
	addItem(selector_box);
	selector_box->setZValue(10000);
	
	const static int start_x = 43;
	const static int width = 86;
	const static int row_y[4] = {60, 60+120, 60+120*2, 60+120*3};

	foreach(QString name, names){
		CardItem *item =  new CardItem(name);
		item->setObjectName(name);
		general_items << item;
	}

	qShuffle(general_items);

	int i, n=names.length();
	for(i=0; i<n; i++){

		int row, column;
		if(i < 5){
			row = 1;
			column = i;
		}else{
			row = 2;
			column = i - 5;
		}

		CardItem *general_item = general_items.at(i);
		general_item->scaleSmoothly(0.4);
		general_item->setParentItem(selector_box);
		general_item->setPos(start_x + width * column, row_y[row]);
		general_item->setHomePos(general_item->pos());
	}
}

void RoomScene::fillGenerals3v3(const QStringList &names){
	QString temperature;
	if(Self->getRole().startsWith("l"))
		temperature = "warm";
	else
		temperature = "cool";

	QString path = QString("image/system/3v3/select-%1.png").arg(temperature);
	selector_box = new Pixmap(path, true);
	addItem(selector_box);
	selector_box->setZValue(10000);
	selector_box->setPos(table_center_pos);

	const static int start_x = 62;
	const static int width = 148-62;
	const static int row_y[4] = {85, 206, 329, 451};

	int n = names.length();
	for(int i = 0; i < n; i++){

		int row, column;
		if(i < 8){
			row = 1;
			column = i;
		}else{
			row = 2;
			column = i - 8;
		}

		CardItem *general_item = new CardItem(names.at(i));
		general_item->scaleSmoothly(0.4);
		general_item->setParentItem(selector_box);
		general_item->setPos(start_x + width * column, row_y[row]);
		general_item->setHomePos(general_item->pos());
		general_item->setObjectName(names.at(i));

		general_items << general_item;
	}
}

void RoomScene::fillGenerals(const QStringList &names){
	if(ServerInfo.GameMode == "06_3v3")
		fillGenerals3v3(names);
	else if(ServerInfo.GameMode == "02_1v1")
		fillGenerals1v1(names);
}

void RoomScene::bringToFront(QGraphicsItem* front_item)
{
	z_value_mutex.lock();
	if(_m_last_front_item != NULL)
		_m_last_front_item->setZValue(last_front_z_value);
	_m_last_front_item = front_item;
	last_front_z_value = front_item->zValue();
	front_item->setZValue(10000);
	z_value_mutex.unlock();
}

void RoomScene::takeGeneral(const QString &who, const QString &name){
	bool self_taken;
	if(who == "warm")
		self_taken = Self->getRole().startsWith("l");
	else
		self_taken = Self->getRole().startsWith("r");
	QList<CardItem *> *to_add = self_taken ? &down_generals : &up_generals;

	CardItem *general_item = NULL;
	foreach(CardItem *item, general_items){
		if(item->objectName() == name){
			general_item = item;
			break;
		}
	}

	Q_ASSERT(general_item);

	general_item->disconnect(this);
	general_items.removeOne(general_item);
	to_add->append(general_item);

	int x,y;
	if(ServerInfo.GameMode == "06_3v3"){
		x = 62 + (to_add->length() - 1) * (148-62);
		y = self_taken ? 451 : 85;
	}else{
		x = 43 + (to_add->length() - 1) * 86;
		y = self_taken ? 60 + 120 * 3 : 60;
	}

	general_item->setHomePos(QPointF(x, y));
	general_item->goBack(true);
}

void RoomScene::recoverGeneral(int index, const QString &name){
	QString obj_name = QString("x%1").arg(index);

	foreach(CardItem *item, general_items){
		if(item->objectName() == obj_name){
			item->changeGeneral(name);
			item->scaleSmoothly(0.4);
			break;
		}
	}
}

void RoomScene::startGeneralSelection(){
	foreach(CardItem *item, general_items){
		item->setFlag(QGraphicsItem::ItemIsFocusable);
		connect(item, SIGNAL(double_clicked()), this, SLOT(selectGeneral()));
	}
}

void RoomScene::selectGeneral(){
	CardItem *item = qobject_cast<CardItem *>(sender());

	if(item){
		ClientInstance->notifyServer(BP::TakeGeneral, item->objectName());

		foreach(CardItem *item, general_items){
			item->setFlag(QGraphicsItem::ItemIsFocusable, false);
			item->disconnect(this);
		}
	}
}

void RoomScene::changeGeneral(const QString &general){
	if(to_change && arrange_button){
		to_change->changeGeneral(general);
		to_change->scaleSmoothly(0.4);
	}
}

void RoomScene::revealGeneral(bool self, const QString &general){
	if(self)
		self_box->revealGeneral(general);
	else
		enemy_box->revealGeneral(general);
}

void RoomScene::startArrange(){
	QString mode;
	QList<QPointF> positions;
	if(ServerInfo.GameMode == "06_3v3"){
		mode = "3v3";
		positions << QPointF(233, 291)
				<< QPointF(361, 291)
				<< QPointF(489, 291);
	}else{
		mode = "1v1";
		positions << QPointF(84, 269)
				<< QPointF(214, 269)
				<< QPointF(344, 269);
	}

	selector_box->load(QString("image/system/%1/arrange.png").arg(mode));

	foreach(CardItem *item, down_generals){
		item->setFlag(QGraphicsItem::ItemIsFocusable);
		item->setAutoBack(false);
		connect(item, SIGNAL(released()), this, SLOT(toggleArrange()));
	}

	QRect rect(0, 0, 80, 120);

	foreach(QPointF pos, positions){
		QGraphicsRectItem *rect_item = new QGraphicsRectItem(rect, selector_box);
		rect_item->setPos(pos);
		rect_item->setPen(Qt::NoPen);
		arrange_rects << rect_item;
	}

	arrange_button = new Button(tr("Complete"), 0.8);
	arrange_button->setParentItem(selector_box);
	arrange_button->setPos(600, 330);
	connect(arrange_button, SIGNAL(clicked()), this, SLOT(finishArrange()));
}

void RoomScene::toggleArrange(){
	CardItem *item = qobject_cast<CardItem *>(sender());

	if(item == NULL)
		return;

	QGraphicsItem *arrange_rect = NULL;
	int index = -1, i;
	for(i = 0; i < 3; i++){
		QGraphicsItem *rect = arrange_rects.at(i);
		if(item->collidesWithItem(rect)){
			arrange_rect = rect;
			index = i;
		}
	}

	if(arrange_rect == NULL){
		if(arrange_items.contains(item)){
			arrange_items.removeOne(item);
			down_generals << item;
		}
	}else{
		arrange_items.removeOne(item);
		down_generals.removeOne(item);

		arrange_items.insert(index, item);
	}

	int n = qMin(arrange_items.length(), 3);
	for(i = 0; i < n; i++){
		QPointF pos = arrange_rects.at(i)->pos();
		CardItem *item = arrange_items.at(i);
		item->setHomePos(pos);
		item->goBack(true);
	}

	while(arrange_items.length() > 3){
		CardItem *last = arrange_items.takeLast();
		down_generals << last;
	}

	for(i=0; i<down_generals.length(); i++){
		QPointF pos;
		if(ServerInfo.GameMode == "06_3v3")
			pos = QPointF(62 + i * 86, 451);
		else
			pos = QPointF(43 + i * 86, 60 + 120 * 3);

		CardItem *item = down_generals.at(i);
		item->setHomePos(pos);
		item->goBack(true);
	}
}

void RoomScene::finishArrange(){
	if(arrange_items.length() != 3)
		return;

	arrange_button->deleteLater();
	arrange_button = NULL;

	QJsonArray names;
	foreach(CardItem *item, arrange_items)
		names.append(item->objectName());

	ClientInstance->notifyServer(BP::Arrange, names);
}

static inline void AddRoleIcon(QMap<QChar, QPixmap> &map, char c, const QString &role){
	QPixmap pixmap(QString("image/system/roles/small-%1.png").arg(role));

	QChar qc(c);
	map[qc.toUpper()] = pixmap;

	Pixmap::MakeGray(pixmap);
	map[qc.toLower()] = pixmap;
}

void RoomScene::updateRoles(const QString &roles)
{
	foreach(QGraphicsItem *item, role_items)
		removeItem(item);
	role_items.clear();

	if(ServerInfo.EnableHegemony) return;
	static QMap<QChar, QPixmap> map;
	if(map.isEmpty()){
		AddRoleIcon(map, 'Z', "lord");
		AddRoleIcon(map, 'C', "loyalist");
		AddRoleIcon(map, 'F', "rebel");
		AddRoleIcon(map, 'N', "renegade");
	}

	foreach(QChar c, roles){
		if(map.contains(c)){
			QGraphicsPixmapItem *item = addPixmap(map.value(c));
			role_items << item;
		}
	}
	updateRolesBox();
}

void RoomScene::updateRolesBox()
{
	double centerX = roles_box->boundingRect().width() / 2;
	int n = role_items.length();
	for(int i = 0; i < n; i++){
		QGraphicsPixmapItem *item = role_items[i];
		item->setParentItem(roles_box);
		item->setPos(21 * (i - n / 2) + centerX, 6);
	}
	pile_card_num_info_text_box->setTextWidth(roles_box->boundingRect().width());
	pile_card_num_info_text_box->setPos(0, 35);
}

void RoomScene::adjustPrompt()
{
	static int fitSize = 140 ;
	int height = ClientInstance->getPromptDoc()->size().height();

	QFont ft=prompt_box_widget->font();
	int fz = ft.pixelSize() * qSqrt(fitSize * 1.0 / height);
	if(fz > 21) fz = 21;

	ft.setPixelSize(fz);
	prompt_box_widget->setFont(ft);

	while(ClientInstance->getPromptDoc()->size().height() > fitSize){
		ft.setPixelSize(ft.pixelSize()-1);
		prompt_box_widget->setFont(ft);
	}
	//else m_pileCardNumInfoTextBox->setFont(QFont("SimHei",10));
}

void RoomScene::clearDiscardPile(){
	ClientPlayer *player = qobject_cast<ClientPlayer *>(sender());
	if(player == NULL || player->getPhase() == Player::NotActive){
		discard_pile->clearCards();
	}
}

void RoomScene::appendChatEdit(QString txt){
	chat_edit->setText(chat_edit->text() +  " " + txt);
	chat_edit->setFocus();
}

void RoomScene::appendChatBox(QString txt){
	QString prefix = "<img src='image/system/chatface/";
	QString suffix = ".png'></img>";
	txt=txt.replace("<#", prefix);
	txt=txt.replace("#>", suffix);
	chat_box->append(txt);
}
