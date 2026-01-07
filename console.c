#define WIN32_LEAN_AND_MEAN
#include "console.h"
#include <windows.h>
#include <stdio.h>

static void win_clear(HANDLE h) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD count;
    DWORD cellCount;
    COORD home = {0, 0};

    if (!GetConsoleScreenBufferInfo(h, &csbi)) return;
    cellCount = (DWORD)csbi.dwSize.X * (DWORD)csbi.dwSize.Y;

    FillConsoleOutputCharacterA(h, ' ', cellCount, home, &count);
    FillConsoleOutputAttribute(h, csbi.wAttributes, cellCount, home, &count);
    SetConsoleCursorPosition(h, home);
}

void console_init(Console* c) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    c->hOut = h;
    c->vt = false;

    // On tente d'activer l'ANSI (VT)
    DWORD mode = 0;
    if (GetConsoleMode(h, &mode)) {
        DWORD newMode = mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (SetConsoleMode(h, newMode)) {
            c->vt = true;
        }
    }
}

void console_hide_cursor(Console* c, int hide) {
    HANDLE h = (HANDLE)c->hOut;
    CONSOLE_CURSOR_INFO info;
    if (!GetConsoleCursorInfo(h, &info)) return;
    info.bVisible = hide ? FALSE : TRUE;
    SetConsoleCursorInfo(h, &info);
}

void console_clear(Console* c) {
    HANDLE h = (HANDLE)c->hOut;
    if (c->vt) {
        printf("\x1b[2J\x1b[H");
        fflush(stdout);
    } else {
        win_clear(h);
    }
}

void console_goto(Console* c, int row, int col) {
    HANDLE h = (HANDLE)c->hOut;
    if (c->vt) {
        printf("\x1b[%d;%dH", row + 1, col + 1); // ANSI is 1-based
        fflush(stdout);
    } else {
        COORD p;
        p.X = (SHORT)col;
        p.Y = (SHORT)row;
        SetConsoleCursorPosition(h, p);
    }
}

void console_write_at(Console* c, int row, int col, const char* text) {
    console_goto(c, row, col);
    fputs(text, stdout);
    fflush(stdout);
}
