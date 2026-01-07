#include "rules.h"
#include "config.h"
#include <string.h>

/* Niveau courant (pour activer la nouvelle figure spéciale au niveau 3) */
static int g_rules_level = 1;
void rules_set_level(int level) { g_rules_level = level; }

static void clear_mask(int m[BOARD_ROWS][BOARD_COLS]) {
    for (int r = 0; r < BOARD_ROWS; r++)
        for (int c = 0; c < BOARD_COLS; c++)
            m[r][c] = 0;
}

static void mark_all_type(const Board* b, int type, int m[BOARD_ROWS][BOARD_COLS]) {
    for (int r = 0; r < BOARD_ROWS; r++)
        for (int c = 0; c < BOARD_COLS; c++)
            if (b->cells[r][c] == type) m[r][c] = 1;
}

/* =========================
    Figure spéciale (niveau 3) :
   - "L de 5" : 3 en ligne + 3 en colonne partageant une case (coin).
   - Effet : explosion 3x3 autour d1
   1u coin.
   ========================= */
static int mark_L5_blast3x3(const Board* b, int m[BOARD_ROWS][BOARD_COLS]) {
    int ok = 0;

    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; c++) {
            int t = b->cells[r][c];
            if (t <= 0) continue; // vide ou mur

            int L1 = (c + 2 < BOARD_COLS && r + 2 < BOARD_ROWS &&
                      b->cells[r][c+1] == t && b->cells[r][c+2] == t &&
                      b->cells[r+1][c] == t && b->cells[r+2][c] == t);

            int L2 = (c + 2 < BOARD_COLS && r - 2 >= 0 &&
                      b->cells[r][c+1] == t && b->cells[r][c+2] == t &&
                      b->cells[r-1][c] == t && b->cells[r-2][c] == t);

            int L3 = (c - 2 >= 0 && r + 2 < BOARD_ROWS &&
                      b->cells[r][c-1] == t && b->cells[r][c-2] == t &&
                      b->cells[r+1][c] == t && b->cells[r+2][c] == t);

            int L4 = (c - 2 >= 0 && r - 2 >= 0 &&
                      b->cells[r][c-1] == t && b->cells[r][c-2] == t &&
                      b->cells[r-1][c] == t && b->cells[r-2][c] == t);

            if (!(L1 || L2 || L3 || L4)) continue;

            ok = 1;

            // Explosion 3x3 autour du coin (r,c)
            for (int rr = r - 1; rr <= r + 1; rr++) {
                for (int cc = c - 1; cc <= c + 1; cc++) {
                    if (rr < 0 || rr >= BOARD_ROWS || cc < 0 || cc >= BOARD_COLS) continue;
                    if (b->cells[rr][cc] == CELL_WALL) continue; // mur intouchable
                    m[rr][cc] = 1;
                }
            }
        }
    }

    return ok;
}

/* suites >=3 */
static int mark_runs3(const Board* b, int m[BOARD_ROWS][BOARD_COLS]) {
    int ok = 0;

    // lignes
    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; ) {
            int t = b->cells[r][c];
            if (t <= 0) { c++; continue; } // vide ou mur
            int s = c;
            while (c + 1 < BOARD_COLS && b->cells[r][c + 1] == t) c++;
            if (c - s + 1 >= 3) {
                ok = 1;
                for (int k = s; k <= c; k++) m[r][k] = 1;
            }
            c++;
        }
    }

    // colonnes
    for (int c = 0; c < BOARD_COLS; c++) {
        for (int r = 0; r < BOARD_ROWS; ) {
            int t = b->cells[r][c];
            if (t <= 0) { r++; continue; } // vide ou mur
            int s = r;
            while (r + 1 < BOARD_ROWS && b->cells[r + 1][c] == t) r++;
            if (r - s + 1 >= 3) {
                ok = 1;
                for (int k = s; k <= r; k++) m[k][c] = 1;
            }
            r++;
        }
    }
    return ok;
}

/* suite >=6 => supprime tout le type */
static int mark_run6_global(const Board* b, int m[BOARD_ROWS][BOARD_COLS]) {
    int ok = 0;
    int types[ITEM_TYPES]; memset(types, 0, sizeof(types));

    // lignes
    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; ) {
            int t = b->cells[r][c];
            if (t <= 0) { c++; continue; } // vide ou mur
            int s = c;
            while (c + 1 < BOARD_COLS && b->cells[r][c + 1] == t) c++;
            if (c - s + 1 >= 6 && t >= 1 && t <= ITEM_TYPES) types[t - 1] = 1;
            c++;
        }
    }

    // colonnes
    for (int c = 0; c < BOARD_COLS; c++) {
        for (int r = 0; r < BOARD_ROWS; ) {
            int t = b->cells[r][c];
            if (t <= 0) { r++; continue; } // vide ou mur
            int s = r;
            while (r + 1 < BOARD_ROWS && b->cells[r + 1][c] == t) r++;
            if (r - s + 1 >= 6 && t >= 1 && t <= ITEM_TYPES) types[t - 1] = 1;
            r++;
        }
    }

    for (int i = 0; i < ITEM_TYPES; i++) {
        if (types[i]) {
            ok = 1;
            mark_all_type(b, i + 1, m);
        }
    }
    return ok;
}

/* croix (centre + haut/bas/gauche/droite) => supprime type sur ligne+colonne */
static int mark_cross5_rowcol(const Board* b, int m[BOARD_ROWS][BOARD_COLS]) {
    int ok = 0;
    for (int r = 1; r < BOARD_ROWS - 1; r++) {
        for (int c = 1; c < BOARD_COLS - 1; c++) {
            int t = b->cells[r][c];
            if (t <= 0) continue; // vide ou mur
            if (b->cells[r-1][c]==t && b->cells[r+1][c]==t &&
                b->cells[r][c-1]==t && b->cells[r][c+1]==t) {
                ok = 1;
                for (int cc = 0; cc < BOARD_COLS; cc++)
                    if (b->cells[r][cc] == t) m[r][cc] = 1;
                for (int rr = 0; rr < BOARD_ROWS; rr++)
                    if (b->cells[rr][c] == t) m[rr][c] = 1;
            }
        }
    }
    return ok;
}

int rules_mark_eliminations(const Board* b, int out[BOARD_ROWS][BOARD_COLS]) {
    int m[BOARD_ROWS][BOARD_COLS];
    clear_mask(m);

    int any = 0;

    // Nouveau : figure spéciale seulement à partir du niveau 3
    if (g_rules_level >= 3) {
        any |= mark_L5_blast3x3(b, m);
    }

    any |= mark_run6_global(b, m);
    any |= mark_cross5_rowcol(b, m);
    any |= mark_runs3(b, m);

    if (out) {
        for (int r = 0; r < BOARD_ROWS; r++)
            for (int c = 0; c < BOARD_COLS; c++)
                out[r][c] = m[r][c];
    }
    return any;
}

int rules_apply_eliminations(Board* b, int removedCounts[ITEM_TYPES]) {
    int m[BOARD_ROWS][BOARD_COLS];
    if (!rules_mark_eliminations(b, m)) return 0;

    for (int i = 0; i < ITEM_TYPES; i++) removedCounts[i] = 0;

    int removed = 0;
    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; c++) {
            if (m[r][c]) {
                int type = b->cells[r][c];

                // Mur intouchable
                if (type == CELL_WALL) continue;

                if (type >= 1 && type <= ITEM_TYPES) removedCounts[type - 1]++;
                b->cells[r][c] = CELL_EMPTY;
                removed++;
            }
        }
    }
    return removed;
}

int rules_swap_creates_elimination(Board* b, int r1, int c1, int r2, int c2) {
    board_swap(b, r1, c1, r2, c2);
    int ok = rules_mark_eliminations(b, NULL);
    board_swap(b, r1, c1, r2, c2);
    return ok;
}

int rules_apply_cascade(Board* b, int removedTotal[ITEM_TYPES]) {
    for (int i = 0; i < ITEM_TYPES; i++) removedTotal[i] = 0;

    int totalRemoved = 0;
    while (1) {
        int removedCounts[ITEM_TYPES];
        int rem = rules_apply_eliminations(b, removedCounts);
        if (rem <= 0) break;

        for (int i = 0; i < ITEM_TYPES; i++)
            removedTotal[i] += removedCounts[i];

        totalRemoved += rem;
        board_apply_gravity_and_refill(b);
    }
    return totalRemoved;
}
