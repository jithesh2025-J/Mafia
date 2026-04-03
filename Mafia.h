#ifndef Mafia_H
#define Mafia_H
#include "Player.h"

class Mafia : public Player
{
    public:
    Mafia();
    Player::Role getRole() const override;
    std::string getRoleName() const override;
    void reset();
};

#endif

