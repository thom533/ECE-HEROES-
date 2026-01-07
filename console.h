#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdbool.h>

typedef struct {
    void* hOut;      // HANDLE (Windows) stocké en void* pour éviter windows.h ici
    bool vt;         // Virtual Terminal / ANSI actif
} Console;

void console_init(Console* c);
void console_clear(Console* c);
void console_hide_cursor(Console* c, int hide);

// Nouveau : écrire à un endroit précis
void console_goto(Console* c, int row, int col);
void console_write_at(Console* c, int row, int col, const char* text);

#endif
