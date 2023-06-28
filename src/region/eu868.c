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

#define CFLIST_CHANNELS 5
#define CFLIST_CH_FIRST 3
#define CFLIST_CH_SIZE 3 // bytes
#define CFLIST_FREQ_STEP 100 // Hz

static void eu868_init(void);
static void eu868_handle_cflist(const uint8_t *cflist);

const struct uwan_region region_eu868 = {
    .init = eu868_init,
    .handle_cflist = eu868_handle_cflist,
};

void eu868_init()
{
    uwan_set_channel(0, 868100000);
    uwan_set_channel(1, 868300000);
    uwan_set_channel(2, 868500000);
    uwan_set_rx2(868100000, UWAN_DR_0);
}

static void eu868_handle_cflist(const uint8_t *cflist)
{
    uint8_t cflist_type = cflist[LORAWAN_CFLIST_SIZE - 1];

    if (cflist_type != 0)
        return;

    for (int idx = 0; idx < CFLIST_CHANNELS; idx++, cflist += CFLIST_CH_SIZE) {
        uint32_t freq = cflist[0] | cflist[1] << 8 | cflist[2] << 16;
        if (freq == 0)
            uwan_enable_channel(idx + CFLIST_CH_FIRST, false);
        else
            uwan_set_channel(idx + CFLIST_CH_FIRST, freq * CFLIST_FREQ_STEP);
    }
}
