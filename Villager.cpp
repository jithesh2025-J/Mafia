#include "Villager.h"

Villager::Villager()
    :Player(){}

Player::Role Villager::getRole() const { return Player::Role::VILLAGER; }

std::string Villager::getRoleName() const {
    return "Villager";
}

void Villager::reset()
{
    Player::reset();
}

