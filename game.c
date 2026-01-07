#define _CRT_SECURE_NO_WARNINGS
#include "game.h"

#include "config.h"
#include "console.h"
#include "board.h"
#include "rules.h"
#include "hud.h"
#include "save.h"

#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <windows.h>

/* ========================================================= */
/* =================== OUTILS CONSOLE ====================== */
/* ========================================================= */

static void console_force_size(int width, int height) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE) return;

    COORD buf;
    buf.X = (SHORT)width;
    buf.Y = (SHORT)height;
    SetConsoleScreenBufferSize(h, buf);

    SMALL_RECT win;
    win.Left = 0;
    win.Top = 0;
    win.Right = (SHORT)(width - 1);
    win.Bottom = (SHORT)(height - 1);
    SetConsoleWindowInfo(h, TRUE, &win);
}

/* ========================================================= */
/* =================== COULEURS ITEMS ====================== */
/* ========================================================= */

#define COL_RESET (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define COL_WHITE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)

static WORD color_for_item(int v) {
    switch (v) {
        case CELL_WALL: return COL_WHITE; // mur
        case 1: return FOREGROUND_RED   | FOREGROUND_INTENSITY; // X
        case 2: return FOREGROUND_GREEN | FOREGROUND_INTENSITY; // O
        case 3: return FOREGROUND_BLUE  | FOREGROUND_INTENSITY; // A
        case 4: return FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_INTENSITY; // $
        case 5: return FOREGROUND_RED   | FOREGROUND_BLUE  | FOREGROUND_INTENSITY; // *
        case CELL_EMPTY: return FOREGROUND_BLUE  | FOREGROUND_GREEN; // .
        default:
            return COL_RESET;
    }
}

static void console_set_color(WORD attr) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE) return;
    SetConsoleTextAttribute(h, attr);
}

/* ========================================================= */
/* =================== OUTILS AFFICHAGE ==================== */
/* ========================================================= */

static char item_char(int v) {
    static const char map[ITEM_TYPES + 1] = { ' ', 'X', 'O', 'A', '$', '*' };
    if (v == CELL_WALL) return '#';
    if (v >= 1 && v <= ITEM_TYPES) return map[v];
    if (v == CELL_EMPTY) return '.';
    return '?';
}

static void afficher_hud(Console* con, const HUD* hud) {
    char ligne[256];
    snprintf(ligne, sizeof(ligne),
             "Niveau %d/%d | Vies : %d | Coups : %d | Temps : %3d s",
             hud->level, MAX_LEVELS, hud->lives, hud->movesLeft, hud->secondsLeft);

    char padded[320];
    snprintf(padded, sizeof(padded), "%-200s", ligne);
    console_set_color(COL_RESET);
    console_write_at(con, 0, 0, padded);
}

static void dessiner_ecran(Console* con,
                           const Board* b,
                           const HUD* hud,
                           int curR, int curC,
                           int selOn, int selR, int selC,
                           const char* msg)
{
    console_clear(con);

    afficher_hud(con, hud);

    int ligne = 2;
    console_set_color(COL_RESET);
    console_write_at(con, ligne++, 0, "=== ECE HEROES ===");
    console_write_at(con, ligne++, 0, "ZQSD: deplacer | ESPACE: selection | P: sauvegarder/quitter");
    ligne++;

    char contrat[512] = "Contrat restant : ";
    for (int t = 1; t <= ITEM_TYPES; t++) {
        int restant = hud->target[t - 1] - hud->achieved[t - 1];
        if (restant < 0) restant = 0;

        char tmp[32];
        snprintf(tmp, sizeof(tmp), "%c:%d  ", item_char(t), restant);
        strcat(contrat, tmp);
    }
    console_write_at(con, ligne++, 0, contrat);
    ligne++;

    char cols[2048];
    snprintf(cols, sizeof(cols), "  |");
    for (int c = 0; c < BOARD_COLS; c++) {
        char tmp[8];
        snprintf(tmp, sizeof(tmp), "%3d", c);
        strcat(cols, tmp);
    }
    console_write_at(con, ligne++, 0, cols);

    for (int r = 0; r < BOARD_ROWS; r++) {
        char left[16];
        snprintf(left, sizeof(left), "%2d |", r);
        console_set_color(COL_RESET);
        console_write_at(con, ligne, 0, left);

        int x = (int)strlen(left);

        for (int c = 0; c < BOARD_COLS; c++) {
            int v = b->cells[r][c];
            char sym = item_char(v);

            int estSel = selOn && (r == selR && c == selC);
            int estCur = (r == curR && c == curC);

            char L = ' ', R = ' ';
            if (estSel || estCur) { L = '['; R = ']'; }

            WORD colSym = color_for_item(v);
            WORD colCadre = COL_RESET;

            if (estSel) colCadre = colSym;
            else if (estCur) colCadre = COL_WHITE;

            console_set_color((L == ' ') ? COL_RESET : colCadre);
            console_write_at(con, ligne, x + 0, (char[2]){ L, '\0' });

            console_set_color(colSym);
            console_write_at(con, ligne, x + 1, (char[2]){ sym, '\0' });

            console_set_color((R == ' ') ? COL_RESET : colCadre);
            console_write_at(con, ligne, x + 2, (char[2]){ R, '\0' });

            x += 3;
        }

        console_set_color(COL_RESET);
        ligne++;
    }

    if (msg && msg[0]) {
        ligne++;
        console_set_color(COL_RESET);
        console_write_at(con, ligne, 0, msg);
    }

    console_set_color(COL_RESET);
}

/* ========================================================= */
/* =================== LOGIQUE JEU ========================== */
/* ========================================================= */

static int sont_adjacentes(int r1, int c1, int r2, int c2) {
    int dr = r1 - r2; if (dr < 0) dr = -dr;
    int dc = c1 - c2; if (dc < 0) dc = -dc;
    return (dr + dc) == 1;
}

static int demander_pseudo(char pseudo[64], const char* prompt) {
    pseudo[0] = '\0';
    printf("%s", prompt);
    fflush(stdout);

    if (!fgets(pseudo, 64, stdin)) return 0;
    size_t n = strlen(pseudo);
    if (n && pseudo[n - 1] == '\n') pseudo[n - 1] = '\0';
    if (!pseudo[0]) return 0;
    return 1;
}

/* Reprendre un niveau depuis un état sauvegardé (TEXTE) */
static int jouer_niveau_depuis_etat(Console* con, SaveGameState* st) {
    Board b = st->board;
    HUD hud = st->hud;

    rules_set_level(hud.level);

    int curR = st->curR, curC = st->curC;
    int selOn = st->selOn, selR = st->selR, selC = st->selC;

    char msg[128] = "Partie reprise.";
    int needRedraw = 1;

    DWORD lastTick = GetTickCount();

    while (1) {
        DWORD now = GetTickCount();
        if (now - lastTick >= 1000) {
            lastTick += 1000;
            hud.secondsLeft--;
            afficher_hud(con, &hud);

            if (hud.secondsLeft <= 0) {
                snprintf(msg, sizeof(msg), "Temps ecoule ! Niveau perdu.");
                dessiner_ecran(con, &b, &hud, curR, curC, selOn, selR, selC, msg);
                Sleep(800);
                st->lives--;
                return 0;
            }
        }

        if (needRedraw) {
            dessiner_ecran(con, &b, &hud, curR, curC, selOn, selR, selC, msg);
            needRedraw = 0;
        }

        if (!_kbhit()) {
            Sleep(10);
            continue;
        }

        int ch = _getch();

        if (ch == 'p' || ch == 'P') {
            console_clear(con);
            char pseudo[64];
            if (demander_pseudo(pseudo, "Pseudo (meme pseudo pour reprendre) : ")) {
                SaveGameState out;
                out.level = hud.level;
                out.lives = st->lives;

                out.board = b;
                out.hud = hud;

                out.curR = curR; out.curC = curC;
                out.selOn = selOn; out.selR = selR; out.selC = selC;

                save_game_state_txt(pseudo, &out);
            }
            return -1;
        }

        if (ch == ' ') {
            if (!selOn) {
                selOn = 1;
                selR = curR;
                selC = curC;
                snprintf(msg, sizeof(msg), "Selection : (%d,%d)", selR, selC);
            } else {
                selOn = 0;
                selR = selC = -1;
                snprintf(msg, sizeof(msg), "Selection annulee.");
            }
            needRedraw = 1;
            continue;
        }

        int dr = 0, dc = 0;
        if (ch == 'z' || ch == 'Z') dr = -1;
        else if (ch == 's' || ch == 'S') dr = +1;
        else if (ch == 'q' || ch == 'Q') dc = -1;
        else if (ch == 'd' || ch == 'D') dc = +1;
        else continue;

        if (!selOn) {
            curR += dr;
            curC += dc;
            if (curR < 0) curR = 0;
            if (curC < 0) curC = 0;
            if (curR >= BOARD_ROWS) curR = BOARD_ROWS - 1;
            if (curC >= BOARD_COLS) curC = BOARD_COLS - 1;
            needRedraw = 1;
        } else {
            int r2 = selR + dr;
            int c2 = selC + dc;

            if (!board_in_bounds(r2, c2)) {
                snprintf(msg, sizeof(msg), "Hors limites.");
                needRedraw = 1;
                continue;
            }

            if (!sont_adjacentes(selR, selC, r2, c2)) {
                snprintf(msg, sizeof(msg), "Cases non adjacentes.");
                needRedraw = 1;
                continue;
            }

            // Empêche swap avec un mur
            if (b.cells[selR][selC] == CELL_WALL || b.cells[r2][c2] == CELL_WALL) {
                snprintf(msg, sizeof(msg), "Impossible: mur.");
                needRedraw = 1;
                continue;
            }

            if (!rules_swap_creates_elimination(&b, selR, selC, r2, c2)) {
                snprintf(msg, sizeof(msg), "Echange refuse (aucune elimination).");
                needRedraw = 1;
                continue;
            }

            board_swap(&b, selR, selC, r2, c2);
            hud.movesLeft--;

            int removed[ITEM_TYPES] = {0};
            rules_apply_cascade(&b, removed);
            hud_add_removed(&hud, removed);

            selOn = 0;
            selR = selC = -1;
            snprintf(msg, sizeof(msg), "Echange OK.");
            needRedraw = 1;

            if (hud_contract_done(&hud)) {
                snprintf(msg, sizeof(msg), "Niveau REUSSI !");
                dessiner_ecran(con, &b, &hud, curR, curC, selOn, selR, selC, msg);
                Sleep(900);

                st->level = hud.level + 1;
                st->hud = hud;
                st->board = b;
                st->curR = curR; st->curC = curC;
                st->selOn = 0; st->selR = -1; st->selC = -1;

                return 1;
            }

            if (hud.movesLeft <= 0) {
                snprintf(msg, sizeof(msg), "Plus de coups ! Niveau perdu.");
                dessiner_ecran(con, &b, &hud, curR, curC, selOn, selR, selC, msg);
                Sleep(900);
                st->lives--;
                return 0;
            }
        }
    }
}

static int jouer_niveau(Console* con, int niveau, int* ioVies) {
    Board b;
    board_init_level(&b, niveau);

    HUD hud;
    hud_init_level(&hud, niveau, *ioVies);

    rules_set_level(niveau);

    int curR = 0, curC = 0;
    int selOn = 0, selR = -1, selC = -1;

    char msg[128] = "Pret.";
    int needRedraw = 1;

    DWORD lastTick = GetTickCount();

    while (1) {
        DWORD now = GetTickCount();
        if (now - lastTick >= 1000) {
            lastTick += 1000;
            hud.secondsLeft--;
            afficher_hud(con, &hud);

            if (hud.secondsLeft <= 0) {
                snprintf(msg, sizeof(msg), "Temps ecoule ! Niveau perdu.");
                dessiner_ecran(con, &b, &hud, curR, curC, selOn, selR, selC, msg);
                Sleep(800);
                (*ioVies)--;
                return 0;
            }
        }

        if (needRedraw) {
            dessiner_ecran(con, &b, &hud, curR, curC, selOn, selR, selC, msg);
            needRedraw = 0;
        }

        if (!_kbhit()) {
            Sleep(10);
            continue;
        }

        int ch = _getch();

        if (ch == 'p' || ch == 'P') {
            console_clear(con);

            char pseudo[64];
            if (demander_pseudo(pseudo, "Pseudo (meme pseudo pour reprendre) : ")) {
                SaveGameState out;
                out.level = niveau;
                out.lives = *ioVies;

                out.board = b;
                out.hud = hud;

                out.curR = curR; out.curC = curC;
                out.selOn = selOn; out.selR = selR; out.selC = selC;

                save_game_state_txt(pseudo, &out);
            }
            return -1;
        }

        if (ch == ' ') {
            if (!selOn) {
                selOn = 1;
                selR = curR;
                selC = curC;
                snprintf(msg, sizeof(msg), "Selection : (%d,%d)", selR, selC);
            } else {
                selOn = 0;
                selR = selC = -1;
                snprintf(msg, sizeof(msg), "Selection annulee.");
            }
            needRedraw = 1;
            continue;
        }

        int dr = 0, dc = 0;
        if (ch == 'z' || ch == 'Z') dr = -1;
        else if (ch == 's' || ch == 'S') dr = +1;
        else if (ch == 'q' || ch == 'Q') dc = -1;
        else if (ch == 'd' || ch == 'D') dc = +1;
        else continue;

        if (!selOn) {
            curR += dr;
            curC += dc;
            if (curR < 0) curR = 0;
            if (curC < 0) curC = 0;
            if (curR >= BOARD_ROWS) curR = BOARD_ROWS - 1;
            if (curC >= BOARD_COLS) curC = BOARD_COLS - 1;
            needRedraw = 1;
        } else {
            int r2 = selR + dr;
            int c2 = selC + dc;

            if (!board_in_bounds(r2, c2)) {
                snprintf(msg, sizeof(msg), "Hors limites.");
                needRedraw = 1;
                continue;
            }

            if (!sont_adjacentes(selR, selC, r2, c2)) {
                snprintf(msg, sizeof(msg), "Cases non adjacentes.");
                needRedraw = 1;
                continue;
            }

            // Empêche swap avec un mur
            if (b.cells[selR][selC] == CELL_WALL || b.cells[r2][c2] == CELL_WALL) {
                snprintf(msg, sizeof(msg), "Impossible: mur.");
                needRedraw = 1;
                continue;
            }

            if (!rules_swap_creates_elimination(&b, selR, selC, r2, c2)) {
                snprintf(msg, sizeof(msg), "Echange refuse (aucune elimination).");
                needRedraw = 1;
                continue;
            }

            board_swap(&b, selR, selC, r2, c2);
            hud.movesLeft--;

            int removed[ITEM_TYPES] = {0};
            rules_apply_cascade(&b, removed);
            hud_add_removed(&hud, removed);

            selOn = 0;
            selR = selC = -1;
            snprintf(msg, sizeof(msg), "Echange OK.");
            needRedraw = 1;

            if (hud_contract_done(&hud)) {
                snprintf(msg, sizeof(msg), "Niveau REUSSI !");
                dessiner_ecran(con, &b, &hud, curR, curC, selOn, selR, selC, msg);
                Sleep(900);
                return 1;
            }

            if (hud.movesLeft <= 0) {
                snprintf(msg, sizeof(msg), "Plus de coups ! Niveau perdu.");
                dessiner_ecran(con, &b, &hud, curR, curC, selOn, selR, selC, msg);
                Sleep(900);
                (*ioVies)--;
                return 0;
            }
        }
    }
}

/* ========================================================= */
/* =================== MENU ================================ */
/* ========================================================= */

static void afficher_menu(Console* con) {
    console_clear(con);
    printf("=== ECE HEROES ===\n");
    printf("1) Lire les regles\n");
    printf("2) Nouvelle partie\n");
    printf("3) Reprendre une partie\n");
    printf("4) Quitter\n");
    printf("Choix : ");
    fflush(stdout);
}

static void afficher_regles(Console* con) {
    console_clear(con);
    printf("Regles du jeu :\n");
    printf("- Suite (>=3) : suppression\n");
    printf("- Suite (>=6) : supprime tout le type\n");
    printf("- Croix : supprime ligne + colonne\n");
    printf("- Niveau 2 : murs (#) qui bloquent la chute\n");
    printf("- Niveau 3 : L de 5 -> explosion 3x3\n");
    printf("- Seuls les echanges qui creent une elimination sont autorises\n");
    printf("\nAppuyez sur une touche...\n");
    _getch();
}

void game_menu_loop(void) {
    Console con;
    console_init(&con);

    console_force_size(140, 45);
    console_hide_cursor(&con, 1);

    while (1) {
        afficher_menu(&con);

        char buf[16];
        if (!fgets(buf, sizeof(buf), stdin)) break;

        int choix = 0;
        sscanf(buf, "%d", &choix);

        if (choix == 1) {
            afficher_regles(&con);

        } else if (choix == 2) {
            int vies = START_LIVES;
            int niveau = 1;

            while (vies > 0 && niveau <= MAX_LEVELS) {
                int r = jouer_niveau(&con, niveau, &vies);
                if (r == -1) break;
                if (r == 1) niveau++;
            }

            console_clear(&con);
            if (vies <= 0) printf("GAME OVER.\n");
            else if (niveau > MAX_LEVELS) printf("VICTOIRE ! Tous les niveaux sont termines.\n");
            else printf("Retour au menu.\n");
            printf("Appuyez sur une touche...\n");
            _getch();

        } else if (choix == 3) {
            console_clear(&con);

            char pseudo[64];
            if (!demander_pseudo(pseudo, "Pseudo : ")) continue;

            SaveGameState st;
            if (!load_game_state_txt(pseudo, &st)) {
                printf("Aucune sauvegarde de partie trouvee pour ce pseudo.\n");
                printf("Appuyez sur une touche...\n");
                _getch();
                continue;
            }

            int vies = st.lives;
            int niveau = st.level;

            int r = jouer_niveau_depuis_etat(&con, &st);
            vies = st.lives;

            if (r == -1) {
                continue;
            }

            if (r == 1) {
                niveau = st.level;
                while (vies > 0 && niveau <= MAX_LEVELS) {
                    int rr = jouer_niveau(&con, niveau, &vies);
                    if (rr == -1) break;
                    if (rr == 1) niveau++;
                }
            }

            console_clear(&con);
            if (vies <= 0) printf("GAME OVER.\n");
            else if (niveau > MAX_LEVELS) printf("VICTOIRE ! Tous les niveaux sont termines.\n");
            else printf("Retour au menu.\n");
            printf("Appuyez sur une touche...\n");
            _getch();

        } else if (choix == 4) {
            console_clear(&con);
            printf("Au revoir.\n");
            break;

        } else {
            console_clear(&con);
            printf("Choix invalide.\n");
            Sleep(700);
        }
    }

    console_hide_cursor(&con, 0);
    console_set_color(COL_RESET);
}
