/**
 * MIT License
 *
 * Copyright (c) 2023 Alexey Ryabov
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

#include <uwan/stack.h>
#include "../channels.h"
#include "common.h"

#define CFLIST_CHANNELS 5
#define CFLIST_CH_SIZE 3 // bytes
#define CFLIST_FREQ_STEP 100 // Hz

void region_86x_handle_cflist(const uint8_t *cflist, uint8_t ch_first)
{
    uint8_t cflist_type = cflist[LORAWAN_CFLIST_SIZE - 1];

    if (cflist_type != 0)
        return;

    for (int idx = 0; idx < CFLIST_CHANNELS; idx++, cflist += CFLIST_CH_SIZE) {
        uint32_t freq = cflist[0] | cflist[1] << 8 | cflist[2] << 16;
        if (freq == 0)
            uwan_enable_channel(idx + ch_first, false);
        else
            uwan_set_channel(idx + ch_first, freq * CFLIST_FREQ_STEP);
    }
}

bool region_86x_handle_adr_ch_mask(uint16_t ch_mask, uint8_t ch_mask_cntl,
    bool dry_run)
{
    switch (ch_mask_cntl) {
    case 0:
        for (int i = 0; i < 16; i++) {
            bool enable = (ch_mask & (1 << i)) != 0;
            if (dry_run) {
                // validate mask
                if (enable && (channel_is_exist(i) == false))
                    return false;
            }
            else {
                // apply mask
                uwan_enable_channel(i, enable);
            }
        }
        return true;

    case 6:
        if (dry_run == false)
            channels_enable_all();
        return true;

    default:
        return false;
    }
}
