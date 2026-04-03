#include "RouinTan.h"

RouinTan::RouinTan()
    :Villager()
{
    onceDied = false;
}

void RouinTan::reset()
{
    Player::reset();
}

bool RouinTan::isAlreadyDied(){ return onceDied;}

void RouinTan::setOnceDead()
{
    onceDied = true;
}

Player::Role RouinTan::getRole() const { return Player::Role::ROUINTAN; }

std::string RouinTan::getRoleName() const {
    return "RouinTan";
}

