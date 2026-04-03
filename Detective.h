#ifndef Detective_H
#define Detective_H
#include "Villager.h"

class Detective : public Villager
{
    public:
    Detective();
    Player::Role getRole() const override;
    std::string getRoleName() const override;
    bool isAlreadyAsked();
    void setAsked();
    void reset();

    private:
    bool askedOnce = false;
};

#endif

