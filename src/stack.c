/**
 * MIT License
 *
 * Copyright (c) 2021-2025 Alexey Ryabov
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

#include <string.h>

#include <uwan/stack.h>
#include "adr.h"
#include "channels.h"
#include "mac.h"
#include "stack.h"
#include "utils.h"

#define MAJOR_MASK 0x3
#define MAJOR_OFFSET 0
#define MAJOR_LORAWAN_R1 0x0

#define MTYPE_MASK 0x7
#define MTYPE_OFFSET 5

#define FCTRL_ADR 0x80
#define FCTRL_UPLINK_ADR_ACK_REQ 0x40
#define FCTRL_ACK 0x20
#define FCTRL_DOWNLINK_FPENDING 0x10
#define FCTRL_UPLINK_CLASSB 0x10
#define FCTRL_FOPTS_MASK 0xf

#define RX1_DELAY 1000
#define JOIN_DELAY 5000
#define SECOND_RX_OFFSET 1000

#define B0_DIR_UPLINK 0
#define B0_DIR_DOWNLINK 1
#define MIC_LEN 4
#define FRAME_MAX_SIZE 255
#define RX_SYMB_TIMEOUT 0x08

#define FREQ_MIN 860000000
#define FREQ_MAX 870000000

enum stack_states {
    UWAN_STATE_NOT_INIT,
    UWAN_STATE_IDLE,
    UWAN_STATE_TX,
    UWAN_STATE_RX1,
    UWAN_STATE_RX2,
};

struct node_dr {
    enum uwan_sf sf;
    enum uwan_bw bw;
};

/* DR0..DR5 is enough for EU868 and RU864 */
static const struct node_dr uw_dr_table[UWAN_DR_COUNT] = {
    {UWAN_SF_12, UWAN_BW_125},
    {UWAN_SF_11, UWAN_BW_125},
    {UWAN_SF_10, UWAN_BW_125},
    {UWAN_SF_9, UWAN_BW_125},
    {UWAN_SF_8, UWAN_BW_125},
    {UWAN_SF_7, UWAN_BW_125},
};

static const uint8_t uw_tx_power_table[] = {
    0, 2, 4, 6, 8, 10, 12, 14,
};

static const uint8_t uw_max_app_pld_size[] = {
    51, 51, 51, 115, 222, 222,
};

const struct uwan_region *uw_region;
struct node_session uw_session;

static const struct radio_dev *uw_radio;
static const struct stack_hal *uw_stack_hal;
static struct uwan_packet_params pkt_params;
static uint8_t uw_frame[FRAME_MAX_SIZE];
static enum stack_states uw_state = UWAN_STATE_NOT_INIT;
static uint32_t uw_rx1_delay;
static uint8_t uw_rx1_offset;
static uint32_t uw_rx2_delay;
static bool uw_is_join_state;
static uint32_t uw_dev_nonce;
static uint8_t uw_dev_eui[UWAN_DEV_EUI_SIZE];
static uint8_t uw_app_eui[UWAN_APP_EUI_SIZE];
static uint8_t uw_app_key[UWAN_APP_KEY_SIZE];

static uint32_t default_join_delay = JOIN_DELAY;
static uint32_t default_rx1_delay = RX1_DELAY;
static enum uwan_dr default_dr = UWAN_DR_0;
static uint8_t default_nb_trans = NB_TRANS_MIN;
static uint8_t uw_nb_trans = NB_TRANS_MIN;
static uint8_t default_tx_power = 0;
static uint8_t uw_tx_power = 0;
static int8_t default_max_eirp = 14;
static int8_t current_snr;

/* RX2 window settings */
static uint32_t uw_rx2_frequency;
static enum uwan_dr uw_rx2_dr;

static void apply_tx_power(void)
{
    int power = default_max_eirp - uw_tx_power_table[uw_tx_power];
    uw_radio->set_power(power);
}

static enum uwan_dr get_current_dr(void)
{
    if (uwan_adr_is_enabled())
        return uw_session.dr;

    return default_dr;
}

static uint32_t adjust_rx_delay(uint32_t rx_delay)
{
    uint32_t timeout;
    timeout = uw_radio->get_tcxo_timeout ? uw_radio->get_tcxo_timeout() : 0;
    if (rx_delay > timeout)
        rx_delay -= timeout;
    return rx_delay;
}

static void encrypt_payload(uint8_t *buf, uint8_t size, const uint8_t *key,
    uint8_t dir)
{
    uint8_t a_block[UWAN_AES_BLOCK_SIZE];
    uint8_t s_block[UWAN_AES_BLOCK_SIZE];
    uint8_t a_block_i = 1;
    uint8_t src_pos = 0;
    uint32_t f_cnt = (dir) ? uw_session.f_cnt_down : uw_session.f_cnt_up;

    // prepare Block Ai
    uint8_t pos = 0;
    a_block[pos++] = 0x01;
    a_block[pos++] = 0x00;
    a_block[pos++] = 0x00;
    a_block[pos++] = 0x00;
    a_block[pos++] = 0x00;
    a_block[pos++] = dir;
    a_block[pos++] = uw_session.dev_addr & 0xff;
    a_block[pos++] = (uw_session.dev_addr >> 8) & 0xff;
    a_block[pos++] = (uw_session.dev_addr >> 16) & 0xff;
    a_block[pos++] = (uw_session.dev_addr >> 24) & 0xff;
    a_block[pos++] = f_cnt & 0xff;
    a_block[pos++] = (f_cnt >> 8) & 0xff;
    a_block[pos++] = (f_cnt >> 16) & 0xff;
    a_block[pos++] = (f_cnt >> 24) & 0xff;
    a_block[pos++] = 0x00;

    void *ctx = uw_stack_hal->crypto_aes_create_context(key);

    for (; size > 0; a_block_i++) {
        a_block[pos] = a_block_i;

        uw_stack_hal->crypto_aes_encrypt(ctx, s_block, a_block);

        uint8_t chunk_size = size >= UWAN_AES_BLOCK_SIZE ? UWAN_AES_BLOCK_SIZE : size;
        for (uint8_t i = 0; i < chunk_size; i++, src_pos++) {
            buf[src_pos] = buf[src_pos] ^ s_block[i];
        }
        size -= chunk_size;
    }

    uw_stack_hal->crypto_aes_delete_context(ctx);
}

static void calc_mic(uint8_t *mic, const uint8_t *msg, uint8_t msg_len,
    const uint8_t *key, uint8_t dir, uint32_t f_cnt, bool b0)
{
    uint8_t cmac_mic[UWAN_CMAC_DIGESTLEN];

    void *ctx = uw_stack_hal->crypto_cmac_create_context(key);

    if (b0) {
        uint8_t block_b0[UWAN_AES_BLOCK_SIZE];
        uint8_t pos = 0;

        block_b0[pos++] = 0x49;
        block_b0[pos++] = 0x00;
        block_b0[pos++] = 0x00;
        block_b0[pos++] = 0x00;
        block_b0[pos++] = 0x00;
        block_b0[pos++] = dir;
        block_b0[pos++] = uw_session.dev_addr & 0xff;
        block_b0[pos++] = (uw_session.dev_addr >> 8) & 0xff;
        block_b0[pos++] = (uw_session.dev_addr >> 16) & 0xff;
        block_b0[pos++] = (uw_session.dev_addr >> 24) & 0xff;
        block_b0[pos++] = f_cnt & 0xff;
        block_b0[pos++] = (f_cnt >> 8) & 0xff;
        block_b0[pos++] = (f_cnt >> 16) & 0xff;
        block_b0[pos++] = (f_cnt >> 24) & 0xff;
        block_b0[pos++] = 0x00;
        block_b0[pos++] = msg_len;

        uw_stack_hal->crypto_cmac_update(ctx, block_b0, sizeof(block_b0));
    }

    uw_stack_hal->crypto_cmac_update(ctx, msg, msg_len);
    uw_stack_hal->crypto_cmac_finish(ctx, cmac_mic);
    uw_stack_hal->crypto_cmac_delete_context(ctx);

    memcpy(mic, cmac_mic, MIC_LEN);
}

static void derive_session_key(uint8_t *key, uint8_t key_type,
    uint32_t app_nonce, uint32_t net_id)
{
    uint8_t offset = 0;
    key[offset++] = key_type;
    key[offset++] = app_nonce & 0xff;
    key[offset++] = (app_nonce >> 8) & 0xff;
    key[offset++] = (app_nonce >> 16) & 0xff;
    key[offset++] = net_id & 0xff;
    key[offset++] = (net_id >> 8) & 0xff;
    key[offset++] = (net_id >> 16) & 0xff;
    key[offset++] = uw_dev_nonce & 0xff;
    key[offset++] = (uw_dev_nonce >> 8) & 0xff;
    while (offset < UWAN_AES_BLOCK_SIZE)
        key[offset++] = 0x00;

    void *ctx = uw_stack_hal->crypto_aes_create_context(uw_app_key);
    uw_stack_hal->crypto_aes_encrypt(ctx, key, key);
    uw_stack_hal->crypto_aes_delete_context(ctx);
}

static enum uwan_errs handle_join_msg(struct uwan_dl_packet *pkt)
{
    uint8_t *buf = pkt->data;
    uint8_t mhdr;
    uint8_t mic[MIC_LEN];
    uint8_t offset = 0;
    bool cflist;

    if (pkt->size == (UWAN_AES_BLOCK_SIZE + 1))
        cflist = false;
    else if (pkt->size == (2 * UWAN_AES_BLOCK_SIZE + 1))
        cflist = true;
    else
        return UWAN_ERR_MSG_LEN;

    mhdr = buf[offset++];
    if (mhdr != ((UWAN_MTYPE_JOIN_ACCEPT << MTYPE_OFFSET) |
                 (MAJOR_LORAWAN_R1 << MAJOR_OFFSET)))
        return UWAN_ERR_MSG_MHDR;

    void *ctx = uw_stack_hal->crypto_aes_create_context(uw_app_key);
    uw_stack_hal->crypto_aes_encrypt(ctx, buf + sizeof(mhdr), buf + sizeof(mhdr));
    if (cflist) {
        uw_stack_hal->crypto_aes_encrypt(ctx,
            buf + sizeof(mhdr) + UWAN_AES_BLOCK_SIZE,
            buf + sizeof(mhdr) + UWAN_AES_BLOCK_SIZE);
    }
    uw_stack_hal->crypto_aes_delete_context(ctx);

    calc_mic(mic, buf, pkt->size - sizeof(mic), uw_app_key, 0, 0, false);
    if (memcmp(buf + pkt->size - sizeof(mic), mic, sizeof(mic)))
        return UWAN_ERR_MSG_MIC;

    uint32_t app_nonce, net_id;

    app_nonce = buf[offset++];
    app_nonce |= buf[offset++] << 8;
    app_nonce |= buf[offset++] << 16;

    net_id = buf[offset++];
    net_id |= buf[offset++] << 8;
    net_id |= buf[offset++] << 16;

    uint32_t dev_addr = buf[offset++];
    dev_addr |= buf[offset++] << 8;
    dev_addr |= buf[offset++] << 16;
    dev_addr |= buf[offset++] << 24;

    // TODO duplicated code, see mac.c rx_param_setup function
    uint8_t dl_settings = buf[offset++];
    uint8_t rx1_dr_offset = (dl_settings >> 4) & 7;
    uint8_t rx2_dr = dl_settings & 0xf;
    if (is_valid_dr(rx1_dr_offset))
        uw_rx1_offset = rx1_dr_offset;
    if (is_valid_dr(rx2_dr))
        uw_rx2_dr = (enum uwan_dr)rx2_dr;

    // TODO duplicated code, see mac.c rx_timing_setup function
    uint8_t rx1_delay = buf[offset++] & 0xf;
    if (rx1_delay == 0)
        rx1_delay = 1;
    default_rx1_delay = rx1_delay * 1000;

    if (cflist)
        uw_region->handle_cflist(buf + offset);

    const uint8_t nwk = 0x01;
    const uint8_t app = 0x02;
    uint8_t nwk_s_key[UWAN_NWK_S_KEY_SIZE];
    uint8_t app_s_key[UWAN_APP_S_KEY_SIZE];
    derive_session_key(nwk_s_key, nwk, app_nonce, net_id);
    derive_session_key(app_s_key, app, app_nonce, net_id);
    uwan_set_session(dev_addr, 0, 0, nwk_s_key, app_s_key);

    return UWAN_ERR_NO;
}

static enum uwan_errs handle_data_msg(struct uwan_dl_packet *pkt)
{
    uint8_t mhdr;
    uint32_t dev_addr;
    uint8_t f_ctrl;
    uint16_t f_cnt;
    uint8_t f_opts_len;
    uint8_t mic[MIC_LEN];
    uint8_t offset = 0;
    uint8_t *buf = pkt->data;
    uint8_t msg_min_size = sizeof(mhdr) + sizeof(dev_addr) + sizeof(f_ctrl)
        + sizeof(f_cnt) + sizeof(mic);

    if (pkt->size < msg_min_size)
        return UWAN_ERR_MSG_LEN;

    mhdr = buf[offset++];
    if (((mhdr >> MAJOR_OFFSET) & MAJOR_MASK) != MAJOR_LORAWAN_R1)
        return UWAN_ERR_MSG_MHDR;

    uint8_t mtype = (mhdr >> MTYPE_OFFSET) & MTYPE_MASK;
    if (mtype != UWAN_MTYPE_UNCONF_DATA_DOWN && mtype != UWAN_MTYPE_CONF_DATA_DOWN)
        return UWAN_ERR_MSG_MHDR;

    if (mtype == UWAN_MTYPE_CONF_DATA_DOWN)
        uw_session.ack_required = true;

    dev_addr = buf[offset++];
    dev_addr |= buf[offset++] << 8;
    dev_addr |= buf[offset++] << 16;
    dev_addr |= buf[offset++] << 24;

    if (uw_session.dev_addr != dev_addr)
        return UWAN_ERR_DEV_ADDR;

    f_ctrl = buf[offset++];
    f_opts_len = f_ctrl & FCTRL_FOPTS_MASK;
    if (pkt->size < (msg_min_size + f_opts_len))
        return UWAN_ERR_MSG_LEN;

    f_cnt = buf[offset++];
    f_cnt |= buf[offset++] << 8;

    uint32_t new_f_cnt_down;
    if (uw_session.f_cnt_down == 0) {
        // accept initial value
        new_f_cnt_down = f_cnt;
    }
    else {
        uint16_t f_cnt_prev = (uint16_t)uw_session.f_cnt_down;
        int32_t f_cnt_diff = f_cnt - f_cnt_prev;

        if (f_cnt_diff == 0)
            return UWAN_ERR_FCNT;

        if (f_cnt_diff > 0)
            new_f_cnt_down = uw_session.f_cnt_down + f_cnt_diff;
        else {
            // considering counter rollover
            uint32_t f_cnt_hi = uw_session.f_cnt_down & 0xffff0000;
            new_f_cnt_down = f_cnt_hi + 0x10000 + f_cnt;
        }
    }

    current_snr = pkt->snr;

    uint8_t *fopts_buf = buf + offset;
    offset += f_opts_len;

    uint8_t *pld = NULL;
    uint8_t pld_size = pkt->size - (msg_min_size + f_opts_len);
    if (pld_size > 0) {
        pkt->f_port = buf[offset++];
        pld = buf + offset;
        pld_size--;

        if (f_opts_len && pkt->f_port == 0)
            return UWAN_ERR_MSG_FHDR;
    }

    calc_mic(mic, buf, pkt->size - sizeof(mic), uw_session.nwk_s_key,
        B0_DIR_DOWNLINK, new_f_cnt_down, true);
    if (memcmp(buf + pkt->size - sizeof(mic), mic, sizeof(mic)))
        return UWAN_ERR_MSG_MIC;

    uw_session.f_cnt_down = new_f_cnt_down; // accept new value if mic is ok

    if (pld_size > 0) {
        const uint8_t *key;
        key = (pkt->f_port == 0) ? uw_session.nwk_s_key : uw_session.app_s_key;
        encrypt_payload(pld, pld_size, key, B0_DIR_DOWNLINK);
    }

    if (f_opts_len > 0)
        mac_handle_commands(fopts_buf, f_opts_len);
    else if (pld_size && pkt->f_port == 0)
        mac_handle_commands(pld, pld_size);

    adr_handle_downlink();

    pkt->data = pld;
    pkt->size = pld_size;

    return UWAN_ERR_NO;
}

static void handle_downlink(enum uwan_errs err)
{
    struct uwan_dl_packet pkt = {0};
    enum uwan_mtypes mtype = UWAN_MTYPE_JOIN_REQUEST;

    if (err == UWAN_ERR_NO) {
        pkt.data = uw_frame;
        pkt.size = sizeof(uw_frame);
        uw_radio->read_packet(&pkt);
    }

    uw_radio->sleep();

    if (err == UWAN_ERR_NO) {
        mtype = (enum uwan_mtypes)((uw_frame[0] >> MTYPE_OFFSET) & MTYPE_MASK);
        if (uw_is_join_state)
            err = handle_join_msg(&pkt);
        else
            err = handle_data_msg(&pkt);
    }

    uw_is_join_state = false;
    uw_stack_hal->downlink_callback(err, mtype, &pkt);
}

static void evt_handler(uint8_t evt_mask)
{
    if (uw_state <= UWAN_STATE_IDLE)
        return;

    switch (uw_state) {
    case UWAN_STATE_TX:
        if (evt_mask & RADIO_IRQF_TX_DONE) {
            uw_state = UWAN_STATE_RX1;
            uw_stack_hal->start_timer(UWAN_TIMER_RX1,
                adjust_rx_delay(uw_rx1_delay));
            uw_stack_hal->start_timer(UWAN_TIMER_RX2,
                adjust_rx_delay(uw_rx2_delay));

            if (uw_rx1_offset) {
                enum uwan_dr dr_id = get_current_dr();
                if (uw_rx1_offset > dr_id)
                    dr_id = UWAN_DR_0;
                else
                    dr_id -= uw_rx1_offset;
                const struct node_dr *dr = &uw_dr_table[dr_id];
                pkt_params.sf = dr->sf;
                pkt_params.bw = dr->bw;
            }

            pkt_params.inverted_iq = true;
            uw_radio->setup(&pkt_params);

            // notify MAC that TX completed
            mac_on_tx_complete();
        }
        break;

    case UWAN_STATE_RX1:
        if (evt_mask & RADIO_IRQF_RX_TIMEOUT) {
            // prepare radio for RX2
            uw_state = UWAN_STATE_RX2;
            const struct node_dr *dr = &uw_dr_table[uw_rx2_dr];
            pkt_params.sf = dr->sf;
            pkt_params.bw = dr->bw;
            pkt_params.inverted_iq = true;
            uw_radio->set_frequency(uw_rx2_frequency);
            uw_radio->setup(&pkt_params);
        }
        else if (evt_mask & RADIO_IRQF_RX_DONE) {
            uw_stack_hal->stop_timer(UWAN_TIMER_RX2);
            uw_state = UWAN_STATE_IDLE;
            if (evt_mask & RADIO_IRQF_CRC_ERROR)
                handle_downlink(UWAN_ERR_RX_CRC);
            else
                handle_downlink(UWAN_ERR_NO);
        }
        break;

    case UWAN_STATE_RX2:
        if (evt_mask & RADIO_IRQF_RX_TIMEOUT) {
            uw_state = UWAN_STATE_IDLE;
            handle_downlink(UWAN_ERR_RX_TIMEOUT);
        }
        else if (evt_mask & RADIO_IRQF_RX_DONE) {
            uw_state = UWAN_STATE_IDLE;
            if (evt_mask & RADIO_IRQF_CRC_ERROR)
                handle_downlink(UWAN_ERR_RX_CRC);
            else
                handle_downlink(UWAN_ERR_NO);
        }
        break;

    default:
        break;
    }
}

void uwan_init(const struct radio_dev *radio, const struct stack_hal *stack,
    const struct uwan_region *region)
{
    uw_state = UWAN_STATE_IDLE;
    uw_radio = radio;
    uw_stack_hal = stack;
    uw_region = region;

    memset(&uw_session, 0, sizeof(uw_session));

    radio->set_evt_handler(evt_handler);

    mac_init();
    channels_init();
    uw_region->init();
    utils_random_init(radio->rand());

    pkt_params.cr = UWAN_CR_4_5;
    pkt_params.preamble_len = 8;
    pkt_params.crc_on = true;
    pkt_params.implicit_header = false;
}

void uwan_set_otaa_keys(const uint8_t *dev_eui, const uint8_t *app_eui,
    const uint8_t *app_key)
{
    memcpy(uw_dev_eui, dev_eui, UWAN_DEV_EUI_SIZE);
    memcpy(uw_app_eui, app_eui, UWAN_APP_EUI_SIZE);
    memcpy(uw_app_key, app_key, UWAN_APP_KEY_SIZE);
}

void uwan_set_session(uint32_t dev_addr, uint32_t f_cnt_up, uint32_t f_cnt_down,
    const uint8_t *nwk_s_key, const uint8_t *app_s_key)
{
    uw_session.dev_addr = dev_addr;
    uw_session.f_cnt_up = f_cnt_up;
    uw_session.f_cnt_down = f_cnt_down;
    uw_session.ack_required = false;
    uw_session.dr = default_dr;

    memcpy(uw_session.nwk_s_key, nwk_s_key, UWAN_NWK_S_KEY_SIZE);
    memcpy(uw_session.app_s_key, app_s_key, UWAN_APP_S_KEY_SIZE);

    uw_session.is_joined = true;
}

bool uwan_is_joined()
{
    return uw_session.is_joined;
}

enum uwan_errs uwan_join()
{
    uint8_t offset = 0;

    if (uw_state != UWAN_STATE_IDLE)
        return UWAN_ERR_STATE;

    uint32_t frequency = channels_get_next();
    if (!frequency)
        return UWAN_ERR_CHANNEL;

    const struct node_dr *dr = &uw_dr_table[default_dr];

    pkt_params.sf = dr->sf;
    pkt_params.bw = dr->bw;
    pkt_params.inverted_iq = false;
    uw_radio->set_frequency(frequency);
    uw_radio->setup(&pkt_params);
    apply_tx_power();

    uw_session.is_joined = false;
    uw_is_join_state = true;
    uw_frame[offset++] = (UWAN_MTYPE_JOIN_REQUEST << MTYPE_OFFSET) | MAJOR_LORAWAN_R1;

    memcpy(&uw_frame[offset], uw_app_eui, UWAN_APP_EUI_SIZE);
    offset += UWAN_APP_EUI_SIZE;
    for (uint8_t i = UWAN_DEV_EUI_SIZE; i > 0; i--)
        uw_frame[offset++] = uw_dev_eui[i - 1];

    uw_dev_nonce = utils_get_random(65536);
    uw_frame[offset++] = uw_dev_nonce & 0xff;
    uw_frame[offset++] = (uw_dev_nonce >> 8) & 0xff;

    calc_mic(&uw_frame[offset], uw_frame, offset, uw_app_key, B0_DIR_UPLINK, 0,
        false);
    offset += MIC_LEN;

    uw_rx1_delay = default_join_delay;
    uw_rx2_delay = default_join_delay + SECOND_RX_OFFSET;
    uw_state = UWAN_STATE_TX;
    uw_radio->tx(uw_frame, offset);

    return UWAN_ERR_NO;
}

uint8_t uwan_get_max_payload_size()
{
    uint8_t max_pld_size = 0;
    enum uwan_dr dr = get_current_dr();

    if (dr < sizeof(uw_max_app_pld_size) / sizeof(uw_max_app_pld_size[0]))
        max_pld_size = uw_max_app_pld_size[dr];

    if (max_pld_size >= mac_get_payload_size())
        max_pld_size -= mac_get_payload_size();
    else
        max_pld_size = 0;

    return max_pld_size;
}

enum uwan_errs uwan_send_frame(uint8_t f_port, const uint8_t *payload,
    uint8_t pld_len, bool confirm)
{
    uint8_t offset = 0;

    if (uw_state != UWAN_STATE_IDLE)
        return UWAN_ERR_STATE;

    uint32_t frequency = channels_get_next();
    if (!frequency)
        return UWAN_ERR_CHANNEL;

    if (pld_len > uwan_get_max_payload_size())
        return UWAN_ERR_MSG_LEN;

    uint8_t mac_pld_size = mac_get_payload_size();
    if (!pld_len && !mac_pld_size)
        return UWAN_ERR_MSG_LEN;

    const struct node_dr *dr = &uw_dr_table[get_current_dr()];
    pkt_params.sf = dr->sf;
    pkt_params.bw = dr->bw;
    pkt_params.inverted_iq = false;
    uw_radio->set_frequency(frequency);
    uw_radio->setup(&pkt_params);
    apply_tx_power();

    uint8_t mtype;
    if (confirm)
        mtype = UWAN_MTYPE_CONF_DATA_UP;
    else
        mtype = UWAN_MTYPE_UNCONF_DATA_UP; // TODO take into account nbTrans

    uw_frame[offset++] = (mtype << MTYPE_OFFSET) | MAJOR_LORAWAN_R1;
    uw_frame[offset++] = uw_session.dev_addr & 0xff;
    uw_frame[offset++] = (uw_session.dev_addr >> 8) & 0xff;
    uw_frame[offset++] = (uw_session.dev_addr >> 16) & 0xff;
    uw_frame[offset++] = (uw_session.dev_addr >> 24) & 0xff;

    uint8_t f_ctrl = 0;
    if (uwan_adr_is_enabled())
        f_ctrl |= FCTRL_ADR;
    if (adr_get_req_bit())
        f_ctrl |= FCTRL_UPLINK_ADR_ACK_REQ;
    if (uw_session.ack_required) {
        uw_session.ack_required = false;
        f_ctrl |= FCTRL_ACK;
    }
    f_ctrl |= mac_get_payload_size() & FCTRL_FOPTS_MASK;
    uw_frame[offset++] = f_ctrl;
    uw_frame[offset++] = uw_session.f_cnt_up & 0xff;
    uw_frame[offset++] = (uw_session.f_cnt_up >> 8) & 0xff;
    offset += mac_get_payload(uw_frame + offset, 15);

    if (pld_len) {
        uw_frame[offset++] = f_port; // optional
        memcpy(&uw_frame[offset], payload, pld_len);
        // Encrypt FRMPayload before MIC calculation
        encrypt_payload(&uw_frame[offset], pld_len, uw_session.app_s_key,
            B0_DIR_UPLINK);
        offset += pld_len;
    }

    calc_mic(&uw_frame[offset], uw_frame, offset, uw_session.nwk_s_key,
        B0_DIR_UPLINK, uw_session.f_cnt_up, true);
    offset += MIC_LEN;

    uw_session.f_cnt_up++;

    uw_rx1_delay = default_rx1_delay;
    uw_rx2_delay = default_rx1_delay + SECOND_RX_OFFSET;
    uw_state = UWAN_STATE_TX;
    uw_radio->tx(uw_frame, offset);

    adr_handle_uplink();

    return UWAN_ERR_NO;
}

void uwan_timer_callback(enum uwan_timer_ids timer_id)
{
    if (uw_state == UWAN_STATE_RX1 && timer_id == UWAN_TIMER_RX1) {
        uw_radio->rx(FRAME_MAX_SIZE, RX_SYMB_TIMEOUT, 0);
    }
    else if (uw_state == UWAN_STATE_RX2 && timer_id == UWAN_TIMER_RX2) {
        uw_radio->rx(FRAME_MAX_SIZE, RX_SYMB_TIMEOUT, 0);
    }
}

void uwan_get_f_cnt(uint32_t *f_cnt_up, uint32_t *f_cnt_down)
{
    *f_cnt_up = uw_session.f_cnt_up;
    *f_cnt_down = uw_session.f_cnt_down;
}

void uwan_set_dr(enum uwan_dr dr)
{
    if (is_valid_dr(dr))
        default_dr = uw_session.dr = dr;
}

bool uwan_set_nb_trans(uint8_t nb_trans)
{
    if (set_nb_trans(nb_trans)) {
        default_nb_trans = nb_trans;
        return true;
    }

    return false;
}

void uwan_set_max_eirp(int8_t max_eirp)
{
    default_max_eirp = max_eirp;
}

bool uwan_set_tx_power(uint8_t tx_power)
{
    if (set_tx_power(tx_power)) {
        default_tx_power = tx_power;
        return true;
    }

    return false;
}

enum uwan_errs uwan_set_rx2(uint32_t frequency, enum uwan_dr dr)
{
    if (!is_valid_dr(dr))
        return UWAN_ERR_DATARATE;

    if (!is_valid_frequency(frequency))
        return UWAN_ERR_FREQUENCY;

    uw_rx2_frequency = frequency;
    uw_rx2_dr = dr;

    return UWAN_ERR_NO;
}

bool uwan_set_rx1_dr_offset(uint8_t rx1_dr_offset)
{
    if (is_valid_dr(rx1_dr_offset)) {
        uw_rx1_offset = rx1_dr_offset;
        return true;
    }

    return false;
}

bool uwan_set_rx1_delay(uint8_t delay)
{
    if (delay >= 1 && delay <= 15) {
        default_rx1_delay = delay * 1000;
        return true;
    }

    return false;
}

bool is_valid_dr(uint8_t dr)
{
    return (dr < UWAN_DR_COUNT);
}

bool is_valid_frequency(uint32_t freq)
{
    return (freq >= FREQ_MIN && freq <= FREQ_MAX);
}

bool set_nb_trans(uint8_t nb_trans)
{
    if (nb_trans >= NB_TRANS_MIN && nb_trans <= NB_TRANS_MAX) {
        uw_nb_trans = nb_trans;
        return true;
    }

    return false;
}

void reset_nb_trans()
{
    uw_nb_trans = default_nb_trans;
}

bool is_valid_tx_power(uint8_t tx_power)
{
    uint8_t count = sizeof(uw_tx_power_table) / sizeof(uw_tx_power_table[0]);
    return (tx_power < count);
}

bool set_tx_power(uint8_t tx_power)
{
    if (is_valid_tx_power(tx_power)) {
        uw_tx_power = tx_power;
        return true;
    }

    return false;
}

int8_t get_snr()
{
    return current_snr;
}
