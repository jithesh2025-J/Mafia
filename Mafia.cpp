#include "Mafia.h"

Mafia::Mafia()
    : Player(){}

Player::Role Mafia::getRole() const { return Player::Role::MAFIA; }

std::string Mafia::getRoleName() const {
    return "Mafia";
}

void Mafia::reset()
{
    Player::reset();
}

