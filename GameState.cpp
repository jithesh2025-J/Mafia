#include "GameState.h"

GameState getVisualState(GamePhases phase) {
    switch (phase) {
        case GamePhases::START:
            return GameState::MENU;
        case GamePhases::DAY:
            return GameState::DAY;
        case GamePhases::NIGHTVOTE:
        case GamePhases::DETECTIVE:
        case GamePhases::HEALER:
        case GamePhases::SILENCER:
            return GameState::NIGHT; // or VOTING for some, but simple
        case GamePhases::ENDGAME:
            return GameState::RESULT;
        default:
            return GameState::MENU;
    }
}

