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

#include <string.h>

#include <uwan/ext/clock_sync.h>
#include "../utils.h"

#define PACKAGE_ID 1
#define PACKAGE_VERSION 1

/* Commands transmitted by end device */
#define PACKAGE_VERSION_ANS 0x0
#define APP_TIME_REQ 0x1
#define DEVICE_APP_TIME_PERIODICITY_ANS 0x2

/* Commands transmitted by server */
#define PACKAGE_VERSION_REQ 0x0
#define APP_TIME_ANS 0x1
#define DEVICE_APP_TIME_PERIODICITY_REQ 0x2
#define FORCE_DEVICE_RESYNC_REQ 0x3

#define BUF_SIZE 32
#define APP_TIME_RQ_LEN 6

/* Saved settings */
static bool adr_enabled_prev;
static int nb_trans_prev;

static bool app_time_req_pending;
static uint8_t state_token_req;

static struct uwan_clock_sync_callbacks *cs_callbacks;
static uint8_t ans_buf[BUF_SIZE];
static uint8_t ans_buf_data_size;
static int force_resync_nb_of_rep = 1; // TODO
static bool ans_pending;

static uint32_t periodicity_from_period_id(int id)
{
    int mul = 2;

    /* 0 - 128 sec
     * 1 - 256 sec
     * 2 - 512 sec
     * 3 - 17 min
     * 4 - 34 min
     * 5 - 68 min
     * 6 - 136 min
     * 7 - 4,5 hours
     * 8 - 9 hours
     * 9 - 18 hours
     * 10 - 36 hours
     * 11 - 73 hours
     * 12 - 6 days
     * 13 - 12 days
     * 14 - 24 days
     * 15 - 48 days
     */

    if (id == 0) {
        mul = 1;
    }
    else {
        for (int i = 0; i < id - 1; i++)
            mul = mul * mul;
    }

    return 128 * mul;
}

static void handle_answ(const uint8_t *buf, uint8_t size)
{
    uint8_t offset = 0;
    uint8_t ans_buf_offset = 0;
    uint8_t exec_cmd_mask = 0;
    int32_t corr_value;
    uint8_t periodicity;
    uint8_t token;

    if (cs_callbacks == NULL)
        return;

    while (offset < size)
    {
        uint8_t cmd = buf[offset++];

        // Prevent re-execute command and buffer overflow
        uint8_t cmd_mask = (1 << cmd);
        if (exec_cmd_mask & cmd_mask)
            break;
        exec_cmd_mask |= (1 << cmd);

        switch (cmd)
        {
        case PACKAGE_VERSION_REQ:
            ans_buf[ans_buf_offset++] = PACKAGE_VERSION_ANS;
            ans_buf[ans_buf_offset++] = PACKAGE_ID;
            ans_buf[ans_buf_offset++] = PACKAGE_VERSION;
            break;

        case APP_TIME_ANS:
            if (size - offset < 5)
                break;
            corr_value = buf[offset++];
            corr_value |= buf[offset++] << 8;
            corr_value |= buf[offset++] << 16;
            corr_value |= buf[offset++] << 24;
            token = buf[offset++] & 0xf;

            if (token == state_token_req) {
                cs_callbacks->set_clock_correction(corr_value);
                state_token_req = (state_token_req + 1) & 0xf;
            }
            break;

        case DEVICE_APP_TIME_PERIODICITY_REQ:
            if (size - offset < 1)
                break;

            periodicity = buf[offset++];
            ans_buf[ans_buf_offset++] = DEVICE_APP_TIME_PERIODICITY_ANS;

            if (cs_callbacks->handle_app_time_periodicity_req == NULL)
                ans_buf[ans_buf_offset++] = 0x01; // Not supported
            else {
                uint32_t seconds = periodicity_from_period_id(periodicity);
                cs_callbacks->handle_app_time_periodicity_req(seconds);
                ans_buf[ans_buf_offset++] = 0x00;
            }

            uint32_t cur_time = utils_unix_to_gps(cs_callbacks->get_unixtime());
            ans_buf[ans_buf_offset++] = cur_time;
            ans_buf[ans_buf_offset++] = cur_time >> 8;
            ans_buf[ans_buf_offset++] = cur_time >> 16;
            ans_buf[ans_buf_offset++] = cur_time >> 24;
            break;

        case FORCE_DEVICE_RESYNC_REQ:
            force_resync_nb_of_rep = buf[offset++] & 0x7;
            if (force_resync_nb_of_rep != 0)
                cs_callbacks->force_device_resync_req();
            break;

        default:
            break;
        }
    }

    ans_buf_data_size = ans_buf_offset;
    ans_pending = ans_buf_offset != 0;
}

void uwan_clock_sync_init(struct uwan_clock_sync_callbacks *cbs)
{
    cs_callbacks = cbs;
    state_token_req = 0;
    app_time_req_pending = false;
    ans_pending = false;
    ans_buf_data_size = 0;
}

void uwan_clock_sync_handle_time_answ(enum uwan_errs err,
    enum uwan_mtypes m_type, const struct uwan_dl_packet *pkt)
{
    if (err != UWAN_ERR_NO)
        return;

    if (app_time_req_pending) {
        // TODO Restore ADR
        // TODO Restore NbTrans

        app_time_req_pending = false;
    }

    ans_pending = false;

    if (pkt->size && pkt->f_port == UWAN_EXT_CLOCK_SYNC_PORT)
        handle_answ(pkt->data, pkt->size);
}

enum uwan_errs uwan_clock_sync_send_time_req(bool ans_required)
{
    uint8_t rq_buf[BUF_SIZE];
    uint8_t offset = 0;

    if (cs_callbacks == NULL)
        return UWAN_ERR_STATE;

    if (ans_pending) {
        memcpy(rq_buf, ans_buf, ans_buf_data_size);
        offset = ans_buf_data_size;
    }

    if (app_time_req_pending == false) {
        // TODO Disable ADR
        // TODO Set NbTrans

        app_time_req_pending = true;
    }

    if (sizeof(rq_buf) - offset < APP_TIME_RQ_LEN) {
        offset = 0;
        ans_pending = false;
    }

    uint32_t cur_time = utils_unix_to_gps(cs_callbacks->get_unixtime());
    rq_buf[offset++] = APP_TIME_REQ;
    rq_buf[offset++] = cur_time;
    rq_buf[offset++] = cur_time >> 8;
    rq_buf[offset++] = cur_time >> 16;
    rq_buf[offset++] = cur_time >> 24;
    rq_buf[offset++] = state_token_req | (ans_required << 4);

    return uwan_send_frame(UWAN_EXT_CLOCK_SYNC_PORT, rq_buf, offset, false);
}

bool uwan_clock_sync_is_answ_pending()
{
    return ans_pending;
}

enum uwan_errs uwan_clock_sync_send_answ()
{
    if (ans_pending == false)
        return UWAN_ERR_STATE;

    return uwan_send_frame(UWAN_EXT_CLOCK_SYNC_PORT, ans_buf, ans_buf_data_size,
        false);
}
