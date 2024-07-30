/**
 * MIT License
 *
 * Copyright (c) 2021-2024 Alexey Ryabov
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

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define UWAN_DEV_EUI_SIZE 8
#define UWAN_APP_EUI_SIZE 8
#define UWAN_APP_KEY_SIZE 16
#define UWAN_NWK_S_KEY_SIZE 16
#define UWAN_APP_S_KEY_SIZE 16

#define UWAN_RX_NO_TIMEOUT 0
#define UWAN_RX_INFINITE 0xffffff

#define LORAWAN_PUBLIC_SYNC_WORD_MSB 0x34
#define LORAWAN_PUBLIC_SYNC_WORD_LSB 0x44
#define LORAWAN_PRIVATE_SYNC_WORD_MSB 0x14
#define LORAWAN_PRIVATE_SYNC_WORD_LSB 0x24
#define LORAWAN_CFLIST_SIZE 16

#define LORAWAN_MAC_BAT_LEVEL_EXT 0
#define LORAWAN_MAC_BAT_LEVEL_UNKNOWN 255

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
    // other datarates aren't supported yet
    UWAN_DR_COUNT,
};

enum uwan_errs {
    UWAN_ERR_NO,
    UWAN_ERR_STATE,
    UWAN_ERR_DATARATE,
    UWAN_ERR_CHANNEL,
    UWAN_ERR_FREQUENCY,
    UWAN_ERR_RX_TIMEOUT,
    UWAN_ERR_RX_CRC,
    UWAN_ERR_MSG_LEN,
    UWAN_ERR_MSG_MHDR,
    UWAN_ERR_MSG_MIC,
    UWAN_ERR_DEV_ADDR,
    UWAN_ERR_FCNT,
};

enum uwan_mtypes {
    UWAN_MTYPE_JOIN_REQUEST = 0x0,
    UWAN_MTYPE_JOIN_ACCEPT = 0x1,
    UWAN_MTYPE_UNCONF_DATA_UP = 0x2,
    UWAN_MTYPE_UNCONF_DATA_DOWN = 0x3,
    UWAN_MTYPE_CONF_DATA_UP = 0x4,
    UWAN_MTYPE_CONF_DATA_DOWN = 0x5,
    UWAN_MTYPE_PROPRIETARY = 0x7,
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

struct uwan_dl_packet {
    uint8_t *data;
    uint8_t size;
    uint8_t f_port;
    int16_t rssi;
    int8_t snr;
};

struct uwan_packet_params {
    enum uwan_sf sf;
    enum uwan_bw bw;
    enum uwan_cr cr;
    uint16_t preamble_len;
    bool crc_on;
    bool inverted_iq;
    bool implicit_header;
};

struct uwan_mac_callbacks {
    uint8_t (*get_battery_level)(void); // optional, see ch. 5.5 of LoRaWAN spec
};

struct radio_dev {
    bool (*init)(const struct radio_hal *hal, const void *opts);
    void (*sleep)(void);
    void (*set_frequency)(uint32_t frequency);
    bool (*set_power)(int8_t power);
    void (*set_public_network)(bool is_public);
    void (*setup)(const struct uwan_packet_params *params);
    void (*tx)(const uint8_t *buf, uint8_t len);
    void (*rx)(uint8_t len, uint16_t symb_timeout, uint32_t timeout);
    void (*read_packet)(struct uwan_dl_packet *pkt);
    uint32_t (*rand)(void);
    uint8_t (*irq_handler)(void);
    void (*set_evt_handler)(void (*handler)(uint8_t evt_mask));
};

struct stack_hal {
    void (*start_timer)(enum uwan_timer_ids timer_id, uint32_t timeout_ms);
    void (*stop_timer)(enum uwan_timer_ids timer_id);
    void (*downlink_callback)(enum uwan_errs err, enum uwan_mtypes m_type,
        const struct uwan_dl_packet *pkt);
};

struct uwan_region {
    void (*init)(void);
    void (*handle_cflist)(const uint8_t *cflist);
    bool (*handle_adr_ch_mask)(uint16_t ch_mask, uint8_t ch_mask_cntl,
        bool dry_run);
};

/**
 * \brief Initialize stack
 *
 * \param pointer to radio device (sx127x_dev or sx126x_dev)
 * \param pointer to hal for stack
 * \param pointer to region struct (region_eu868 for example)
 */
void uwan_init(const struct radio_dev *radio, const struct stack_hal *stack,
    const struct uwan_region *region);

/**
 * \brief Set keys for OTAA activation
 *
 * \param dev_eui pointer to device EUI
 * \param app_eui pointer to application EUI
 * \param app_key pointer to application key
 */
void uwan_set_otaa_keys(const uint8_t *dev_eui, const uint8_t *app_eui,
    const uint8_t *app_key);

/**
 * \brief Set keys for ABP activation
 *
 * \param dev_addr device address
 * \param f_cnt_up uplink fCnt
 * \param f_cnt_down downlinks fCnt
 * \param nwk_s_key pointer to network session key
 * \param app_s_key pointer to application session key
 */
void uwan_set_session(uint32_t dev_addr, uint32_t f_cnt_up, uint32_t f_cnt_down,
    const uint8_t *nwk_s_key, const uint8_t *app_s_key);

size_t uwan_get_session_size(void);

size_t uwan_save_session(void *dst, size_t dst_max_size);

bool uwan_restore_session(const void *src, size_t src_size);

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
 */
enum uwan_errs uwan_set_channel(uint8_t index, uint32_t frequency);

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
 * \brief Return maximum payload size available for application
 *
 * The application payload size depends on fOpts field size
 */
uint8_t uwan_get_max_payload_size(void);

/**
 * \brief Send uplink
 *
 * \param f_port application-specific port field (1..223)
 * \param payload pointer to payload, can be null if pld_len == 0
 * \param pld_len size of payload, can be zero to send MAC payload only
 * \param confirm send uplink with confirmation if true
 */
enum uwan_errs uwan_send_frame(uint8_t f_port, const uint8_t *payload,
    uint8_t pld_len, bool confirm);

/**
 * \brief Timer callback
 */
void uwan_timer_callback(enum uwan_timer_ids timer_id);

/**
 * \brief Set default datarate
 */
void uwan_set_dr(enum uwan_dr dr);

/**
 * \brief Set number of repeats for unconfirmed transmissions
 *
 * \param nb_trans number of repeats, valid values range from 1 to 15
 */
bool uwan_set_nb_trans(uint8_t nb_trans);

/**
 * \brief Set Max EIRP
 *
 * \param max_eirp value of Max EIRP
 */
void uwan_set_max_eirp(int8_t max_eirp);

/**
 * \brief Set index of tx power
 *
 * \param tx_power index of tx power, 0 equals max
 */
bool uwan_set_tx_power(uint8_t tx_power);

/**
 * \brief Set RX1 datarate offset
 *
 * \param rx1_dr_offset offset index from 0 to 5
 */
bool uwan_set_rx1_dr_offset(uint8_t rx1_dr_offset);

/**
 * \brief Set RX1 delay
 *
 * \param delay new delay in seconds from 1 to 15
 */
bool uwan_set_rx1_delay(uint8_t delay);

bool uwan_adr_is_enabled(void);

void uwan_adr_enable(bool enable);

void uwan_adr_setup_ack(uint8_t limit, uint8_t delay);

void uwan_set_mac_handlers(const struct uwan_mac_callbacks *cbs);

#endif
