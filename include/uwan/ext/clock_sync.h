/**
 * MIT License
 *
 * Copyright (c) 2024 Alexey Ryabov
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

#ifndef __UWAN_EXT_CLOCK_SYNC_H__
#define __UWAN_EXT_CLOCK_SYNC_H__

#include <uwan/stack.h>

#define UWAN_EXT_CLOCK_SYNC_PORT 202

struct uwan_clock_sync_callbacks {
    void (* set_clock_correction)(int32_t value); // Required
    void (* handle_app_time_periodicity_req)(uint32_t period_sec); // Optional
    uint32_t (* get_unixtime)(void); // Required
    void (* force_device_resync_req)(void); // Required
};

void uwan_clock_sync_init(struct uwan_clock_sync_callbacks *cbs);

void uwan_clock_sync_handle_time_answ(enum uwan_errs err,
    enum uwan_mtypes m_type, const struct uwan_dl_packet *pkt);

enum uwan_errs uwan_clock_sync_send_time_req(bool ans_required);

bool uwan_clock_sync_is_answ_pending(void);

enum uwan_errs uwan_clock_sync_send_answ(void);

#endif
