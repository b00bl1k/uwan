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

#ifndef __UWAN_STACK_H__
#define __UWAN_STACK_H__

#include <stdint.h>
#include <stdbool.h>

#define UWAN_DEV_EUI_SIZE 8
#define UWAN_APP_EUI_SIZE 8
#define UWAN_APP_KEY_SIZE 16
#define UWAN_NWK_S_KEY_SIZE 16
#define UWAN_APP_S_KEY_SIZE 16

#define UWAN_RX_NO_TIMEOUT 0
#define UWAN_RX_INFINITE 0xffffff

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

enum uwan_dr {
    UWAN_DR_0,
    UWAN_DR_1,
    UWAN_DR_2,
    UWAN_DR_3,
    UWAN_DR_4,
    UWAN_DR_5,
    // other datarates not support yet
    UWAN_DR_COUNT,
};

enum uwan_errs {
    UWAN_ERR_NO,
    UWAN_ERR_STATE,
    UWAN_ERR_DATARATE,
    UWAN_ERR_CHANNEL,
    UWAN_ERR_RX_TIMEOUT,
    UWAN_ERR_RX_CRC,
    UWAN_ERR_MSG_LEN,
    UWAN_ERR_MSG_MHDR,
    UWAN_ERR_MSG_MIC,
};

enum uwan_status {
    UWAN_ST_NO,
    UWAN_ST_JOINED,
    UWAN_ST_NOT_JOINED,
};

enum uwan_timer_ids {
    UWAN_TIMER_RX1,
    UWAN_TIMER_RX2,
};

enum radio_irq_flags {
    RADIO_IRQF_RX_TIMEOUT = 0x1,
    RADIO_IRQF_RX_DONE = 0x2,
    RADIO_IRQF_TX_DONE = 0x4,
    RADIO_IRQF_CRC_ERROR = 0x8,
};

struct radio_hal {
    uint8_t (*spi_xfer)(uint8_t data);
    void (*reset)(bool enable);
    void (*select)(bool enable);
    void (*delay_us)(uint32_t us);
    bool (*is_busy)(void); // only for sx126x
    void (*io_init)(void); // optional
    void (*io_deinit)(void); // optional
    void (*ant_sw_ctrl)(bool is_rx); // optional
};

struct radio_dev {
    bool (*init)(const struct radio_hal *hal);
    void (*sleep)(void);
    void (*set_frequency)(uint32_t frequency);
    bool (*set_power)(int8_t power);
    void (*setup)(enum uwan_sf sf, enum uwan_bw bw, enum uwan_cr cr);
    void (*tx)(const uint8_t *buf, uint8_t len);
    void (*rx)(uint16_t symb_timeout, uint32_t timeout);
    uint8_t (*read_fifo)(uint8_t *buf, uint8_t buf_size);
    uint32_t (*rand)(void);
    void (*irq_handler)(void);
    void (*set_evt_handler)(void (*handler)(uint8_t evt_mask));
};

struct stack_hal {
    void (*start_timer)(enum uwan_timer_ids timer_id, uint32_t timeout_ms);
    void (*stop_timer)(enum uwan_timer_ids timer_id);
    void (*downlink_callback)(enum uwan_errs err, enum uwan_status status);
};

void uwan_init(const struct radio_dev *radio, const struct stack_hal *stack);

void uwan_set_otaa_keys(const uint8_t *dev_eui, const uint8_t *app_eui,
    const uint8_t *app_key);

void uwan_set_session(uint32_t dev_addr, uint32_t f_cnt_up, uint32_t f_cnt_down,
    const uint8_t *nwk_s_key, const uint8_t *app_s_key);

/**
 * \brief Check for stack is joined
 */
bool uwan_is_joined(void);

/**
 * \brief Enable or disable channel
 *
 * \param index index of channel in range 0..(MAX_CHANNELS - 1)
 * \param enable state of channel. true for enable, false for disable
 */
enum uwan_errs uwan_enable_channel(uint8_t index, bool enable);

/**
 * \brief Set and enable channel
 *
 * \param index index of channel in range 0..(MAX_CHANNELS - 1)
 * \param frequency actual channel frequency in Hz
 * \param dr_min minimum data rate
 * \param dr_max maximum data rate
 */
enum uwan_errs uwan_set_channel(uint8_t index, uint32_t frequency,
    enum uwan_dr dr_min, enum uwan_dr dr_max);

/**
 * \brief Setup RX2 window
 *
 * \param frequency frequency in Hz
 * \param dr data rate
 */
enum uwan_errs uwan_set_rx2(uint32_t frequency, enum uwan_dr dr);

/**
 * \brief Send join-request message
 *
 * Network paramaters should be set by \sa uwan_set_otaa_keys
 */
enum uwan_errs uwan_join(void);

/**
 * \brief Send uplink
 *
 * \param f_port application-specific port field (1..223)
 * \param payload pointer to payload
 * \param pld_len size of payload
 * \param confirm send uplink with confirmation if true
 */
enum uwan_errs uwan_send_frame(uint8_t f_port, const uint8_t *payload,
    uint8_t pld_len, bool confirm);

/**
 * \brief Timer callback
 */
void uwan_timer_callback(enum uwan_timer_ids timer_id);

#endif /* ~__UWAN_STACK_H__ */
