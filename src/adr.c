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
#include "stack.h"

/* Fields of LinkADRReq command */
#define DRTX_DR_MASK 0xf
#define DRTX_DR_SHIFT 4
#define DRTX_TX_POWER_MASK 0xf
#define DRTX_TX_POWER_SHIFT 0
#define REDUNDANCY_CH_MASK_CNTL_MASK 0x7
#define REDUNDANCY_CH_MASK_CNTL_SHIFT 4
#define REDUNDANCY_NB_TRANS_MASK 0xf
#define REDUNDANCY_NB_TRANS_SHIFT 0

/* Status byte of LinkADRAns command */
#define STATUS_CH_MASK_ACK 0x1
#define STATUS_DR_ACK      0x2
#define STATUS_POWER_ACK   0x4
#define STATUS_OK          0x7

#define ADR_ACK_LIMIT 64
#define ADR_ACK_DELAY 32

static uint32_t ack_cnt;
static uint8_t ack_limit = ADR_ACK_LIMIT;
static uint8_t ack_delay = ADR_ACK_DELAY;
static bool adr_is_enabled;

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
    if (adr_is_enabled == false || uw_session.dr == UWAN_DR_0)
        return false;

    return ack_cnt >= ack_limit;
}

bool adr_handle_link_req(uint8_t dr_txpow,uint16_t ch_mask, uint8_t redundancy)
{
    uint8_t result = 0;

    uint8_t dr = (dr_txpow >> DRTX_DR_SHIFT) & DRTX_DR_MASK;
    if (is_valid_dr(dr))
        result |= STATUS_DR_ACK;

    uint8_t ch_mask_cntl = (redundancy >> REDUNDANCY_CH_MASK_CNTL_SHIFT) &
        REDUNDANCY_CH_MASK_CNTL_SHIFT;
    if (uw_region->handle_adr_ch_mask(ch_mask, ch_mask_cntl, true))
        result |= STATUS_CH_MASK_ACK;

    uint8_t tx_power = dr_txpow & DRTX_TX_POWER_MASK;
    if (is_valid_tx_power(tx_power)) {
        result |= STATUS_POWER_ACK;
    }

    if (result == STATUS_OK) {
        uint8_t nb_trans = redundancy & REDUNDANCY_NB_TRANS_MASK;
        if (set_nb_trans(nb_trans) == false)
            reset_nb_trans();

        set_tx_power(tx_power);
        uw_session.dr = (enum uwan_dr)dr;
        uw_region->handle_adr_ch_mask(ch_mask, ch_mask_cntl, false);
    }

    return mac_enqueue(CID_LINK_ADR, &result, sizeof(result));
}

void adr_handle_uplink()
{
    ack_cnt++;

    if (uw_session.dr != UWAN_DR_0 && ack_cnt > (ack_limit + ack_delay)) {
        uw_session.dr--;
        ack_cnt = 0;
    }
}

void adr_handle_downlink()
{
    ack_cnt = 0;
}
