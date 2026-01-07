
#ifndef RULES_H
#define RULES_H

#include "board.h"

// Nouveau : permet d'activer des règles selon le niveau (ex: nouvelle figure spéciale niveau 3)
void rules_set_level(int level);

// Marque les cases à supprimer (out peut être NULL)
int rules_mark_eliminations(const Board* b, int out[BOARD_ROWS][BOARD_COLS]);

// Applique suppression: met à 0, renvoie nb supprimé, remplit removedCounts[type-1]
int rules_apply_eliminations(Board* b, int removedCounts[ITEM_TYPES]);

// Test si un swap crée une élimination
int rules_swap_creates_elimination(Board* b, int r1, int c1, int r2, int c2);

// Cascade complète: suppressions + gravité + refill jusqu'à stable
int rules_apply_cascade(Board* b, int removedTotal[ITEM_TYPES]);

// Utilisé par board.c
void board_apply_gravity_and_refill(Board* b);

#endif
