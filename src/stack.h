/**
 * MIT License
 *
 * Copyright (c) 2023-2024 Alexey Ryabov
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

#ifndef __STACK_H__
#define __STACK_H__

#include <uwan/stack.h>

#define NB_TRANS_MIN 1
#define NB_TRANS_MAX 15
#define TX_POWER_MAX 15

#define NODE_SESSION_VERSION 1

struct node_session {
    uint8_t version;
    uint16_t size;
    uint8_t is_joined;
    uint8_t ack_required;
    uint8_t dr;
    uint32_t dev_addr;
    uint32_t f_cnt_up;
    uint32_t f_cnt_down;
    uint8_t nwk_s_key[UWAN_NWK_S_KEY_SIZE];
    uint8_t app_s_key[UWAN_APP_S_KEY_SIZE];
};

extern const struct uwan_region *uw_region;
extern struct node_session uw_session;

bool is_valid_dr(uint8_t dr);
bool is_valid_frequency(uint32_t freq);
bool set_nb_trans(uint8_t nb_trans);
void reset_nb_trans(void);
bool is_valid_tx_power(uint8_t tx_power);
bool set_tx_power(uint8_t tx_power);

#endif
