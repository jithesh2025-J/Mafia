#ifndef Villager_H
#define Villager_H
#include "Player.h"

class Villager : public Player
{
    public:
    Villager();
    Player::Role getRole() const override;
    std::string getRoleName() const override;
    void reset();
};

#endif

