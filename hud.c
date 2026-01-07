#include "hud.h"

static void level_contract(int level, int target[ITEM_TYPES], int* moves, int* seconds) {
    for (int i = 0; i < ITEM_TYPES; i++) target[i] = 0;

    if (level == 1) {
        target[0] = 20; target[1] = 15; target[2] = 10;
        *moves = 35; *seconds = 60;
    } else if (level == 2) {
        target[0] = 25; target[3] = 20; target[4] = 15;
        *moves = 32; *seconds = 60;
    } else {
        target[1] = 30; target[2] = 25; target[4] = 20;
        *moves = 30; *seconds = 60;
    }
}

void hud_init_level(HUD* hud, int level, int lives) {
    hud->level = level;
    hud->lives = lives;

    int moves = 0, sec = 0;
    level_contract(level, hud->target, &moves, &sec);

    hud->movesLeft = moves;
    hud->secondsLeft = sec;

    for (int i = 0; i < ITEM_TYPES; i++) hud->achieved[i] = 0;
}

void hud_add_removed(HUD* hud, const int removedCounts[ITEM_TYPES]) {
    for (int i = 0; i < ITEM_TYPES; i++)
        hud->achieved[i] += removedCounts[i];
}

int hud_contract_done(const HUD* hud) {
    for (int i = 0; i < ITEM_TYPES; i++)
        if (hud->achieved[i] < hud->target[i]) return 0;
    return 1;
}
