#include "Doctor.h"

Doctor::Doctor()
    :Villager()
{
    healedOnce = false;
}

void Doctor::reset()
{
    healedOnce = false;
    Player::reset();
}

Player::Role Doctor::getRole() const { return Player::Role::DOCTOR; }

std::string Doctor::getRoleName() const {
    return "Doctor";
}

void Doctor::setHealed()
{
    healedOnce = true;
}

bool Doctor::isAlreadyHealed(){ return healedOnce;}

