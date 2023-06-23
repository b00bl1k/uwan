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

#include <string.h>

#include "adr.h"
#include "mac.h"

#define LINK_ADR_REQ_PAYLOAD_SIZE 4
#define MAC_BUF_SIZE 15

static bool link_adr_parser(const uint8_t **start, const uint8_t *end);
static bool dev_status_parser(const uint8_t **start, const uint8_t *end);

static uint8_t mac_buf[MAC_BUF_SIZE];
static uint8_t mac_buf_pos;

const struct {
    uint8_t cid;
    bool (*handler)(const uint8_t **start, const uint8_t *end);
} mac_commands[] = {
    {CID_LINK_ADR, link_adr_parser},
    {CID_DEV_STATUS, dev_status_parser},
};

static bool link_adr_parser(const uint8_t **start, const uint8_t *end)
{
    const uint8_t *src = *start;

    if ((end - src) < LINK_ADR_REQ_PAYLOAD_SIZE)
        return false;

    *start = src + LINK_ADR_REQ_PAYLOAD_SIZE;
    return adr_handle_link_req(src[0], src[1] | src[2] << 8, src[3]);
}

static bool dev_status_parser(const uint8_t **start, const uint8_t *end)
{
    uint8_t status[] = {254, 25}; // TODO
    return mac_enqueue_ans(CID_DEV_STATUS, status, sizeof(status));
}

void mac_handle_commands(const uint8_t *buf, uint8_t len)
{
    const uint8_t *start = buf;
    const uint8_t *end = start + len;

    while (start < end) {
        uint8_t cid = *start++;
        bool status = false;

        for (int i = 0; i < sizeof(mac_commands) / sizeof(mac_commands[0]); i++) {
            if (cid == mac_commands[i].cid) {
                status = mac_commands[i].handler(&start, end);
                break;
            }
        }

        if (status != true)
            break;
    }
}

bool mac_enqueue_ans(uint8_t cid, const uint8_t *data, uint8_t size)
{
    uint8_t free = sizeof(mac_buf) - mac_buf_pos;

    if ((sizeof(cid) + size) > free)
        return false;

    mac_buf[mac_buf_pos++] = cid;
    memcpy(mac_buf + mac_buf_pos, data, size);
    mac_buf_pos += size;

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
