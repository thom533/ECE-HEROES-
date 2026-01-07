#ifndef BOARD_H
#define BOARD_H

#include "config.h"

typedef struct {
    int cells[BOARD_ROWS][BOARD_COLS];
} Board;

int  board_in_bounds(int r, int c);
void board_swap(Board* b, int r1, int c1, int r2, int c2);

// Init sans élimination au départ (plateau simple)
void board_init_random_stable(Board* b);

// Nouveau : init plateau selon le niveau (ajoute murs au niveau 2)
void board_init_level(Board* b, int level);

#endif
