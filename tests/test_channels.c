#include <assert.h>
#include <stdlib.h>

#include <uwan/stack.h>
#include "channels.h"
#include "utils.h"

uint32_t random_val;

void utils_random_init(uint32_t seed)
{
}

uint32_t utils_get_random(uint32_t max)
{
    return random_val;
}

uint8_t utils_checksum(const void *src, size_t size)
{
    return 0xff;
}

uint32_t utils_gps_to_unix(uint32_t timestamp)
{
    return timestamp;
}

int main()
{
    uint32_t ch;
    enum uwan_errs result;

    channels_init();

    random_val = 5;
    ch = channels_get_next();
    assert(ch == 0);

    result = uwan_set_channel(3, 869100000);
    assert(result == UWAN_ERR_NO);

    result = uwan_set_channel(7, 868800000);
    assert(result == UWAN_ERR_NO);

    random_val = 5;
    ch = channels_get_next();
    assert(ch == 868800000);

    random_val = 8;
    ch = channels_get_next();
    assert(ch == 869100000);

    result = uwan_set_channel(16, 868800000);
    assert(result == UWAN_ERR_CHANNEL);

    return 0;
}
