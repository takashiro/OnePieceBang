#include "DiscardPile.h"
#include <QParallelAnimationGroup>

QList<CardItem*> DiscardPile::removeCardItems(const QList<int> &card_ids, Player::Place place)
{
	QList<CardItem*> result;
	pile_mutex.lock();
	result = _createCards(card_ids);
	_disperseCards(result, cards_display_region, Qt::AlignCenter, false, true);
	foreach(CardItem* card, result){
		for(int i = visible_cards.size() - 1; i >= 0; i--){
			if(visible_cards[i]->getCard()->getId() == card->getId()){
				card->setPos(visible_cards[i]->pos());
				break;
			}
		}
		
	}
	pile_mutex.unlock();
	return result;
}

bool DiscardPile::_addCardItems(QList<CardItem*> &card_items, Player::Place place)
{
	pile_mutex.lock();
	visible_cards.append(card_items);
	int numAdded = card_items.size();
	int numRemoved = visible_cards.size() - qMax(visible_card_num, numAdded + 1);
	for(int i = 0; i < numRemoved; i++){
		CardItem* toRemove = visible_cards.first();
		toRemove->setZValue(0.0);
		toRemove->setHomeOpacity(0.0);
		connect(toRemove, SIGNAL(movement_animation_finished()), this, SLOT(_destroyCard()));
		toRemove->goBack(true);
		visible_cards.removeFirst();
	}
	foreach(CardItem* card_item, visible_cards){
		card_item->setHomeOpacity(0.7);
	}
	foreach(CardItem* card_item, card_items){
		card_item->setHomeOpacity(1.0);
	}
	visible_cards.last()->setHomeOpacity(1.0);
	pile_mutex.unlock();
	adjustCards();
	return false;
}
	
void DiscardPile::adjustCards()
{
	_disperseCards(visible_cards, cards_display_region, Qt::AlignCenter, true, true);
	QParallelAnimationGroup *animation = new QParallelAnimationGroup;
	foreach(CardItem* card_item, visible_cards)
		animation->addAnimation(card_item->getGoBackAnimation(true));
	animation->start();
}

void DiscardPile::clearCards(){
	pile_mutex.lock();
	foreach(CardItem *card_item, visible_cards){
		card_item->setZValue(0.0);
		card_item->setHomeOpacity(0.0);
		connect(card_item, SIGNAL(movement_animation_finished()), this, SLOT(_destroyCard()));
	}

	foreach(CardItem *card_item, visible_cards){
		card_item->setZValue(0.0);
		card_item->setHomeOpacity(0.0);
		connect(card_item, SIGNAL(movement_animation_finished()), this, SLOT(_destroyCard()));
		card_item->goBack(true);
	}

	visible_cards.clear();

	pile_mutex.unlock();
}

// @todo: adjust here!!!
const QRect DrawPile::S_DISPLAY_CARD_REGION(0, CardItem::S_NORMAL_CARD_HEIGHT / 2, CardItem::S_NORMAL_CARD_WIDTH, CardItem::S_NORMAL_CARD_HEIGHT);

QList<CardItem*> DrawPile::removeCardItems(const QList<int> &card_ids, Player::Place place){
	QList<CardItem*> result = _createCards(card_ids);
	_disperseCards(result, S_DISPLAY_CARD_REGION, Qt::AlignCenter, false, true);
	return result;
}

bool DrawPile::_addCardItems(QList<CardItem*> &card_items, Player::Place place){
	foreach(CardItem* card_item, card_items){
		card_item->setHomeOpacity(0.0);
	}
	return true;
}
