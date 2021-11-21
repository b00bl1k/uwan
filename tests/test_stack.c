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

#include <assert.h>
#include <string.h>
#include <uwan/stack.h>

static uint8_t tx_frame[256];
static uint8_t tx_frame_size;

static bool radio_tx(const uint8_t *buf, uint8_t len)
{
    memcpy(tx_frame, buf, len);
    tx_frame_size = len;
}

static const struct radio_dev radio = {
    .tx = radio_tx,
};

static const uint8_t nwkskey[] = {
    0x00, 0x01, 0x02, 0x03,
    0x00, 0x01, 0x02, 0x03,
    0x00, 0x01, 0x02, 0x03,
    0x00, 0x01, 0x02, 0x03,
};

static const uint8_t appskey[] = {
    0x04, 0x05, 0x06, 0x07,
    0x04, 0x05, 0x06, 0x07,
    0x04, 0x05, 0x06, 0x07,
    0x04, 0x05, 0x06, 0x07,
};

static const uint8_t tx_payload[] = {
    0x00, 0x01, 0x02, 0x03,
};

int main()
{
    const uint32_t dev_addr = 0x01020304;
    const uint16_t f_cnt_up = 2;
    const uint16_t f_cnt_down = 2;
    const uint8_t f_port = 4;
    const bool confirm = false;

    uwan_init(&radio);
    uwan_set_session(dev_addr, f_cnt_up, f_cnt_down, nwkskey, appskey);
    uwan_send_frame(f_port, tx_payload, sizeof(tx_payload), confirm);

    uint8_t mic[] = {0xbf, 0x26, 0x16, 0x0a};
    for (int i = 0; i < sizeof(mic); i++) {
        assert(mic[i] == tx_frame[tx_frame_size - 4 + i]);
    }

    uint8_t enc_payload[] = {0xb8, 0x66, 0x87, 0x5b};
    for (int i = 0; i < sizeof(enc_payload); i++) {
        assert(enc_payload[i] == tx_frame[tx_frame_size - 8 + i]);
    }

    return 0;
}
