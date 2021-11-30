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
static uint32_t radio_freq;
static enum uwan_sf radio_sf;
static enum uwan_bw radio_bw;
static enum uwan_cr radio_cr;

void radio_set_frequency(uint32_t frequency)
{
    radio_freq = frequency;
}

void radio_setup(enum uwan_sf sf, enum uwan_bw bw, enum uwan_cr cr)
{
    radio_sf = sf;
    radio_bw = bw;
    radio_cr = cr;
}

static void radio_tx(const uint8_t *buf, uint8_t len)
{
    memcpy(tx_frame, buf, len);
    tx_frame_size = len;
}

static void radio_rx(bool continuous)
{
}

static uint32_t radio_rand(void)
{
    return 0;
}

static uint8_t radio_handle_dio(int dio_num)
{
    uint8_t irq = 0;
    return irq;
}

static const struct radio_dev radio = {
    .set_frequency = radio_set_frequency,
    .setup = radio_setup,
    .tx = radio_tx,
    .rx = radio_rx,
    .rand = radio_rand,
    .handle_dio = radio_handle_dio,
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

void app_start_timer(enum uwan_timer_ids timer_id, uint32_t timeout_ms)
{

}

void app_stop_timer(enum uwan_timer_ids timer_id)
{

}

void app_downlink_callback(enum uwan_errs err, enum uwan_status status)
{

}

static const struct stack_hal app_hal = {
    .start_timer = app_start_timer,
    .stop_timer = app_stop_timer,
    .downlink_callback = app_downlink_callback,
};

int main()
{
    enum uwan_errs result;

    const uint32_t dev_addr = 0x01020304;
    const uint16_t f_cnt_up = 2;
    const uint16_t f_cnt_down = 2;
    const uint8_t f_port = 4;
    const bool confirm = false;

    uwan_init(&radio, &app_hal);
    uwan_set_session(dev_addr, f_cnt_up, f_cnt_down, nwkskey, appskey);
    result = uwan_send_frame(f_port, tx_payload, sizeof(tx_payload), confirm);
    assert(result == UWAN_ERR_CHANNEL);

    result = uwan_set_channel(1, 868800000, UWAN_DR_0, UWAN_DR_5);
    assert(result == UWAN_ERR_NO);
    result = uwan_send_frame(f_port, tx_payload, sizeof(tx_payload), confirm);
    assert(result == UWAN_ERR_NO);

    assert(radio_freq == 868800000);
    assert(radio_bw == UWAN_BW_125);
    assert(radio_sf == UWAN_SF_7);
    assert(radio_cr == UWAN_CR_4_5);

    const uint8_t mic[] = {0xbf, 0x26, 0x16, 0x0a};
    for (int i = 0; i < sizeof(mic); i++) {
        assert(mic[i] == tx_frame[tx_frame_size - 4 + i]);
    }

    const uint8_t enc_payload[] = {0xb8, 0x66, 0x87, 0x5b};
    for (int i = 0; i < sizeof(enc_payload); i++) {
        assert(enc_payload[i] == tx_frame[tx_frame_size - 8 + i]);
    }

    // TODO test rx

    return 0;
}
