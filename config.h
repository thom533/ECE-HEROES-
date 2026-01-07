#ifndef CONFIG_H
#define CONFIG_H

// Plateau taille
#define BOARD_ROWS 14
#define BOARD_COLS 20

// Items (1..ITEM_TYPES)
#define ITEM_TYPES 5

// Valeurs spéciales de cellules
#define CELL_EMPTY 0
#define CELL_WALL  (-1)

// Partie
#define MAX_LEVELS 3
#define START_LIVES 3

// Sauvegarde
#define SAVE_FILE "saves.txt"

// Affichage / input
#define USE_ANSI 1

#endif
