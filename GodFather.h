#ifndef GodFather_H
#define GodFather_H
#include "Mafia.h"

class GodFather : public Mafia
{
    public:
    GodFather();
    Player::Role getRole() const override;
    std::string getRoleName() const override;
    void reset();
};

#endif

