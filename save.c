#define _CRT_SECURE_NO_WARNINGS
#include "save.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

#define SAVE_TXT_MAGIC   "ECEH_SAVE_TXT"
#define SAVE_TXT_VERSION 1

static void make_save_filename(const char* pseudo, char out[260]) {
    /* Exemple : save_Vianney.txt */
    snprintf(out, 260, "save_%s.txt", pseudo);
}

int save_game_state_txt(const char* pseudo, const SaveGameState* st) {
    if (!pseudo || !pseudo[0] || !st) return 0;

    char path[260];
    make_save_filename(pseudo, path);

    FILE* f = fopen(path, "w");
    if (!f) return 0;

    /* Header */
    fprintf(f, "%s %d\n", SAVE_TXT_MAGIC, SAVE_TXT_VERSION);

    /* Meta */
    fprintf(f, "LEVEL %d\n", st->level);
    fprintf(f, "LIVES %d\n", st->lives);

    /* UI */
    fprintf(f, "CUR %d %d\n", st->curR, st->curC);
    fprintf(f, "SEL %d %d %d\n", st->selOn, st->selR, st->selC);

    /* HUD */
    fprintf(f, "HUD %d %d %d %d\n",
            st->hud.level, st->hud.lives, st->hud.movesLeft, st->hud.secondsLeft);

    fprintf(f, "TARGETS");
    for (int i = 0; i < ITEM_TYPES; i++) fprintf(f, " %d", st->hud.target[i]);
    fprintf(f, "\n");

    fprintf(f, "ACHIEVED");
    for (int i = 0; i < ITEM_TYPES; i++) fprintf(f, " %d", st->hud.achieved[i]);
    fprintf(f, "\n");

    /* Board */
    fprintf(f, "BOARD %d %d\n", BOARD_ROWS, BOARD_COLS);
    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; c++) {
            fprintf(f, "%d%c", st->board.cells[r][c], (c + 1 < BOARD_COLS) ? ' ' : '\n');
        }
    }

    fclose(f);
    return 1;
}

int load_game_state_txt(const char* pseudo, SaveGameState* st) {
    if (!pseudo || !pseudo[0] || !st) return 0;

    char path[260];
    make_save_filename(pseudo, path);

    FILE* f = fopen(path, "r");
    if (!f) return 0;

    /* Vérif header */
    char magic[64] = {0};
    int version = 0;
    if (fscanf(f, "%63s %d", magic, &version) != 2) { fclose(f); return 0; }
    if (strcmp(magic, SAVE_TXT_MAGIC) != 0) { fclose(f); return 0; }
    if (version != SAVE_TXT_VERSION) { fclose(f); return 0; }

    /* Lecture ligne par ligne "tag" */
    char tag[64];

    if (fscanf(f, "%63s %d", tag, &st->level) != 2) { fclose(f); return 0; }   /* LEVEL */
    if (strcmp(tag, "LEVEL") != 0) { fclose(f); return 0; }

    if (fscanf(f, "%63s %d", tag, &st->lives) != 2) { fclose(f); return 0; }   /* LIVES */
    if (strcmp(tag, "LIVES") != 0) { fclose(f); return 0; }

    if (fscanf(f, "%63s %d %d", tag, &st->curR, &st->curC) != 3) { fclose(f); return 0; } /* CUR */
    if (strcmp(tag, "CUR") != 0) { fclose(f); return 0; }

    if (fscanf(f, "%63s %d %d %d", tag, &st->selOn, &st->selR, &st->selC) != 4) { fclose(f); return 0; } /* SEL */
    if (strcmp(tag, "SEL") != 0) { fclose(f); return 0; }

    /* HUD */
    if (fscanf(f, "%63s %d %d %d %d", tag,
               &st->hud.level, &st->hud.lives, &st->hud.movesLeft, &st->hud.secondsLeft) != 5) {
        fclose(f); return 0;
    }
    if (strcmp(tag, "HUD") != 0) { fclose(f); return 0; }

    if (fscanf(f, "%63s", tag) != 1) { fclose(f); return 0; } /* TARGETS */
    if (strcmp(tag, "TARGETS") != 0) { fclose(f); return 0; }
    for (int i = 0; i < ITEM_TYPES; i++) {
        if (fscanf(f, "%d", &st->hud.target[i]) != 1) { fclose(f); return 0; }
    }

    if (fscanf(f, "%63s", tag) != 1) { fclose(f); return 0; } /* ACHIEVED */
    if (strcmp(tag, "ACHIEVED") != 0) { fclose(f); return 0; }
    for (int i = 0; i < ITEM_TYPES; i++) {
        if (fscanf(f, "%d", &st->hud.achieved[i]) != 1) { fclose(f); return 0; }
    }

    /* Board */
    int rows = 0, cols = 0;
    if (fscanf(f, "%63s %d %d", tag, &rows, &cols) != 3) { fclose(f); return 0; } /* BOARD */
    if (strcmp(tag, "BOARD") != 0) { fclose(f); return 0; }
    if (rows != BOARD_ROWS || cols != BOARD_COLS) { fclose(f); return 0; }

    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; c++) {
            if (fscanf(f, "%d", &st->board.cells[r][c]) != 1) { fclose(f); return 0; }
        }
    }

    fclose(f);
    return 1;
}
