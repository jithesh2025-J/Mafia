#ifndef Doctor_H
#define Doctor_H
#include "Villager.h"

class Doctor : public Villager
{
    public:
    Doctor();
    Player::Role getRole() const override;
    std::string getRoleName() const override;
    bool isAlreadyHealed();
    void setHealed();
    void reset();

    private:
    bool healedOnce = false;
};

#endif

