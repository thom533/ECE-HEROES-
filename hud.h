#ifndef HUD_H
#define HUD_H

#include "config.h"

typedef struct {
    int level;
    int lives;
    int movesLeft;
    int secondsLeft;

    int target[ITEM_TYPES];     // objectifs
    int achieved[ITEM_TYPES];   // réalisés
} HUD;

void hud_init_level(HUD* hud, int level, int lives);
void hud_add_removed(HUD* hud, const int removedCounts[ITEM_TYPES]);
int  hud_contract_done(const HUD* hud);

#endif
