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

#include <uwan/device/sx127x.h>

/* export funcs for radio driver struct */
static bool sx127x_init(const struct radio_hal *r_hal);
static void sx127x_set_freq(uint32_t freq);
static bool sx127x_set_power(int8_t power);
static void sx127x_setup(enum uwan_sf sf, enum uwan_bw bw, enum uwan_cr cr);
static void sx127x_tx(const uint8_t *buf, uint8_t len);
static void sx127x_rx(bool continous);
static uint8_t sx127x_handle_dio(int dio_num);

/* lookup table for spreading factor */
static const uint8_t sf_table[] = {
    MODEM_CONFIG2_SPREADING_FACTOR_6,
    MODEM_CONFIG2_SPREADING_FACTOR_7,
    MODEM_CONFIG2_SPREADING_FACTOR_8,
    MODEM_CONFIG2_SPREADING_FACTOR_9,
    MODEM_CONFIG2_SPREADING_FACTOR_10,
    MODEM_CONFIG2_SPREADING_FACTOR_11,
    MODEM_CONFIG2_SPREADING_FACTOR_12,
};

/* lookup table for bandwidth */
static const uint8_t bw_table[] = {
    MODEM_CONFIG1_BW_125KHZ,
    MODEM_CONFIG1_BW_250KHZ,
    MODEM_CONFIG1_BW_500KHZ,
};

/* lookup table for coding rate */
static const uint8_t cr_table[] = {
    MODEM_CONFIG1_CODING_RATE_4_5,
    MODEM_CONFIG1_CODING_RATE_4_6,
    MODEM_CONFIG1_CODING_RATE_4_7,
    MODEM_CONFIG1_CODING_RATE_4_8,
};

/* private pointer to actual HAL */
static const struct radio_hal *hal;

/* export radio driver */
const struct radio_dev sx127x_dev = {
    .init = sx127x_init,
    .set_frequency = sx127x_set_freq,
    .set_power = sx127x_set_power,
    .setup = sx127x_setup,
    .tx = sx127x_tx,
    .rx = sx127x_rx,
    .handle_dio = sx127x_handle_dio,
};

static void set_op_mode(uint8_t mode)
{
    uint8_t op_mode = radio_read_reg(hal, SX127X_REG_OP_MODE);
    op_mode = (op_mode & ~(_OP_MODE_MODE_MASK << _OP_MODE_MODE_SHIFT)) | mode;
    radio_write_reg(hal, SX127X_REG_OP_MODE, op_mode);
}

static void lora_set_modem_conf1(uint8_t bw, uint8_t cr, bool imp_header)
{
    uint8_t conf = bw | cr;

    if (imp_header) {
        conf |= MODEM_CONFIG1_IMPICIT_HEADER_MODE_ON;
    }

    radio_write_reg(hal, SX127X_REG_LR_MODEM_CONFIG1, conf);
}

static void lora_set_modem_conf2(uint8_t sf, bool crc_on, bool tx_cont,
    uint16_t symb_timeout)
{
    uint8_t conf = sf | ((symb_timeout >> 8) & _MODEM_CONFIG2_SYMB_TIMEOUT_MASK);

    if (crc_on) {
        conf |= MODEM_CONFIG2_RX_PAYLOAD_CRC_ON;
    }
    if (tx_cont) {
        conf |= MODEM_CONFIG2_TX_CONTINUOUS_MODE_ON;
    }

    radio_write_reg(hal, SX127X_REG_LR_MODEM_CONFIG2, conf);
}

static void lora_set_modem_conf3(bool low_dr_opti, bool agc_auto_on)
{
    uint8_t conf = 0x0;

    if (low_dr_opti) {
        conf |= MODEM_CONFIG3_LOW_DATA_RATE_OPTIMIZE_ON;
    }

    if (agc_auto_on) {
        conf |= MODEM_CONFIG3_AGC_AUTO_ON;
    }

    radio_write_reg(hal, SX127X_REG_LR_MODEM_CONFIG3, conf);
}

static void set_inverted_iq(bool inverted_iq)
{
    if (inverted_iq)
    {
        radio_write_reg(hal, SX127X_REG_LR_INVERT_IQ, 0x66);
        radio_write_reg(hal, SX127X_REG_LR_INVERT_IQ2, INVERT_IQ2_ON);
    }
    else
    {
        radio_write_reg(hal, SX127X_REG_LR_INVERT_IQ, 0x27);
        radio_write_reg(hal, SX127X_REG_LR_INVERT_IQ2, INVERT_IQ2_OFF);
    }
}

static bool sx127x_init(const struct radio_hal *r_hal)
{
    hal = r_hal;

    hal->reset(true);
    hal->delay_us(150); // >100us

    hal->reset(false);
    hal->delay_us(6000); // >5ms

    if (radio_read_reg(hal, SX127X_REG_VERSION) != VERSION_RESET_VALUE)
        return false;

    radio_write_reg(hal, SX127X_REG_OP_MODE, OP_MODE_MODE_SLEEP);
    radio_write_reg(hal, SX127X_REG_OP_MODE,
        OP_MODE_LONG_RANGE_MODE_ON | OP_MODE_MODE_SLEEP);

    radio_write_reg(hal, SX127X_REG_LR_SYNC_WORD, LORAWAN_SYNC_WORD);
    radio_write_reg(hal, SX127X_REG_PA_RAMP, PA_RAMP_PA_RAMP_50US);
    radio_write_reg(hal, SX127X_REG_LNA, LNA_LNA_GAIN_G1 | LNA_LNA_BOOST_HF_BOOST);

    return true;
}

static void sx127x_set_freq(uint32_t freq)
{
    uint64_t rf_carrier_freq = ((uint64_t)freq << 19) / 32000000;

    radio_write_reg(hal, SX127X_REG_FRF_MSB, (rf_carrier_freq >> 16) & 0xff);
    radio_write_reg(hal, SX127X_REG_FRF_MID, (rf_carrier_freq >> 8) & 0xff);
    radio_write_reg(hal, SX127X_REG_FRF_LSB, rf_carrier_freq & 0xff);
}

bool sx127x_set_power(int8_t power)
{
    uint8_t pa_conf = 0x0;

    if (power <= 14)
    {
        uint8_t pmax = 0x7; // set max power to 15
        pa_conf = PA_CONFIG_PA_SELECT_RFO | pmax << 4;
        pa_conf |= ((uint8_t)power & _PA_CONFIG_OUTPUT_POWER_MASK);
    }
    else
    {
        return false; // TODO
    }

    radio_write_reg(hal, SX127X_REG_PA_CONFIG, pa_conf);

    return true;
}

static void sx127x_setup(enum uwan_sf sf, enum uwan_bw bw, enum uwan_cr cr)
{
    const bool imp_header = false;
    const bool crc_on = true;
    uint16_t symb_timeout;

    symb_timeout = (sf >= UWAN_SF_10) ? 0x05 : 0x08;

    set_op_mode(OP_MODE_MODE_STDBY);

    lora_set_modem_conf1(bw_table[bw], cr_table[cr], imp_header);
    lora_set_modem_conf2(sf_table[sf], crc_on, false, symb_timeout);
    radio_write_reg(hal, SX127X_REG_LR_SYMB_TIMEOUT_LSB, symb_timeout & 0xff);

    bool low_dr_opti = (sf >= UWAN_SF_11);
    lora_set_modem_conf3(low_dr_opti, true);
}

static void sx127x_tx(const uint8_t *buf, uint8_t len)
{
    set_inverted_iq(false);

    radio_write_reg(hal, SX127X_REG_LR_FIFO_TX_BASE_ADDR, 0);
    radio_write_reg(hal, SX127X_REG_LR_FIFO_ADDR_PTR, 0);

    radio_write_array(hal, SX127X_REG_FIFO, buf, len);
    radio_write_reg(hal, SX127X_REG_LR_PAYLOAD_LENGTH, len);

    uint8_t mask = IRQ_FLAGS_MASK_TX_DONE_SET;
    radio_write_reg(hal, SX127X_REG_LR_IRQ_FLAGS_MASK, ~mask);
    radio_write_reg(hal, SX127X_REG_DIO_MAPPING1, DIO_MAPPING1_DIO0_LR_TX_DONE);

    set_op_mode(OP_MODE_MODE_TX);
}

static void sx127x_rx(bool continous)
{
    set_inverted_iq(true);

    radio_write_reg(hal, SX127X_REG_LR_FIFO_TX_BASE_ADDR, 0);
    radio_write_reg(hal, SX127X_REG_LR_FIFO_ADDR_PTR, 0);
    radio_write_reg(hal, SX127X_REG_LR_MAX_PAYLOAD_LENGTH, 0x40);

    // 500kHz Rx optimization
    radio_write_reg(hal, 0x36, 0x02);
    radio_write_reg(hal, 0x3a, 0x64);

    uint8_t mask = IRQ_FLAGS_MASK_RX_TIMEOUT_SET | IRQ_FLAGS_MASK_RX_DONE_SET |
        IRQ_FLAGS_MASK_PAYLOAD_CRC_ERROR_SET;
    radio_write_reg(hal, SX127X_REG_LR_IRQ_FLAGS_MASK, ~mask);
    radio_write_reg(hal, SX127X_REG_DIO_MAPPING1, DIO_MAPPING1_DIO0_LR_RX_DONE
        | DIO_MAPPING1_DIO1_LR_RX_TIMEOUT
        | DIO_MAPPING1_DIO3_LR_PAYLOAD_CRC_ERROR);

    if (continous)
        set_op_mode(OP_MODE_MODE_RX_CONTINUOUS);
    else
        set_op_mode(OP_MODE_MODE_RX_SINGLE);
}

static uint8_t sx127x_handle_dio(int dio_num)
{
    uint8_t result = 0;
    uint8_t flags = radio_read_reg(hal, SX127X_REG_LR_IRQ_FLAGS);
    uint8_t cflags = 0;

    if (flags & IRQ_FLAGS_TX_DONE_SET)
    {
        cflags |= IRQ_FLAGS_TX_DONE_SET;
        result |= RADIO_IRQF_TX_DONE;
    }

    if (flags & IRQ_FLAGS_RX_DONE_SET)
    {
        cflags |= IRQ_FLAGS_RX_DONE_SET;
        result |= RADIO_IRQF_RX_DONE;
    }

    if (flags & IRQ_FLAGS_RX_TIMEOUT_SET)
    {
        cflags |= IRQ_FLAGS_RX_TIMEOUT_SET;
        result |= RADIO_IRQF_RX_TIMEOUT;
    }

    radio_write_reg(hal, SX127X_REG_LR_IRQ_FLAGS, cflags);

    return result;
}
