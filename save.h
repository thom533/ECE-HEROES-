#ifndef SAVE_H
#define SAVE_H

#include "board.h"
#include "hud.h"

/* Etat complet pour reprendre exactement un niveau */
typedef struct SaveGameState {
    int level;
    int lives;

    Board board;
    HUD hud;

    int curR, curC;
    int selOn, selR, selC;
} SaveGameState;

/* Sauvegarde/charge en TEXTE (fichiers .txt) */
int save_game_state_txt(const char* pseudo, const SaveGameState* st);
int load_game_state_txt(const char* pseudo, SaveGameState* st);

#endif
