#include <stdlib.h>
#include "utils.h"

void utils_random_init(uint32_t seed)
{
    srand(seed);
}

uint32_t utils_get_random(uint32_t max)
{
    return rand() % max;
}
