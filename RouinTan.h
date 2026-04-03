#ifndef RouinTan_H
#define RouinTan_H
#include "Villager.h"

class RouinTan : public Villager
{
    public:
    RouinTan();
    Player::Role getRole() const override;
    std::string getRoleName() const override;
    bool isAlreadyDied();
    void setOnceDead();
    void reset();

    private:
    bool onceDied = false;
};

#endif

