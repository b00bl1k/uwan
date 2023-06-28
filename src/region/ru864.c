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

#include <uwan/region/ru864.h>
#include "common.h"

#define CFLIST_CH_FIRST 2

static void ru864_init(void);
static void ru864_handle_cflist(const uint8_t *cflist);

const struct uwan_region region_ru864 = {
    .init = ru864_init,
    .handle_cflist = ru864_handle_cflist,
    .handle_adr_ch_mask = region_86x_handle_adr_ch_mask,
};

static void ru864_init()
{
    uwan_set_channel(0, 868900000);
    uwan_set_channel(1, 869100000);
    uwan_set_rx2(869100000, UWAN_DR_0);
}

static void ru864_handle_cflist(const uint8_t *cflist)
{
    region_86x_handle_cflist(cflist, CFLIST_CH_FIRST);
}
