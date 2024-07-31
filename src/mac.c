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

#include <string.h>

#include "adr.h"
#include "mac.h"
#include "stack.h"
#include "utils.h"

#define LINK_CHECK_ANS_PAYLOAD_SIZE 2
#define LINK_ADR_REQ_PAYLOAD_SIZE 4
#define DUTY_CYCLE_REQ_PAYLOAD_SIZE 1
#define RX_PARAM_SETUP_REQ_PAYLOAD_SIZE 4
#define DEV_STATUS_REQ_PAYLOAD_SIZE 0
#define NEW_CHANNEL_REQ_PAYLOAD_SIZE 5
#define RX_TIMING_SETUP_REQ_PAYLOAD_SIZE 1
#define TX_PARAM_SETUP_REQ_PAYLOAD_SIZE 1
#define DI_CHANNEL_REQ_PAYLOAD_SIZE 4
#define CID_DEVICE_TIME_ANS_PAYLOAD_SIZE 5

#define FREQ_STEP 100
#define DEV_STATUS_MARGIN_MASK 0x3f
#define RX_PARAM_STATUS_CHANNEL_ACK (1 << 0)
#define RX_PARAM_STATUS_RX2_DR_ACK (1 << 1)
#define RX_PARAM_STATUS_RX1_DR_OFFSET_ACK (1 << 2)
#define RX_PARAM_STATUS_OK 7
#define NEW_CHANNEL_STATUS_FREQ_ACK (1 << 0)
#define NEW_CHANNEL_STATUS_DR_RANGE_ACK (1 << 1)
#define NEW_CHANNEL_STATUS_OK 3

#define MAC_BUF_SIZE 15

static bool link_check(const uint8_t *pld);
static bool link_adr(const uint8_t *pld);
static bool duty_cycle(const uint8_t *pld);
static bool rx_param_setup(const uint8_t *pld);
static bool dev_status(const uint8_t *pld);
static bool new_channel(const uint8_t *pld);
static bool rx_timing_setup(const uint8_t *pld);
static bool tx_param_setup(const uint8_t *pld);
static bool di_channel(const uint8_t *pld);
static bool device_time(const uint8_t *pld);

static uint8_t mac_buf[MAC_BUF_SIZE];
static uint8_t mac_buf_pos;
static const struct uwan_mac_callbacks *mac_cbs;

const struct {
    bool (*handler)(const uint8_t *pld);
    uint8_t cid;
    uint8_t pld_size;
} mac_commands[] = {
    {link_check, CID_LINK_CHECK, LINK_CHECK_ANS_PAYLOAD_SIZE},
    {link_adr, CID_LINK_ADR, LINK_ADR_REQ_PAYLOAD_SIZE},
    {duty_cycle, CID_DUTY_CYCLE, DUTY_CYCLE_REQ_PAYLOAD_SIZE},
    {rx_param_setup, CID_RX_PARAM_SETUP, RX_PARAM_SETUP_REQ_PAYLOAD_SIZE},
    {dev_status, CID_DEV_STATUS, DEV_STATUS_REQ_PAYLOAD_SIZE},
    {new_channel, CID_NEW_CHANNEL, NEW_CHANNEL_REQ_PAYLOAD_SIZE},
    {rx_timing_setup, CID_RX_TIMING_SETUP, RX_TIMING_SETUP_REQ_PAYLOAD_SIZE},
    {tx_param_setup, CID_TX_PARAM_SETUP, TX_PARAM_SETUP_REQ_PAYLOAD_SIZE},
    {di_channel, CID_DI_CHANNEL, DI_CHANNEL_REQ_PAYLOAD_SIZE},
    {device_time, CID_DEVICE_TIME, CID_DEVICE_TIME_ANS_PAYLOAD_SIZE},
};

static bool link_check(const uint8_t *pld)
{
    if (mac_cbs && mac_cbs->link_check_result) {
        uint8_t margin = pld[0];
        uint8_t gw_cnt = pld[1];
        mac_cbs->link_check_result(margin, gw_cnt);
    }

    return true;
}

static bool link_adr(const uint8_t *pld)
{
    return adr_handle_link_req(pld[0], pld[1] | pld[2] << 8, pld[3]);
}

static bool duty_cycle(const uint8_t *pld)
{
    // TODO handle DutyCyclePL
    return mac_enqueue(CID_DUTY_CYCLE, NULL, 0);
}

static bool rx_param_setup(const uint8_t *pld)
{
    uint8_t dl_settings = pld[0];
    uint8_t rx1_dr_offset = (dl_settings >> 4) & 7;
    uint8_t rx2_dr = dl_settings & 0xf;
    uint32_t rx2_freq = (pld[1] | (pld[2] << 8) | (pld[3] << 16)) * FREQ_STEP;
    uint8_t status = 0;

    if (is_valid_dr(rx1_dr_offset))
        status |= RX_PARAM_STATUS_RX1_DR_OFFSET_ACK;

    if (is_valid_dr(rx2_dr))
        status |= RX_PARAM_STATUS_RX2_DR_ACK;

    if (is_valid_frequency(rx2_freq))
        status |= RX_PARAM_STATUS_CHANNEL_ACK;

    if (status == RX_PARAM_STATUS_OK) {
        uwan_set_rx1_dr_offset(rx1_dr_offset);
        uwan_set_rx2(rx2_freq, rx2_dr);
    }

    // TODO command should be added in the FOpt field of all uplinks until a
    // class A downlink is received by the end-device
    return mac_enqueue(CID_RX_PARAM_SETUP, &status, sizeof(status));
}

static bool dev_status(const uint8_t *pld)
{
    uint8_t status[] = {
        LORAWAN_MAC_BAT_LEVEL_UNKNOWN,
        (get_snr() & DEV_STATUS_MARGIN_MASK),
    };

    if (mac_cbs && mac_cbs->get_battery_level)
        status[0] = mac_cbs->get_battery_level();

    return mac_enqueue(CID_DEV_STATUS, status, sizeof(status));
}

static bool new_channel(const uint8_t *pld)
{
    uint8_t ch_index = pld[0];
    uint32_t freq = (pld[1] | (pld[2] << 8) | (pld[3] << 16)) * FREQ_STEP;
    uint8_t dr_range = pld[4];
    uint8_t status = 0;

    if (is_valid_frequency(freq))
        status |= NEW_CHANNEL_STATUS_FREQ_ACK;

    if (dr_range == 0x50) // TODO only fixed dr range is supported
        status |= NEW_CHANNEL_STATUS_DR_RANGE_ACK;

    if (status == NEW_CHANNEL_STATUS_OK) {
        if (uwan_set_channel(ch_index, freq) != UWAN_ERR_NO)
            status = 0;
    }

    return mac_enqueue(CID_NEW_CHANNEL, &status, sizeof(status));
}

static bool rx_timing_setup(const uint8_t *pld)
{
    uint8_t delay = pld[0] & 0xf;

    if (delay == 0)
        delay = 1;
    uwan_set_rx1_delay(delay);

    // TODO command should be added in the FOpt field of all uplinks until a
    // class A downlink is received by the end-device
    return mac_enqueue(CID_RX_TIMING_SETUP, NULL, 0);
}

static bool device_time(const uint8_t *pld)
{
    if (mac_cbs && mac_cbs->device_time_result) {
        uint32_t gps_seconds;
        gps_seconds = pld[0] | (pld[1] << 8) | (pld[2] << 16) | (pld[3] << 24);
        uint32_t unixtime = utils_gps_to_unix(gps_seconds);
        uint8_t fraq = pld[4];
        mac_cbs->device_time_result(unixtime, fraq);
    }

    return true;
}

static bool tx_param_setup(const uint8_t *pld)
{
    return true; // not supported, skip silently
}

static bool di_channel(const uint8_t *pld)
{
    return true; // not supported, skip silently
}

void uwan_mac_set_handlers(const struct uwan_mac_callbacks *cbs)
{
    mac_cbs = cbs;
}

bool uwan_mac_link_check_req()
{
    return mac_enqueue(CID_LINK_CHECK, NULL, 0);
}

bool uwan_mac_device_time_req()
{
    return mac_enqueue(CID_DEVICE_TIME, NULL, 0);
}

void mac_init()
{
    mac_buf_pos = 0;
}

void mac_handle_commands(const uint8_t *buf, uint8_t len)
{
    const uint8_t *start = buf;
    const uint8_t *end = start + len;
    const int mac_cmds_count = sizeof(mac_commands) / sizeof(mac_commands[0]);

    while (start < end) {
        uint8_t cid = *start++;
        bool status = false;

        for (int i = 0; i < mac_cmds_count; i++) {
            if (cid == mac_commands[i].cid) {
                if ((end - start) >= mac_commands[i].pld_size) {
                    status = mac_commands[i].handler(start);
                    start += mac_commands[i].pld_size;
                }
                break;
            }
        }

        if (status != true)
            break;
    }
}

bool mac_enqueue(uint8_t cid, const uint8_t *data, uint8_t size)
{
    uint8_t free = sizeof(mac_buf) - mac_buf_pos;

    if ((sizeof(cid) + size) > free)
        return false;

    mac_buf[mac_buf_pos++] = cid;
    if (size) {
        memcpy(mac_buf + mac_buf_pos, data, size);
        mac_buf_pos += size;
    }

    return true;
}

uint8_t mac_get_payload_size()
{
    return mac_buf_pos;
}

uint8_t mac_get_payload(uint8_t *buf, uint8_t buf_size)
{
    uint8_t result = 0;

    if (buf_size >= mac_buf_pos) {
        memcpy(buf, mac_buf, mac_buf_pos);
        result = mac_buf_pos;
        mac_buf_pos = 0;
    }

    return result;
}
