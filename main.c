#define _CRT_SECURE_NO_WARNINGS
#include "game.h"
#include <stdlib.h>
#include <time.h>

int main(void) {
    srand((unsigned)time(NULL));
    game_menu_loop();
    return 0;
}
