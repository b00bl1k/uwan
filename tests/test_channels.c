#include <assert.h>
#include <stdlib.h>
#include "channels.h"
#include "utils.h"

uint32_t random_val;

uint32_t utils_get_random(uint32_t max)
{
    return random_val;
}

int main()
{
    const struct node_channel *ch;
    enum uwan_errs result;

    channels_init();

    random_val = 5;
    ch = channels_get_next();
    assert(ch == NULL);

    result = uwan_set_channel(3, 869100000, UWAN_DR_0, UWAN_DR_5);
    assert(result == UWAN_ERR_NO);

    result = uwan_set_channel(7, 868800000, UWAN_DR_0, UWAN_DR_5);
    assert(result == UWAN_ERR_NO);

    random_val = 5;
    ch = channels_get_next();
    assert(ch->frequency == 868800000);

    random_val = 8;
    ch = channels_get_next();
    assert(ch->frequency == 869100000);

    result = uwan_set_channel(16, 868800000, UWAN_DR_0, UWAN_DR_5);
    assert(result == UWAN_ERR_CHANNEL);

    return 0;
}
