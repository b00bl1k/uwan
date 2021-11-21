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

#ifndef __UWAN_STACK_H__
#define __UWAN_STACK_H__

#include <stdint.h>
#include <stdbool.h>

#define UWAN_NWK_S_KEY_SIZE 16
#define UWAN_APP_S_KEY_SIZE 16

#define LORAWAN_SYNC_WORD 0x34

enum uwan_sf {
    UWAN_SF_6,
    UWAN_SF_7,
    UWAN_SF_8,
    UWAN_SF_9,
    UWAN_SF_10,
    UWAN_SF_11,
    UWAN_SF_12,
};

enum uwan_bw {
    UWAN_BW_125,
    UWAN_BW_250,
    UWAN_BW_500,
};

enum uwan_cr {
    UWAN_CR_4_5,
    UWAN_CR_4_6,
    UWAN_CR_4_7,
    UWAN_CR_4_8,
};

struct radio_hal
{
    uint8_t (*spi_xfer)(uint8_t data);
    void (*reset)(bool enable);
    void (*select)(bool enable);
    void (*delay_us)(uint32_t us);
};

struct radio_dev
{
    bool (*init)(const struct radio_hal *hal);
    void (*set_frequency)(uint32_t frequency);
    bool (*set_power)(int8_t power);
    void (*set_inverted_iq)(bool inverted_iq);
    void (*setup)(enum uwan_sf sf, enum uwan_bw bw, enum uwan_cr cr);
    bool (*tx)(const uint8_t *buf, uint8_t len);
};

void uwan_init(const struct radio_dev *radio);

void uwan_set_session(uint32_t dev_addr, uint16_t f_cnt_up, uint16_t f_cnt_down,
    const uint8_t *nwk_s_key, const uint8_t *app_s_key);

void uwan_send_frame(uint8_t f_port, const uint8_t *payload, uint8_t pld_len,
    bool confirm);

#endif /* ~__UWAN_STACK_H__ */
