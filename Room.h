#ifndef ROOM_H
#define ROOM_H
#include <SFML/Graphics.hpp>
#include <optional>
#include <utility>
#include <vector>
#include <iostream>
#include "Player.h"
#include "Joker.h"
#include "Villager.h"
#include "Detective.h"
#include "Doctor.h"
#include "RouinTan.h"
#include "Mafia.h"
#include "GodFather.h"
#include "Silencer.h"
#include "commonTools.h"
#include "AllExeptions.h"
#include "Vote.h"
#include "GameState.h"
#include <string>

//const int NUMBEROFROLES = 8;

class Room
{
public:
    Room(std::string);
    ~Room();
    enum Action {VOTE, DETECT, HEAL, SILENT};
    void readAction(Action, std::string);
    void readPlayers(std::string);
    void joinPlayers(std::string);
    void getState();
    void endVote();
    
    std::string getName();
    GamePhases getCurrentPhase();
    GameState getVisualState() const;
    void addMessage(const std::string& msg);
    const std::vector<std::string>& getMessages() const;
    void clearMessages();

    /// Draws a grid of cards (one per alive player) with names inside each rectangle.
    void draw(sf::RenderTarget& target, const std::vector<Player>& players, const sf::Font& font,
              int selectedPlayerIndex, const std::vector<int>* voteCounts = nullptr) const;

    /// If \p worldPosition hits an alive player's card, returns that player's index; otherwise std::nullopt.
    std::optional<int> handlePlayerCardClick(sf::Vector2f worldPosition,
                                             const std::vector<Player>& players) const;

private:
    void checkPhase();
    std::string roomName;
    GamePhases currentPhase;
    std::vector<std::string> messages;
    void addJoker(int);
};

#endif

