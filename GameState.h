#ifndef GAMESTATE_H
#define GAMESTATE_H

enum class GameState {
    MENU,
    GAME,
    NIGHT,
    DAY,
    VOTING,
    RESULT,
    GAME_OVER
};

enum class GamePhases { START, DAY, NIGHTVOTE, DETECTIVE, HEALER, SILENCER, ENDGAME };

GameState getVisualState(GamePhases phase);

#endif
