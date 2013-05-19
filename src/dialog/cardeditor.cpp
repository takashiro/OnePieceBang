#include "cardeditor.h"
#include "mainwindow.h"
#include "engine.h"
#include "settings.h"
#include "pixmap.h"

#include <QFormLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QLabel>
#include <QGraphicsSceneMouseEvent>
#include <QApplication>
#include <QCursor>
#include <QKeyEvent>
#include <QMenu>
#include <QMenuBar>
#include <QGraphicsRectItem>
#include <QInputDialog>
#include <QBitmap>
#include <QClipboard>

BlackEdgeTextItem::BlackEdgeTextItem()
	:skip(0), color(Qt::black), outline(0)
{
    setFlags(ItemIsMovable | ItemIsFocusable);
}

QRectF BlackEdgeTextItem::boundingRect() const{
    if(text.isEmpty())
        return QRectF();

    QFontMetrics metric(font);

    QRectF rect;
	rect.setWidth(metric.width(text));
	rect.setHeight(metric.height());
    return rect;
}

void BlackEdgeTextItem::setSkip(int skip){
    this->skip = skip;
    prepareGeometryChange();

    Config.setValue("CardEditor/" + objectName() + "Skip", skip);
}

void BlackEdgeTextItem::setColor(const QColor &color){
    this->color = color;
}

void BlackEdgeTextItem::setOutline(int outline){
    this->outline = outline;
}

void BlackEdgeTextItem::toCenter(const QRectF &rect){
    if(text.isEmpty())
        return;

    QFontMetrics metric(font);
    setX((rect.width() - metric.width(text.at(0)))/2);

    int total_height = (metric.height() - metric.descent()) * text.length();
    setY((rect.height() - total_height)/2);
}

void BlackEdgeTextItem::setText(const QString &text){
    this->text = text;
    prepareGeometryChange();

    Config.setValue("CardEditor/" + objectName() + "Text", text);
}

void BlackEdgeTextItem::setFont(const QFont &font){
    this->font = font;
    prepareGeometryChange();

    Config.setValue("CardEditor/" + objectName() + "Font", font);
}

void BlackEdgeTextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    if(text.isEmpty())
        return;

    painter->setRenderHint(QPainter::Antialiasing);

	if(outline > 0){
        QPen pen(Qt::black);
        pen.setWidth(outline);
        painter->setPen(pen);
	}

	QFontMetrics metric(font);
	int height = metric.height() - metric.descent() + skip;

	for(int i = 0; i < text.length(); i++){
		QString text;
		text.append(this->text.at(i));

		QPainterPath path;
		path.addText(metric.width(this->text.left(i)), height, font, text);

		if(outline > 0)
			painter->drawPath(path);

		painter->fillPath(path, color);
	}

    if(hasFocus()){
        QPen red_pen(Qt::red);
        painter->setPen(red_pen);
        QRectF rect = boundingRect();
		painter->drawRect(-1, -1, rect.width() + 2, rect.height() + 2);
    }
}

AvatarRectItem::AvatarRectItem(qreal width, qreal height)
	:QGraphicsRectItem(0, 0, width, height)
{
    setFlag(ItemIsMovable);
}

void AvatarRectItem::toCenter(QGraphicsScene *scene){
    QRectF scene_rect = scene->sceneRect();
    setPos((scene_rect.width() - rect().width())/2,
           (scene_rect.height() - rect().height())/2);
}

CardScene::CardScene()
	:QGraphicsScene(QRectF(0, 0, 200, 290)), menu(NULL)
{
    photo = NULL;
    frame = new QGraphicsPixmapItem;

    name = new BlackEdgeTextItem;
    name->setObjectName("Name");
	name->setOutline(0);

    title = new BlackEdgeTextItem;
	title->setObjectName("Title");

    addItem(frame);
    addItem(name);
    addItem(title);

    resetPhoto();

    loadConfig();

	big_avatar_rect = new AvatarRectItem(94, 96);
    big_avatar_rect->hide();
    big_avatar_rect->toCenter(this);
    addItem(big_avatar_rect);

	small_avatar_rect = new AvatarRectItem(122, 50);
    small_avatar_rect->hide();
    small_avatar_rect->toCenter(this);
    addItem(small_avatar_rect);

	tiny_avatar_rect = new AvatarRectItem(42, 36);
    tiny_avatar_rect->hide();
    tiny_avatar_rect->toCenter(this);
    addItem(tiny_avatar_rect);

    done_menu = new QMenu();

    QAction *done_action = new QAction(tr("Done"), done_menu);
    connect(done_action, SIGNAL(triggered()), this, SLOT(doneMakingAvatar()));
    done_menu->addAction(done_action);
}

void CardScene::setFrame(const QString &kingdom, bool is_lord){
    QString path;
    if(is_lord){
        path = QString("diy/%1-lord.png").arg(kingdom);

        static QMap<QString, QColor> color_map;
        if(color_map.isEmpty()){
            color_map["wei"] = QColor(88, 101, 205);
            color_map["shu"] = QColor(234, 137, 72);
            color_map["wu"] = QColor(167, 221, 102);
            color_map["qun"] = QColor(146, 146, 146);
            color_map["god"] = QColor(252, 219, 85);
        }
        title->setColor(color_map.value(kingdom));
    }else{
        path = QString("diy/%1.png").arg(kingdom);
		title->setColor(Qt::black);
    }

    frame->setPixmap(QPixmap(path));

    Config.setValue("CardEditor/Kingdom", kingdom);
    Config.setValue("CardEditor/IsLord", is_lord);
}

void CardScene::setGeneralPhoto(const QString &filename){
    photo->load(filename);

    Config.setValue("CardEditor/Photo", filename);
}

void CardScene::saveConfig(){
    Config.beginGroup("CardEditor");
    Config.setValue("NamePos", name->pos());
    Config.setValue("TitlePos", title->pos());
    Config.setValue("PhotoPos", photo->pos());
    Config.endGroup();
}

void CardScene::loadConfig(){
    Config.beginGroup("CardEditor");
    name->setPos(Config.value("NamePos", QPointF(28, 206)).toPointF());
    title->setPos(Config.value("TitlePos", QPointF(49, 128)).toPointF());
    photo->setPos(Config.value("PhotoPos").toPointF());
    Config.endGroup();
}


BlackEdgeTextItem *CardScene::getNameItem() const{
    return name;
}

BlackEdgeTextItem *CardScene::getTitleItem() const{
    return title;
}

#ifdef QT_DEBUG

#include <QKeyEvent>
#include <QMessageBox>

void CardScene::keyPressEvent(QKeyEvent *event){
    QGraphicsScene::keyPressEvent(event);

    if(event->key() == Qt::Key_D){
        //QMessageBox::information(NULL, "", QString("%1, %2").arg(skill_box->x()).arg(skill_box->y()));
    }
}

#endif

void CardScene::setRatio(int ratio){
    photo->setScale(ratio / 100.0);

    Config.setValue("CardEditor/ImageRatio", ratio);
}

void CardScene::makeAvatar(AvatarRectItem *item){
    hideAvatarRects();

	item->show();
}

void CardScene::makeBigAvatar(){
    makeAvatar(big_avatar_rect);
}

void CardScene::makeSmallAvatar(){
    makeAvatar(small_avatar_rect);
}

void CardScene::makeTinyAvatar(){
    makeAvatar(tiny_avatar_rect);
}

void CardScene::doneMakingAvatar(){
    QGraphicsRectItem *avatar_rect = NULL;

    if(big_avatar_rect->isVisible())
        avatar_rect = big_avatar_rect;
    else if(small_avatar_rect->isVisible())
        avatar_rect = small_avatar_rect;
    else
        avatar_rect = tiny_avatar_rect;

    if(avatar_rect){
        avatar_rect->setPen(Qt::NoPen);

        QRectF rect(avatar_rect->scenePos(), avatar_rect->rect().size());
        emit avatar_snapped(rect);

        QPen thick_pen;
        thick_pen.setWidth(4);
        avatar_rect->setPen(thick_pen);
    }
}

void CardScene::hideAvatarRects(){
    big_avatar_rect->hide();
    small_avatar_rect->hide();
    tiny_avatar_rect->hide();
}

void CardScene::resetPhoto(){
    if(photo){
        photo->deleteLater();
        Config.remove("CardEditor/Photo");
    }

    photo = new Pixmap;
    photo->setZValue(-1);
    photo->setFlag(QGraphicsItem::ItemIsMovable);
    addItem(photo);
}

void CardScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event){
	QGraphicsItem *item = itemAt(event->scenePos(), QTransform());
    if(item){
		if(item == big_avatar_rect || item == small_avatar_rect || item == tiny_avatar_rect){
            done_menu->popup(event->screenPos());
            return;
        }
    }
}

void CardScene::setMenu(QMenu *menu){
    this->menu = menu;
}

CardEditor::CardEditor(QWidget *parent) :
    QMainWindow(parent)
{
    setWindowTitle(tr("Card editor"));

    QHBoxLayout *layout = new QHBoxLayout;
    QGraphicsView *view = new QGraphicsView;

	view->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);

    card_scene = new CardScene;
    connect(card_scene, SIGNAL(avatar_snapped(QRectF)), this, SLOT(saveAvatar(QRectF)));

    view->setScene(card_scene);
	view->setFixedSize(card_scene->sceneRect().width() + 2,
                       card_scene->sceneRect().height() + 2);

    layout->addWidget(createLeft());
    layout->addWidget(view);

    QWidget *central_widget = new QWidget;
    central_widget->setLayout(layout);
    setCentralWidget(central_widget);

    QMenuBar *menu_bar = new QMenuBar;
    setMenuBar(menu_bar);

    QMenu *file_menu = new QMenu(tr("File"));
    QAction *import = new QAction(tr("Import ..."), file_menu);
    import->setShortcut(Qt::CTRL + Qt::Key_O);
    QAction *save = new QAction(tr("Save ..."), file_menu);
    save->setShortcut(Qt::CTRL + Qt::Key_S);
    QAction *exit = new QAction(tr("Exit"), file_menu);
    exit->setShortcut(Qt::CTRL + Qt::Key_Q);

    file_menu->addAction(import);
    file_menu->addAction(save);
    file_menu->addSeparator();
    file_menu->addAction(exit);

    menu_bar->addMenu(file_menu);

    connect(import, SIGNAL(triggered()), this, SLOT(import()));
    connect(save, SIGNAL(triggered()), this, SLOT(saveImage()));
    connect(exit, SIGNAL(triggered()), this, SLOT(close()));

    QMenu *tool_menu = new QMenu(tr("Tool"));

    QAction *making_big = new QAction(tr("Make big avatar"), tool_menu);
    making_big->setShortcut(Qt::ALT + Qt::Key_B);
    connect(making_big, SIGNAL(triggered()), card_scene, SLOT(makeBigAvatar()));
    tool_menu->addAction(making_big);

    QAction *making_small = new QAction(tr("Make small avatar"), tool_menu);
    making_small->setShortcut(Qt::ALT + Qt::Key_M);
    connect(making_small, SIGNAL(triggered()), card_scene, SLOT(makeSmallAvatar()));
    tool_menu->addAction(making_small);

    QAction *making_tiny = new QAction(tr("Make tiny avatar"), tool_menu);
    making_tiny->setShortcut(Qt::ALT + Qt::Key_T);
    connect(making_tiny, SIGNAL(triggered()), card_scene, SLOT(makeTinyAvatar()));
    tool_menu->addAction(making_tiny);

    QAction *hiding_rect = new QAction(tr("Hide avatar rect"), tool_menu);
    hiding_rect->setShortcut(Qt::ALT + Qt::Key_H);
    connect(hiding_rect, SIGNAL(triggered()), card_scene, SLOT(hideAvatarRects()));
    tool_menu->addAction(hiding_rect);

    tool_menu->addSeparator();

    QAction *reset_photo = new QAction(tr("Reset photo"), tool_menu);
    reset_photo->setShortcut(Qt::ALT + Qt::Key_R);
    connect(reset_photo, SIGNAL(triggered()), card_scene, SLOT(resetPhoto()));
    tool_menu->addAction(reset_photo);

    QAction *copy_photo = new QAction(tr("Copy photo to clipboard"), tool_menu);
    copy_photo->setShortcut(Qt::CTRL + Qt::Key_C);
    connect(copy_photo, SIGNAL(triggered()), this, SLOT(copyPhoto()));
    tool_menu->addAction(copy_photo);

    menu_bar->addMenu(tool_menu);

	card_scene->setMenu(tool_menu);
}

void CardEditor::updateButtonText(const QFont &font){
    QFontDialog *dialog = qobject_cast<QFontDialog *>(sender());
    if(dialog){
        QPushButton *button = dialog2button.value(dialog, NULL);
        if(button)
            button->setText(QString("%1[%2]").arg(font.family()).arg(font.pointSize()));
    }
}

void CardEditor::saveAvatar(const QRectF &rect){
	QString type;
	switch((int) rect.height()){
		case 96:
			type = "big";
			break;
		case 50:
			type = "small";
			break;
		case 36:
			type = "tiny";
			break;
		default:
			type = "";
	}

	QString dir = QString("image/generals/%1").arg(type);
	QString filename = QFileDialog::getSaveFileName(this,
                                                    tr("Select a avatar file"),
													dir,
                                                    tr("Image file (*.png *.jpg *.bmp)"));

    if(!filename.isEmpty()){
        QImage image(rect.width(), rect.height(), QImage::Format_ARGB32);
        QPainter painter(&image);

		QPixmap pixmap = card_scene->views().first()->grab(rect.toRect());

        painter.drawPixmap(0, 0, pixmap);

        image.save(filename);
    }
}

void CardEditor::setMapping(QFontDialog *dialog, QPushButton *button){
    dialog2button.insert(dialog, button);

    connect(dialog, SIGNAL(currentFontChanged(QFont)), this, SLOT(updateButtonText(QFont)));
    connect(button, SIGNAL(clicked()), dialog, SLOT(exec()));
}

QGroupBox *CardEditor::createTextItemBox(const QString &text, const QFont &font, int skip, BlackEdgeTextItem *item){
    QGroupBox *box = new QGroupBox;

    QLineEdit *edit = new QLineEdit;
    QPushButton *font_button = new QPushButton(font.family());
    QSpinBox *size_spinbox = new QSpinBox;
    size_spinbox->setRange(1, 96);
    QSpinBox *space_spinbox = new QSpinBox;
    space_spinbox->setRange(0, 100);

    edit->setObjectName("name");
    size_spinbox->setObjectName("font");
    space_spinbox->setObjectName("space");

    QFormLayout *layout = new QFormLayout;
    layout->addRow(tr("Text"), edit);
    layout->addRow(tr("Font"), font_button);
    layout->addRow(tr("Line spacing"), space_spinbox);

    QFontDialog *font_dialog = new QFontDialog(this);
    setMapping(font_dialog, font_button);

    connect(edit, SIGNAL(textChanged(QString)), item, SLOT(setText(QString)));
    connect(font_dialog, SIGNAL(currentFontChanged(QFont)), item, SLOT(setFont(QFont)));
    connect(space_spinbox, SIGNAL(valueChanged(int)), item, SLOT(setSkip(int)));

    edit->setText(text);
    font_dialog->setCurrentFont(font);
    space_spinbox->setValue(skip);

    box->setLayout(layout);

    return box;
}

QLayout *CardEditor::createGeneralLayout(){
    kingdom_combobox = new QComboBox;
    lord_checkbox = new QCheckBox(tr("Lord"));
    QStringList kingdom_names = Bang->getKingdoms();
    foreach(QString kingdom, kingdom_names){
        QIcon icon(QString("image/kingdom/icon/%1.png").arg(kingdom));
        kingdom_combobox->addItem(icon, Bang->translate(kingdom), kingdom);
    }

    ratio_spinbox = new QSpinBox;
    ratio_spinbox->setRange(1, 1600);
    ratio_spinbox->setValue(100);
    ratio_spinbox->setSuffix(" %");

    QFormLayout *layout = new QFormLayout;
    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addWidget(kingdom_combobox);
    hlayout->addWidget(lord_checkbox);
    layout->addRow(tr("Kingdom"), hlayout);
    layout->addRow(tr("Image ratio"), ratio_spinbox);

    connect(kingdom_combobox, SIGNAL(currentIndexChanged(int)), this, SLOT(setCardFrame()));
    connect(lord_checkbox, SIGNAL(toggled(bool)), this, SLOT(setCardFrame()));
    connect(ratio_spinbox, SIGNAL(valueChanged(int)), card_scene, SLOT(setRatio(int)));

    QString kingdom = Config.value("CardEditor/Kingdom", "wei").toString();
    int is_lord = Config.value("CardEditor/IsLord", false).toBool();
    kingdom_combobox->setCurrentIndex(kingdom_names.indexOf(kingdom));
    lord_checkbox->setChecked(is_lord);
    ratio_spinbox->setValue(Config.value("CardEditor/ImageRatio", 100).toInt());
    QString photo = Config.value("CardEditor/Photo").toString();
    if(!photo.isEmpty())
        card_scene->setGeneralPhoto(photo);

    setCardFrame();

    return layout;
}

void CardEditor::closeEvent(QCloseEvent *event){
    QMainWindow::closeEvent(event);

    card_scene->saveConfig();
}

QWidget *CardEditor::createLeft(){
    QVBoxLayout *layout = new QVBoxLayout;

	QGroupBox *box = createTextItemBox(Config.value("CardEditor/TitleText", tr("Title")).toString(),
                                       Config.value("CardEditor/TitleFont", QFont("Times", 20)).value<QFont>(),
                                       Config.value("CardEditor/TitleSkip", 0).toInt(),
                                       card_scene->getTitleItem());
	box->setTitle(tr("Bounty"));
    layout->addWidget(box);

    box = createTextItemBox(Config.value("CardEditor/NameText", tr("Name")).toString(),
                            Config.value("CardEditor/NameFont", QFont("Times", 36)).value<QFont>(),
                            Config.value("CardEditor/NameSkip", 0).toInt(),
                            card_scene->getNameItem());

    box->setTitle(tr("Name"));
    layout->addWidget(box);

    layout->addLayout(createGeneralLayout());

    QWidget *widget = new QWidget;
    widget->setLayout(layout);
    return widget;
}

void CardEditor::setCardFrame(){
    QString kingdom = kingdom_combobox->itemData(kingdom_combobox->currentIndex()).toString();
    if(kingdom == "god")
        card_scene->setFrame("god", false);
    else
        card_scene->setFrame(kingdom, lord_checkbox->isChecked());
}

void CardEditor::import(){
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Select a photo file ..."),
                                                    Config.value("CardEditor/ImportPath").toString(),
                                                    tr("Images (*.png *.bmp *.jpg)")
                                                    );

    if(!filename.isEmpty()){
        card_scene->setGeneralPhoto(filename);
        Config.setValue("CardEditor/ImportPath", QFileInfo(filename).absolutePath());
    }
}

void CardEditor::saveImage(){
    QString filename = QFileDialog::getSaveFileName(this,
                                                    tr("Select a photo file ..."),
                                                    Config.value("CardEditor/ExportPath").toString(),
                                                    tr("Images (*.png *.bmp *.jpg)")
                                                    );

    if(!filename.isEmpty()){
        card_scene->clearFocus();
		QPixmap card_pixmap = card_scene->views().first()->grab();
		card_pixmap = card_pixmap.copy(1, 1, card_pixmap.width() - 2, card_pixmap.height() - 2);
		card_pixmap.save(filename, 0, 100);
        Config.setValue("CardEditor/ExportPath", QFileInfo(filename).absolutePath());
    }
}



void CardEditor::copyPhoto(){
    card_scene->clearFocus();

	QPixmap card_pixmap = card_scene->views().first()->grab();
	card_pixmap = card_pixmap.copy(1, 1, card_pixmap.width() - 2, card_pixmap.height() - 2);
	qApp->clipboard()->setPixmap(card_pixmap);
}

void MainWindow::on_actionCard_editor_triggered()
{
    static CardEditor *editor;
    if(editor == NULL)
        editor = new CardEditor(this);

    editor->show();
}
