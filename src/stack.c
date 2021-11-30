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
#include <stdlib.h>
#include <string.h>

#include <tinycrypt/aes.h>
#include <tinycrypt/cmac_mode.h>

#include <uwan/stack.h>

#define BYTES_FOR_BITS(bits) ((bits % 8) ? (bits / 8 + 1) : (bits / 8))

#define MAJOR_MASK 0x3
#define MAJOR_OFFSET 0
#define MAJOR_LORAWAN_R1 0x0

#define MTYPE_JOIN_REQUEST 0x0
#define MTYPE_JOIN_ACCEPT 0x1
#define MTYPE_UNCONF_DATA_UP 0x2
#define MTYPE_UNCONF_DATA_DOWN 0x3
#define MTYPE_CONF_DATA_UP 0x4
#define MTYPE_CONF_DATA_DOWN 0x5
#define MTYPE_MASK 0x7
#define MTYPE_OFFSET 5

#define FCTRL_ADR 0x80
#define FCTRL_UPLINK_ADR_ACK_REQ 0x40
#define FCTRL_ACK 0x20
#define FCTRL_DOWNLINK_FPENDING 0x10
#define FCTRL_UPLINK_CLASSB 0x10
#define FCTRL_FOPTS_MASK 0xf

#define AES_BLOCK_SIZE 16

#define MAX_CHANNELS 16

#define RECEIVE_DELAY1 1000
#define RECEIVE_DELAY2 2000
#define JOIN_ACCEPT_DELAY1 5000
#define JOIN_ACCEPT_DELAY2 6000

#define MIC_LEN 4

enum stack_states {
    UWAN_STATE_NOT_INIT,
    UWAN_STATE_IDLE,
    UWAN_STATE_TX,
    UWAN_STATE_RX1,
    UWAN_STATE_RX2,
};

struct node_session {
    uint8_t dev_eui[UWAN_DEV_EUI_SIZE];
    uint8_t app_eui[UWAN_APP_EUI_SIZE];
    uint8_t app_key[UWAN_APP_KEY_SIZE];
    uint32_t dev_addr;
    uint32_t dev_nonce;
    uint16_t f_cnt_up;
    uint16_t f_cnt_down;
    uint8_t nwk_s_key[UWAN_NWK_S_KEY_SIZE];
    uint8_t app_s_key[UWAN_APP_S_KEY_SIZE];
};

struct node_channel {
    uint32_t frequency; // 0 - is disabled
    enum uwan_dr dr_min;
    enum uwan_dr dr_max;
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

static const struct radio_dev *uw_radio;
static const struct stack_hal *uw_stack_hal;
static struct node_session uw_session;
static uint8_t uw_frame[255];
static uint8_t uw_frame_size;
static enum stack_states uw_state = UWAN_STATE_NOT_INIT;
static uint32_t uw_rx1_delay;
static uint32_t uw_rx2_delay;
static bool uw_is_joined;
static uint8_t uw_mtype;

/* Channel plan variables */
static uint8_t uw_channels_mask[BYTES_FOR_BITS(MAX_CHANNELS)];
static struct node_channel uw_channels[MAX_CHANNELS];
static uint32_t uw_rx2_frequency;
static enum uwan_dr uw_rx2_dr;

/* Encryption variables */
static struct tc_aes_key_sched_struct key_sched;
static struct tc_cmac_struct cmac_state;

static const struct node_channel *get_next_channel()
{
    // TODO improve this
    for (int i = 0; i < MAX_CHANNELS; i++)
    {
        if (uw_channels_mask[i / 8] & (1 << (i % 8)))
            return &uw_channels[i];
    }

    return NULL;
}

static void encrypt_payload(uint8_t dir, uint8_t *buf, uint8_t size)
{
    uint8_t a_block[AES_BLOCK_SIZE];
    uint8_t s_block[AES_BLOCK_SIZE];
    uint8_t a_block_i = 1;
    uint8_t src_pos = 0;

    tc_aes128_set_encrypt_key(&key_sched, uw_session.app_s_key);

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
    a_block[pos++] = uw_session.f_cnt_up & 0xff;
    a_block[pos++] = (uw_session.f_cnt_up >> 8) & 0xff;
    a_block[pos++] = 0x00;
    a_block[pos++] = 0x00;
    a_block[pos++] = 0x00;

    for (; size > 0; a_block_i++) {
        a_block[pos] = a_block_i;

        tc_aes_encrypt(s_block, a_block, &key_sched);

        uint8_t chunk_size = size >= AES_BLOCK_SIZE ? AES_BLOCK_SIZE : size;
        for (uint8_t i = 0; i < chunk_size; i++, src_pos++) {
            buf[src_pos] = buf[src_pos] ^ s_block[i];
        }
        size -= chunk_size;
    }
}

static void calc_mic(uint8_t *mic, const uint8_t *msg, uint8_t msg_len,
    const uint8_t *key, uint8_t dir, bool b0)
{
    uint8_t cmac_mic[AES_BLOCK_SIZE];

    tc_cmac_setup(&cmac_state, key, &key_sched);

    if (b0) {
        uint8_t block_b0[AES_BLOCK_SIZE];
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
        block_b0[pos++] = uw_session.f_cnt_up & 0xff;
        block_b0[pos++] = (uw_session.f_cnt_up >> 8) & 0xff;
        block_b0[pos++] = 0x00;
        block_b0[pos++] = 0x00;
        block_b0[pos++] = 0x00;
        block_b0[pos++] = msg_len;

        tc_cmac_update(&cmac_state, block_b0, sizeof(block_b0));
    }

    tc_cmac_update(&cmac_state, msg, msg_len);
    tc_cmac_final(cmac_mic, &cmac_state);

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
    key[offset++] = uw_session.dev_nonce & 0xff;
    key[offset++] = (uw_session.dev_nonce >> 8) & 0xff;
    while (offset < AES_BLOCK_SIZE)
        key[offset++] = 0x00;

    tc_aes128_set_encrypt_key(&key_sched, uw_session.app_key);
    tc_aes_encrypt(key, key, &key_sched);
}

static enum uwan_errs handle_join_msg(uint8_t *buf, uint8_t size)
{
    uint8_t mhdr;
    uint8_t mic[MIC_LEN];
    uint8_t offset = 0;
    bool cflist;

    tc_aes128_set_encrypt_key(&key_sched, uw_session.app_key);

    if (size == (AES_BLOCK_SIZE + 1))
        cflist = false;
    else if (size == (2 * AES_BLOCK_SIZE + 1))
        cflist = true;
    else
        return UWAN_ERR_MSG_LEN;

    mhdr = buf[offset++];
    if (mhdr != (MTYPE_JOIN_ACCEPT << MTYPE_OFFSET)
            | (MAJOR_LORAWAN_R1 << MAJOR_OFFSET))
        return UWAN_ERR_MSG_MHDR;

    tc_aes_encrypt(buf + sizeof(mhdr), buf + sizeof(mhdr), &key_sched);

    if (cflist) {
        tc_aes_encrypt(buf + sizeof(mhdr) + AES_BLOCK_SIZE,
            buf + sizeof(mhdr) + AES_BLOCK_SIZE, &key_sched);
    }

    calc_mic(mic, buf, size - sizeof(mic), uw_session.app_key, 0, false);
    if (memcmp(buf + size - sizeof(mic), mic, sizeof(mic)))
        return UWAN_ERR_MSG_MIC;

    uint32_t app_nonce, net_id;

    app_nonce = buf[offset++];
    app_nonce |= buf[offset++] << 8;
    app_nonce |= buf[offset++] << 16;

    net_id = buf[offset++];
    net_id |= buf[offset++] << 8;
    net_id |= buf[offset++] << 16;

    uw_session.dev_addr = buf[offset++];
    uw_session.dev_addr |= buf[offset++] << 8;
    uw_session.dev_addr |= buf[offset++] << 16;
    uw_session.dev_addr |= buf[offset++] << 24;

    // TODO DLSettings
    // TODO RxDelay

    const uint8_t nwk = 0x01;
    const uint8_t app = 0x02;
    derive_session_key(uw_session.nwk_s_key, nwk, app_nonce, net_id);
    derive_session_key(uw_session.app_s_key, app, app_nonce, net_id);

    return UWAN_ERR_NO;
}

static enum uwan_errs handle_data_msg(uint8_t *buf, uint8_t size)
{
    // TODO
    return UWAN_ERR_NO;
}

static void handle_downlink(enum uwan_errs err)
{
    enum uwan_status status = UWAN_ST_NO;

    if (err == UWAN_ERR_NO) {
        uw_frame_size = uw_radio->read_fifo(uw_frame, sizeof(uw_frame));
    }

    if (uw_mtype == MTYPE_JOIN_REQUEST)
    {
        if (err == UWAN_ERR_NO)
            err = handle_join_msg(uw_frame, uw_frame_size);

        if (err == UWAN_ERR_NO)
        {
            uw_is_joined = true;
            status = UWAN_ST_JOINED;
        }
        else
        {
            status = UWAN_ST_NOT_JOINED;
        }
    }
    else
    {
        err = handle_data_msg(uw_frame, uw_frame_size);
    }

    uw_stack_hal->downlink_callback(err, status);
}

void uwan_init(const struct radio_dev *radio, const struct stack_hal *stack)
{
    uw_state = UWAN_STATE_IDLE;
    uw_radio = radio;
    uw_stack_hal = stack;

    // disable all channels
    memset(uw_channels_mask, 0, sizeof(uw_channels_mask));

    srand(radio->rand());
}

void uwan_set_otaa_keys(const uint8_t *dev_eui, const uint8_t *app_eui,
    const uint8_t *app_key)
{
    memcpy(uw_session.dev_eui, dev_eui, UWAN_DEV_EUI_SIZE);
    memcpy(uw_session.app_eui, app_eui, UWAN_APP_EUI_SIZE);
    memcpy(uw_session.app_key, app_key, UWAN_APP_KEY_SIZE);
}

void uwan_set_session(uint32_t dev_addr, uint16_t f_cnt_up, uint16_t f_cnt_down,
    const uint8_t *nwk_s_key, const uint8_t *app_s_key)
{
    uw_session.dev_addr = dev_addr;
    uw_session.f_cnt_up = f_cnt_up;
    uw_session.f_cnt_down = f_cnt_down;

    memcpy(uw_session.nwk_s_key, nwk_s_key, UWAN_NWK_S_KEY_SIZE);
    memcpy(uw_session.app_s_key, app_s_key, UWAN_APP_S_KEY_SIZE);
}

bool uwan_is_joined()
{
    return uw_is_joined;
}

enum uwan_errs uwan_enable_channel(uint8_t index, bool enable)
{
    if (index >= MAX_CHANNELS)
        return UWAN_ERR_CHANNEL;

    if (enable)
        uw_channels_mask[index / 8] |= 1 << (index % 8);
    else
        uw_channels_mask[index / 8] &= ~(1 << (index % 8));

    return UWAN_ERR_NO;
}

enum uwan_errs uwan_set_channel(uint8_t index, uint32_t frequency,
    enum uwan_dr dr_min, enum uwan_dr dr_max)
{
    if (index >= MAX_CHANNELS)
        return UWAN_ERR_CHANNEL;
    if (dr_min > dr_max || dr_min < UWAN_DR_0 || dr_max >= UWAN_DR_COUNT)
        return UWAN_ERR_DATARATE;

    uw_channels[index].frequency = frequency;
    uw_channels[index].dr_min = dr_min;
    uw_channels[index].dr_max = dr_max;
    uw_channels_mask[index / 8] |= 1 << (index % 8);

    return UWAN_ERR_NO;
}

enum uwan_errs uwan_set_rx2(uint32_t frequency, enum uwan_dr dr)
{
    if (dr < UWAN_DR_0 || dr >= UWAN_DR_COUNT)
        return UWAN_ERR_DATARATE;

    uw_rx2_frequency = frequency;
    uw_rx2_dr = dr;

    return UWAN_ERR_NO;
}

enum uwan_errs uwan_join()
{
    const uint8_t dir = 0;
    uint8_t offset = 0;

    if (uw_state != UWAN_STATE_IDLE)
        return UWAN_ERR_STATE;

    const struct node_channel *ch = get_next_channel();
    if (ch == NULL)
        return UWAN_ERR_CHANNEL;

    const struct node_dr *dr = &uw_dr_table[ch->dr_max];
    uw_radio->set_frequency(ch->frequency);
    uw_radio->setup(dr->sf, dr->bw, UWAN_CR_4_5);

    uw_is_joined = false;
    uw_mtype = MTYPE_JOIN_REQUEST;
    uw_frame[offset++] = (uw_mtype << MTYPE_OFFSET) | MAJOR_LORAWAN_R1;

    memcpy(&uw_frame[offset], uw_session.app_eui, UWAN_APP_EUI_SIZE);
    offset += UWAN_APP_EUI_SIZE;
    for (uint8_t i = UWAN_DEV_EUI_SIZE; i > 0; i--)
        uw_frame[offset++] = uw_session.dev_eui[i - 1];

    uw_session.dev_nonce = rand() % 65536;
    uw_frame[offset++] = uw_session.dev_nonce & 0xff;
    uw_frame[offset++] = (uw_session.dev_nonce >> 8) & 0xff;

    calc_mic(&uw_frame[offset], uw_frame, offset, uw_session.app_key, dir, false);
    offset += 4;

    uw_rx1_delay = JOIN_ACCEPT_DELAY1;
    uw_rx2_delay = JOIN_ACCEPT_DELAY2;
    uw_state = UWAN_STATE_TX;
    uw_radio->tx(uw_frame, offset);

    return UWAN_ERR_NO;
}

enum uwan_errs uwan_send_frame(uint8_t f_port, const uint8_t *payload,
    uint8_t pld_len, bool confirm)
{
    const uint8_t dir = 0; // uplink
    uint8_t offset = 0;

    if (uw_state != UWAN_STATE_IDLE)
        return UWAN_ERR_STATE;

    const struct node_channel *ch = get_next_channel();
    if (ch == NULL)
        return UWAN_ERR_CHANNEL;

    const struct node_dr *dr = &uw_dr_table[ch->dr_max];
    uw_radio->set_frequency(ch->frequency);
    uw_radio->setup(dr->sf, dr->bw, UWAN_CR_4_5);

    if (confirm)
        uw_mtype = MTYPE_CONF_DATA_UP;
    else
        uw_mtype = MTYPE_UNCONF_DATA_UP;

    uw_frame[offset++] = (uw_mtype << MTYPE_OFFSET) | MAJOR_LORAWAN_R1;
    uw_frame[offset++] = uw_session.dev_addr & 0xff;
    uw_frame[offset++] = (uw_session.dev_addr >> 8) & 0xff;
    uw_frame[offset++] = (uw_session.dev_addr >> 16) & 0xff;
    uw_frame[offset++] = (uw_session.dev_addr >> 24) & 0xff;
    uw_frame[offset++] = 0x0; // FCtrl
    uw_frame[offset++] = uw_session.f_cnt_up & 0xff;
    uw_frame[offset++] = (uw_session.f_cnt_up >> 8) & 0xff;
    // TODO FOpts
    uw_frame[offset++] = f_port; // optional

    memcpy(&uw_frame[offset], payload, pld_len);
    // Encrypt FRMPayload before MIC calculation
    encrypt_payload(dir, &uw_frame[offset], pld_len);
    offset += pld_len;

    calc_mic(&uw_frame[offset], uw_frame, offset, uw_session.nwk_s_key, dir, true);
    offset += 4;

    uw_rx1_delay = RECEIVE_DELAY1;
    uw_rx2_delay = RECEIVE_DELAY2;
    uw_state = UWAN_STATE_TX;
    uw_radio->tx(uw_frame, offset);

    return UWAN_ERR_NO;
}

void uwan_radio_dio_callback(int dio_num)
{
    if (uw_state <= UWAN_STATE_IDLE)
        return;

    uint8_t flags = uw_radio->handle_dio(dio_num);

    switch (uw_state)
    {
    case UWAN_STATE_TX:
        uw_state = UWAN_STATE_RX1;
        uw_stack_hal->start_timer(UWAN_TIMER_RX1, uw_rx1_delay);
        uw_stack_hal->start_timer(UWAN_TIMER_RX2, uw_rx2_delay);
        break;

    case UWAN_STATE_RX1:
        if (flags & RADIO_IRQF_RX_TIMEOUT)
        {
            // prepare radio for RX2
            uw_state = UWAN_STATE_RX2;
            const struct node_dr *dr = &uw_dr_table[uw_rx2_dr];
            uw_radio->set_frequency(uw_rx2_frequency);
            uw_radio->setup(dr->sf, dr->bw, UWAN_CR_4_5);
        }
        else if (flags & RADIO_IRQF_RX_DONE)
        {
            // TODO handle_downlink(UWAN_ERR_RX_CRC);
            uw_stack_hal->stop_timer(UWAN_TIMER_RX2);
            handle_downlink(UWAN_ERR_NO);
            uw_state = UWAN_STATE_IDLE;
        }
        break;

    case UWAN_STATE_RX2:
        if (flags & RADIO_IRQF_RX_TIMEOUT)
        {
            handle_downlink(UWAN_ERR_RX_TIMEOUT);
        }
        else if (flags & RADIO_IRQF_RX_DONE)
        {
            // TODO handle_downlink(UWAN_ERR_RX_CRC);
            handle_downlink(UWAN_ERR_NO);
            uw_state = UWAN_STATE_IDLE;
        }
        break;

    default:
        break;
    }
}

void uwan_timer_callback(enum uwan_timer_ids timer_id)
{
    if (uw_state == UWAN_STATE_RX1 && timer_id == UWAN_TIMER_RX1)
    {
        uw_radio->rx(false);
    }
    else if (uw_state == UWAN_STATE_RX2 && timer_id == UWAN_TIMER_RX2)
    {
        uw_radio->rx(false);
    }
}
