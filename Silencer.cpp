#include "Silencer.h"

Silencer::Silencer()
    :Mafia()
{
    silencedOnce = false;
}

void Silencer::reset()
{
    silencedOnce = false;
    Player::reset();
}

Player::Role Silencer::getRole() const { return Player::Role::SILENCER; }

std::string Silencer::getRoleName() const {
    return "Silencer";
}

bool Silencer::isAlreadySilenced(){ return silencedOnce; }

void Silencer::setSilenced()
{
    silencedOnce = true;
}
