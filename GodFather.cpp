#include "GodFather.h"

GodFather::GodFather()
    : Mafia(){}

Player::Role GodFather::getRole() const { return Player::Role::GODFATHER; }

std::string GodFather::getRoleName() const {
    return "GodFather";
}

void GodFather::reset()
{
    Player::reset();
}

