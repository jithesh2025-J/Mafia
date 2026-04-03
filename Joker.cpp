#include "Joker.h"

Joker::Joker()
    :Player(){}

Player::Role Joker::getRole() const { return Player::Role::JOKER; }

std::string Joker::getRoleName() const {
    return "Joker";
}

void Joker::reset()
{
    Player::reset();
}

