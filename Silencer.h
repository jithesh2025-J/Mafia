#ifndef SILENCER_H
#define SILENCER_H
#include "Mafia.h"

class Silencer : public Mafia
{
public:
    Silencer();
    Player::Role getRole() const override;
    std::string getRoleName() const override;
    bool isAlreadySilenced();
    void setSilenced();
    void reset();

private:
    bool silencedOnce;
};

#endif

