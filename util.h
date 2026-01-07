#include "util.h"
#include <stdlib.h>
#include <time.h>

void seed_random(void) {
    srand((unsigned)time(NULL));
}

int rand_item(void) {
    return (rand() % ITEM_TYPES) + 1; // 1..ITEM_TYPES
}
