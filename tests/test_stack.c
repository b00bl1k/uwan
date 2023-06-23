/**
 * MIT License
 *
 * Copyright (c) 2021-2023 Alexey Ryabov
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
#include "utils.h"

static uint8_t radio_frame[256];
static uint8_t radio_frame_size;
static uint32_t radio_freq;
static int radio_sleep_call_count;
static enum uwan_sf radio_sf;
static enum uwan_bw radio_bw;
static enum uwan_cr radio_cr;
static uint8_t radio_dio_irq;
static enum uwan_errs app_err;
static enum uwan_mtypes app_m_type;
static int app_downlink_callback_call_count;
static void (*app_evt_handler)(uint8_t evt_mask);

static const uint8_t dev_eui[] = {
    0x00, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x06, 0x07,
};

static const uint8_t app_eui[] = {
    0x11, 0x22, 0x33, 0x44,
    0x55, 0x66, 0x77, 0x88,
};

static const uint8_t app_key[] = {
    0x04, 0x05, 0x06, 0x07,
    0x04, 0x05, 0x06, 0x07,
    0x04, 0x05, 0x06, 0x07,
    0x04, 0x05, 0x06, 0x07,
};

static const uint8_t tx_payload[] = {
    0x00, 0x01, 0x02, 0x03,
};

static void radio_set_frequency(uint32_t frequency)
{
    radio_freq = frequency;
}

static void radio_sleep(void)
{
    radio_sleep_call_count++;
}

static void radio_setup(const struct uwan_packet_params *params)
{
    radio_sf = params->sf;
    radio_bw = params->bw;
    radio_cr = params->cr;
}

static void radio_tx(const uint8_t *buf, uint8_t len)
{
    memcpy(radio_frame, buf, len);
    radio_frame_size = len;
}

static void radio_rx(uint8_t len, uint16_t symb_timeout, uint32_t timeout)
{
}

static uint8_t radio_read_packet(void *buf, uint8_t buf_size, int16_t *rssi,
    int8_t *snr)
{
    uint8_t size = buf_size < radio_frame_size ? buf_size : radio_frame_size;
    memcpy(buf, radio_frame, size);
    return size;
}

static uint32_t radio_rand(void)
{
    return 0;
}

void utils_random_init(uint32_t seed)
{
}

uint32_t utils_get_random(uint32_t max)
{
    return 0x01234567 % max;
}

static void radio_irq_handler(void)
{
    if (app_evt_handler)
        app_evt_handler(radio_dio_irq);
}

static void radio_set_evt_handler(void (*handler)(uint8_t evt_mask))
{
    app_evt_handler = handler;
}

static const struct radio_dev radio = {
    .set_frequency = radio_set_frequency,
    .sleep = radio_sleep,
    .setup = radio_setup,
    .tx = radio_tx,
    .rx = radio_rx,
    .read_packet = radio_read_packet,
    .rand = radio_rand,
    .irq_handler = radio_irq_handler,
    .set_evt_handler = radio_set_evt_handler,
};

void app_start_timer(enum uwan_timer_ids timer_id, uint32_t timeout_ms)
{

}

void app_stop_timer(enum uwan_timer_ids timer_id)
{

}

void app_downlink_callback(enum uwan_errs err, enum uwan_mtypes m_type,
    uint8_t f_port, uint8_t *payload, uint8_t payload_size, int16_t rssi,
    int8_t snr)
{
    app_downlink_callback_call_count++;
    app_err = err;
    app_m_type = m_type;
}

static const struct stack_hal app_hal = {
    .start_timer = app_start_timer,
    .stop_timer = app_stop_timer,
    .downlink_callback = app_downlink_callback,
};

void test_join_successfull()
{
    enum uwan_errs result;
    const uint8_t join_request[] = {
        0x00, // MHDR
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, // AppEUI
        0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00, // DevEUI
        0x67, 0x45, // DevNonce
        0x25, 0xe6, 0x57, 0xa3, // MIC
    };
    const uint8_t join_accept[] = {
        0x20, // MHDR
        // 0x01, 0x02, 0x03, // AppNonce
        // 0xaa, 0xbb, 0xcc, // NetId
        // 0x00, 0x01, 0x02, 0x03, // DevAddr
        // 0x00, // DLSettings
        // 0x01, // RxDelay
        0xe8, 0x14, 0x30, 0x62,
        0x0d, 0x86, 0xc0, 0xda,
        0xd7, 0x5a, 0xf4, 0x7a,
        0x61, 0xf0, 0xef, 0x0b,
    };

    uwan_set_otaa_keys(dev_eui, app_eui, app_key);
    result = uwan_join();
    assert(result == UWAN_ERR_NO);

    assert(radio_frame_size == sizeof(join_request));
    assert(memcmp(join_request, radio_frame, radio_frame_size) == 0);

    radio_dio_irq = RADIO_IRQF_TX_DONE;
    radio.irq_handler();

    uwan_timer_callback(UWAN_TIMER_RX1);

    radio_frame_size = sizeof(join_accept);
    memcpy(radio_frame, join_accept, radio_frame_size);

    radio_dio_irq = RADIO_IRQF_RX_DONE;
    radio.irq_handler();

    assert(radio_sleep_call_count == 1);
    assert(app_downlink_callback_call_count == 1);
    assert(app_err == UWAN_ERR_NO);
    assert(app_m_type == UWAN_MTYPE_JOIN_ACCEPT);
    assert(uwan_is_joined());
}

void test_send_uplink_successfull()
{
    enum uwan_errs result;

    const uint8_t f_port = 4;
    const bool confirm = false;

    result = uwan_send_frame(f_port, tx_payload, sizeof(tx_payload), confirm);
    assert(result == UWAN_ERR_NO);

    assert(radio_freq == 868800000);
    assert(radio_bw == UWAN_BW_125);
    assert(radio_sf == UWAN_SF_7);
    assert(radio_cr == UWAN_CR_4_5);

    const uint8_t uplink[] = {
        0x40, // MHDR
        0x00, 0x01, 0x02, 0x03, // DevAddr
        0x00, 0x00, 0x00, // FHDR
        f_port,
        0xce, 0x88, 0x53, 0x59, // Payload
        0x61, 0xfa, 0xf3, 0xd8, // MIC
    };

    assert(memcmp(uplink, radio_frame, radio_frame_size) == 0);
}

int main()
{
    enum uwan_errs result;

    uwan_init(&radio, &app_hal);

    assert(uwan_join() == UWAN_ERR_CHANNEL);

    result = uwan_set_channel(1, 868800000, UWAN_DR_0, UWAN_DR_5);
    assert(result == UWAN_ERR_NO);

    uwan_set_default_dr(UWAN_DR_5);

    test_join_successfull();
    test_send_uplink_successfull();

    return 0;
}
