#ifndef STANDARDEQUIPS_H
#define STANDARDEQUIPS_H

#include "standard.h"

class Crossbow:public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE Crossbow(Card::Suit suit, int number = 1);
};

class OkamaMicrophone:public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE OkamaMicrophone(Card::Suit suit = Spade, int number = 2);
};

class WadoIchimonji:public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE WadoIchimonji(Card::Suit suit = Spade, int number = 6);
};

class SandaiKitetsu:public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE SandaiKitetsu(Card::Suit suit = Spade, int number = 5);
};

class Shigure:public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE Shigure(Card::Suit suit = Spade, int number = 12);
};

class ImpactDial:public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE ImpactDial(Card::Suit suit = Diamond, int number = 5);
};

class Yubashiri:public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE Yubashiri(Card::Suit suit = Diamond, int number = 12);
};

class Kabuto:public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE Kabuto(Card::Suit suit = Heart, int number = 5);
};

class CloudDial:public Armor{
    Q_OBJECT

public:
    Q_INVOKABLE CloudDial(Card::Suit suit, int number = 2);
};

class SoulSolid: public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE SoulSolid(Card::Suit suit, int number);
};

class Cloak: public Armor{
    Q_OBJECT

public:
    Q_INVOKABLE Cloak(Card::Suit suit, int number);
};

class StandardCardPackage: public Package{
    Q_OBJECT

public:
    StandardCardPackage();
};

#endif // STANDARDEQUIPS_H
