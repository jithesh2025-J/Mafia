#include "Detective.h"

Detective::Detective()
    :Villager()
{
    askedOnce = false;
}

void Detective::reset()
{
    askedOnce = false;
    Player::reset();
}

Player::Role Detective::getRole() const { return Player::Role::DETECTIVE; }

std::string Detective::getRoleName() const {
    return "Detective";
}

bool Detective::isAlreadyAsked(){ return askedOnce; }

void Detective::setAsked(){ askedOnce = true;}

