#ifndef _DISCARD_PILE_H
#define _DISCARD_PILE_H

#include "pixmap.h"
#include "player.h"
#include "carditem.h"
#include "protocol.h"
#include "generalcardcontainer.h"
#include <QGraphicsObject>
#include <QPixmap>

class DiscardPile: public PlayerCardContainer{
	Q_OBJECT
public:  
	inline DiscardPile() : PlayerCardContainer(true) {}
	virtual QList<CardItem*> removeCardItems(const QList<int> &card_ids, Player::Place place);
	inline void setSize(QSize newSize){
		setSize(newSize.width(), newSize.height());
	}
	inline void setSize(double width, double height){
		cards_display_region = QRect(0, 0, width, height);
		visible_card_num = width / CardItem::S_NORMAL_CARD_WIDTH + 1;
		resetTransform();
		setTransform(QTransform::fromTranslate(-width / 2, -height / 2), true);
	}
	inline void setVisibleCardNum(int num){visible_card_num = num;}
	inline int getVisibleCardNum(){return visible_card_num;}
	void adjustCards();
	void clearCards();
protected:
	virtual bool _addCardItems(QList<CardItem*> &card_items, Player::Place place);
	QList<CardItem*> visible_cards;
	QMutex pile_mutex;
	int visible_card_num;
	QRect cards_display_region;
};

class DrawPile: public PlayerCardContainer{
	Q_OBJECT
public:
	inline DrawPile() : PlayerCardContainer(true) {}
	virtual QList<CardItem*> removeCardItems(const QList<int> &card_ids, Player::Place place);    
protected:
	static const QRect S_DISPLAY_CARD_REGION;
	virtual bool _addCardItems(QList<CardItem*> &card_items, Player::Place place);
};

#endif
