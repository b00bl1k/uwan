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

#include "adr.h"
#include "mac.h"

#define LINK_ADR_REQ_DR_MASK 0xf
#define LINK_ADR_REQ_DR_SHIFT 4

#define LINK_ADR_ANS_STATUS_CH_MASK_ACK 0x1
#define LINK_ADR_ANS_STATUS_DR_ACK      0x2
#define LINK_ADR_ANS_STATUS_POWER_ACK   0x4
#define LINK_ADR_ANS_STATUS_OK          0x7

static uint32_t ack_cnt;
static uint8_t ack_limit = 32;
static uint8_t ack_delay = 16;
static bool adr_is_enabled = false;
static enum uwan_dr curr_dr = UWAN_DR_0;

bool uwan_adr_is_enabled()
{
    return adr_is_enabled;
}

void uwan_adr_enable(bool enable)
{
    adr_is_enabled = enable;
    ack_cnt = 0;
}

void uwan_adr_setup_ack(uint8_t limit, uint8_t delay)
{
    ack_limit = limit;
    ack_delay = delay;
}

bool adr_get_req_bit()
{
    if (adr_is_enabled == false || curr_dr == UWAN_DR_0)
        return false;

    return ack_cnt >= ack_limit;
}

enum uwan_dr adr_get_dr()
{
    return curr_dr;
}

bool adr_handle_link_req(uint8_t dr_txpow, uint16_t ch_mask, uint8_t redundancy)
{
    uint8_t result = 0;

    uint8_t dr = (dr_txpow >> LINK_ADR_REQ_DR_SHIFT) & LINK_ADR_REQ_DR_MASK;
    if (dr < UWAN_DR_COUNT)
        result |= LINK_ADR_ANS_STATUS_DR_ACK;

    // TODO chMask, txPower, redundancy
    result |= LINK_ADR_ANS_STATUS_CH_MASK_ACK;
    result |= LINK_ADR_ANS_STATUS_POWER_ACK;

    if (result == LINK_ADR_ANS_STATUS_OK) {
        // command succeed, change the state
        curr_dr = (enum uwan_dr)dr;
    }

    return mac_enqueue_ans(CID_LINK_ADR, &result, sizeof(result));
}

void adr_handle_uplink()
{
    ack_cnt++;

    if (curr_dr != UWAN_DR_0 && ack_cnt > (ack_limit + ack_delay)) {
        curr_dr--;
        ack_cnt = 0;
    }
}

void adr_handle_downlink()
{
    ack_cnt = 0;
}
