#ifndef PLAYER_H
#define PLAYER_H
#include <iostream>
#include <vector>
#include <map>
#include <string>

class Player
{
public:
    enum class Role {MAFIA, GODFATHER, SILENCER, DOCTOR, DETECTIVE, JOKER, VILLAGER, ROUINTAN};
    Player(std::string n = "", Role role = Role::VILLAGER, bool alive = true, bool human = false);
    virtual ~Player() = default;
    virtual void reset(); 
    void setName(std::string);
    void setHuman(bool h) { isHuman = h; }
    void setRole(Role r) { role = r; }
    virtual Role getRole() const;
    virtual std::string getRoleName() const;
    std::string getName() const;
    bool isSilenced() const;
    bool isDead();
    bool isInDanger();
    bool getIsAlive() const { return isAlive; }
    bool getIsHuman() const { return isHuman; }
    void increaseVoteCount();
    int getVote();
    void die();
    void setInDanger();
    void healed();
    void silence();
    void unsilence();
    static void clearPlayersVotes(std::vector <Player*>&);
    
protected:
    std::string name;
    Role role;
    bool isAlive = true;
    bool isHuman = false;
    bool silenced = false;
    bool dead = false;
    int voteCount = 0;
    bool inDanger = false;
};

#endif

