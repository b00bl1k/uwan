/**
 * MIT License
 *
 * Copyright (c) 2021 Alexey Ryabov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include <uwan/stack.h>
#include "channels.h"
#include "utils.h"

#define MAX_CHANNELS 16

static uint8_t uw_channels_max_count;
static uint8_t uw_channels_mask[BYTES_FOR_BITS(MAX_CHANNELS)];
static uint32_t uw_channels[MAX_CHANNELS];

void channels_init()
{
    memset(uw_channels_mask, 0, sizeof(uw_channels_mask));
    uw_channels_max_count = 0;
}

uint32_t channels_get_next()
{
    uint8_t ch;
    uint8_t start_ch;

    if (uw_channels_max_count == 0)
        return 0;

    ch = start_ch = utils_get_random(uw_channels_max_count);

    do {
        if (BIT_IS_SET(uw_channels_mask, ch))
            return uw_channels[ch];
        ch = (ch + 1) % uw_channels_max_count;
    } while (start_ch != ch);

    return 0;
}

enum uwan_errs uwan_enable_channel(uint8_t index, bool enable)
{
    if (index >= MAX_CHANNELS)
        return UWAN_ERR_CHANNEL;

    if (enable) {
        uw_channels_max_count = MAX(uw_channels_max_count, index + 1);
        BIT_SET(uw_channels_mask, index);
    }
    else {
        BIT_CLEAR(uw_channels_mask, index);
        if (uw_channels_max_count == index + 1) {
            for (uint8_t i = 0; i < index; i++) {
                if (BIT_IS_SET(uw_channels_mask, i))
                    uw_channels_max_count = i + 1;
            }
        }
    }

    return UWAN_ERR_NO;
}

enum uwan_errs uwan_set_channel(uint8_t index, uint32_t frequency)
{
    if (index >= MAX_CHANNELS)
        return UWAN_ERR_CHANNEL;

    uw_channels[index] = frequency;
    uwan_enable_channel(index, true);

    return UWAN_ERR_NO;
}
