#ifndef Joker_H
#define Joker_H
#include "Player.h"

class Joker : public Player
{
    public:
    Joker();
    Player::Role getRole() const override;
    std::string getRoleName() const override;
    void reset();
};

#endif

