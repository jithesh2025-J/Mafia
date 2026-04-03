#include "Player.h"

Player::Player(std::string n, Player::Role r, bool alive, bool human) : name(n), role(r), isAlive(alive), isHuman(human) {}

void Player::setName(std::string n) { name = n; }

Player::Role Player::getRole() const {
    return role;
}

std::string Player::getRoleName() const {
    switch (role) {
        case Role::MAFIA: return "Mafia";
        case Role::GODFATHER: return "GodFather";
        case Role::SILENCER: return "Silencer";
        case Role::DOCTOR: return "Doctor";
        case Role::DETECTIVE: return "Detective";
        case Role::JOKER: return "Joker";
        case Role::VILLAGER: return "Villager";
        case Role::ROUINTAN: return "RouinTan";
    }
    return "Unknown";
}

std::string Player::getName() const { return name; }

bool Player::isSilenced() const {return silenced; }

bool Player::isDead(){ return !isAlive; }

void Player::setInDanger() { inDanger = true; }

void Player::die() { isAlive = false; }

void Player::increaseVoteCount() { voteCount++; }

int Player::getVote(){ return voteCount; }

void Player::healed() { inDanger = false; }

void Player::silence() { silenced = true; }

void Player::unsilence() { silenced = false; }

bool Player::isInDanger(){ return inDanger;}

void Player::clearPlayersVotes(std::vector <Player*> &players)
{
    for(int i = 0; i < players.size(); i++)
        players[i]->voteCount = 0;
}

void Player::reset()
{
    silenced = false;
    voteCount = 0;
    inDanger = false;
    isAlive = true;
}

