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

// TODO errors handling, busy timeout

#include <uwan/device/sx126x.h>

static void wait_busy_on(void);
static void check_device(void);

/* export funcs for radio driver struct */
static bool sx126x_init(const struct radio_hal *r_hal);
static void sx126x_sleep(void);
static void sx126x_set_freq(uint32_t freq);
static bool sx126x_set_power(int8_t power);
static void sx126x_setup(enum uwan_sf sf, enum uwan_bw bw, enum uwan_cr cr);
static void sx126x_tx(const uint8_t *buf, uint8_t len);
static void sx126x_rx(bool continous);
static uint8_t sx126x_read_fifo(uint8_t *buf, uint8_t buf_size);
static uint32_t sx126x_rand(void);
static uint8_t sx126x_handle_dio(int dio_num);

/* private pointer to actual HAL */
static const struct radio_hal *hal;
static bool is_sleep;
static uint16_t preamble_length;

/* export radio driver */
const struct radio_dev sx126x_dev = {
    .init = sx126x_init,
    .sleep = sx126x_sleep,
    .set_frequency = sx126x_set_freq,
    .set_power = sx126x_set_power,
    .setup = sx126x_setup,
    .tx = sx126x_tx,
    .rx = sx126x_rx,
    .read_fifo = sx126x_read_fifo,
    .rand = sx126x_rand,
    .handle_dio = sx126x_handle_dio,
};

/* lookup table for spreading factor */
static const uint8_t sf_table[] = {
    LORA_MOD_PARAM1_SF6,
    LORA_MOD_PARAM1_SF7,
    LORA_MOD_PARAM1_SF8,
    LORA_MOD_PARAM1_SF9,
    LORA_MOD_PARAM1_SF10,
    LORA_MOD_PARAM1_SF11,
    LORA_MOD_PARAM1_SF12,
};

/* lookup table for bandwidth */
static const uint8_t bw_table[] = {
    LORA_MOD_PARAM2_BW_125,
    LORA_MOD_PARAM2_BW_250,
    LORA_MOD_PARAM2_BW_500,
};

/* lookup table for coding rate */
static const uint8_t cr_table[] = {
    LORA_MOD_PARAM3_CR_4_5,
    LORA_MOD_PARAM3_CR_4_6,
    LORA_MOD_PARAM3_CR_4_7,
    LORA_MOD_PARAM3_CR_4_8,
};

static void wait_busy_on()
{
    while (hal->is_busy());
}

static void check_device()
{
    if (is_sleep) {
        hal->select(true);
        hal->spi_xfer(SX126X_CMD_GET_STATUS);
        hal->spi_xfer(0);
        hal->select(false);

        wait_busy_on();

        hal->select(true);
        hal->spi_xfer(SX126X_CMD_SET_STANDBY);
        hal->spi_xfer(0);
        hal->select(false);
        is_sleep = false;
    }

    wait_busy_on();
}

static uint8_t read_register(uint16_t addr)
{
    uint8_t result;

    check_device();

    hal->select(true);
    hal->spi_xfer(SX126X_CMD_READ_REGISTER);
    hal->spi_xfer(addr >> 8);
    hal->spi_xfer(addr & 0xff);
    hal->spi_xfer(0xff); // skip status
    result = hal->spi_xfer(0xff);
    hal->select(false);

    return result;
}

static void write_register(uint16_t addr, uint8_t value)
{
    check_device();

    hal->select(true);
    hal->spi_xfer(SX126X_CMD_WRITE_REGISTER);
    hal->spi_xfer(addr >> 8);
    hal->spi_xfer(addr & 0xff);
    hal->spi_xfer(value);
    hal->select(false);
}

static uint8_t read_command(uint8_t cmd, void *buf, uint16_t size)
{
    uint8_t status;

    check_device();

    hal->select(true);
    hal->spi_xfer(cmd);
    status = hal->spi_xfer(0xff);
    for (uint16_t i = 0; i < size; i++)
        ((uint8_t *)buf)[i] = hal->spi_xfer(0xff);
    hal->select(false);

    return status;
}

static void write_command(uint8_t cmd, const void *buf, uint16_t size)
{
    check_device();

    hal->select(true);
    hal->spi_xfer(cmd);
    for (uint16_t i = 0; i < size; i++)
        hal->spi_xfer(((const uint8_t *)buf)[i]);
    hal->select(false);
}

static void write_registers(uint16_t addr, const uint8_t *values, uint16_t count)
{
    check_device();

    hal->select(true);
    hal->spi_xfer(SX126X_CMD_WRITE_REGISTER);
    hal->spi_xfer(addr >> 8);
    hal->spi_xfer(addr & 0xff);
    for (uint16_t i = 0; i < count; i++)
        hal->spi_xfer(values[i]);
    hal->select(false);
}

static void write_buffer(uint8_t offset, const void *buf, uint16_t size)
{
    check_device();

    hal->select(true);
    hal->spi_xfer(SX126X_CMD_WRITE_BUFFER);
    hal->spi_xfer(offset);
    for (uint16_t i = 0; i < size; i++)
        hal->spi_xfer(((const uint8_t *)buf)[i]);
    hal->select(false);
}

static uint8_t read_buffer(uint8_t offset, void *buf, uint16_t size)
{
    uint8_t status;

    check_device();

    hal->select(true);
    hal->spi_xfer(SX126X_CMD_READ_BUFFER);
    hal->spi_xfer(offset);
    status = hal->spi_xfer(0xff);
    for (uint16_t i = 0; i < size; i++)
        ((uint8_t *)buf)[i] = hal->spi_xfer(0xff);
    hal->select(false);

    return status;
}

static void set_packet_params(uint16_t preamb_len, bool crc_on, bool inverted_iq,
    uint8_t len)
{
    // sx1261-2_v1.2.pdf chapter 15.4
    uint8_t reg = read_register(REG_IQ_POL_FIX);
    if (inverted_iq)
        reg &= ~0x04;
    else
        reg |= 0x04;
    write_register(REG_IQ_POL_FIX, reg);

    uint8_t params[6];
    params[0] = preamb_len >> 8; // tx only
    params[1] = preamb_len & 0xff;
    params[2] = 0x0; // Variable length packet (explicit header)
    params[3] = len;
    params[4] = crc_on ? 0x01 : 0x00;
    params[5] = inverted_iq ? 0x01 : 0x00;
    write_command(SX126X_CMD_SET_PACKET_PARAMS, params, sizeof(params));
}

static void set_standby(uint8_t cfg)
{
    write_command(SX126X_CMD_SET_STANDBY, &cfg, sizeof(cfg));
}

static bool sx126x_init(const struct radio_hal *r_hal)
{
    hal = r_hal;

    hal->reset(true);
    hal->delay_us(1000); // >100us

    hal->reset(false);
    hal->delay_us(20000); // >5ms

    if (hal->io_deinit)
        hal->io_deinit();

    is_sleep = true;

    // TODO cfg
    const uint32_t tcxoTimeout = (10 << 6); // 10ms
    uint8_t tcxo[4] = {
        TCXO_VOLTAGE_1_8V,
        (uint8_t)(tcxoTimeout >> 16),
        (uint8_t)(tcxoTimeout >> 8),
        (uint8_t)tcxoTimeout,
    };
    write_command(SX126X_CMD_SET_DIO3_AS_TCXO_CTRL, &tcxo, sizeof(tcxo));
    write_register(REG_XTA_TRIM, 0);

    const uint8_t calib_cfg = 0x7f;
    write_command(SX126X_CMD_CALIBRATE, &calib_cfg, sizeof(calib_cfg));

    const uint8_t mode = 1; // TODO cfg
    write_command(SX126X_CMD_SET_REGULATOR_MODE, &mode, sizeof(mode));

    sx126x_sleep();

    const uint8_t packet_type = PACKET_TYPE_LORA;
    write_command(SX126X_CMD_SET_PACKET_TYPE, &packet_type, sizeof(packet_type));

    uint8_t sync_word[] = {0x34, 0x44}; // TODO public
    write_registers(REG_LORA_SYNC_WORD_MSB, sync_word, sizeof(sync_word));

    const uint16_t mask = IRQ_MASK_TX_DONE | IRQ_MASK_RX_DONE
        | IRQ_MASK_CRC_ERR | IRQ_MASK_TIMEOUT;
    const uint8_t irq[8] = {
        mask >> 8, mask & 0xff,
        mask >> 8, mask & 0xff,
        0x00, 0x00,
        0x00, 0x00,
    };
    write_command(SX126X_CMD_SET_DIO_IRQ_PARAMS, irq, sizeof(irq));

    return true;
}

static void sx126x_sleep()
{
    const uint8_t sleep_conf = SLEEP_WARM_START;
    write_command(SX126X_CMD_SET_SLEEP, &sleep_conf, sizeof(sleep_conf));
    hal->delay_us(1000);
    is_sleep = true;
}

static void sx126x_set_freq(uint32_t freq)
{
    const uint8_t cal_freq[2] = {0xd7, 0xdb}; // TODO only 868
    write_command(SX126X_CMD_CALIBRATE_IMAGE, cal_freq, sizeof(cal_freq));

    uint64_t rf_freq = ((uint64_t)freq << 25) / 32000000UL;
    uint8_t buf[4];
    buf[0] = (rf_freq >> 24) & 0xff;
    buf[1] = (rf_freq >> 16) & 0xff;
    buf[2] = (rf_freq >> 8) & 0xff;
    buf[3] = rf_freq & 0xff;

    write_command(SX126X_CMD_SET_RF_FREQUENCY, buf, sizeof(buf));
}

static bool sx126x_set_power(int8_t power)
{
    bool is_lp = false;
    uint8_t pa_conf[] = {0x00, 0x00, 0x00, 0x01};

    if (is_lp) {
        if (power > 15 || power < -17)
            return false;

        if (power == 15) {
            pa_conf[0] = 0x06;
            power = 14;
        }
        else
            pa_conf[0] = 0x04;

        pa_conf[1] = 0x0;
        pa_conf[2] = SET_PA_CONFIG_DEV_SEL_SX1261;

        write_register(REG_OCP, 0x18); // for wle
    }
    else {
        if (power > 22 || power < -9)
            return false;

        // sx1261-2_v1.2.pdf chapter 15.2
        write_register(REG_TX_CLAMP_CONFIG, read_register(REG_TX_CLAMP_CONFIG) | 0x1E);

        pa_conf[0] = 0x04;
        pa_conf[1] = 0x07;
        pa_conf[2] = SET_PA_CONFIG_DEV_SEL_SX1262;

        write_register(REG_OCP, 0x38); // for wle
    }

    uint8_t tx_params[] = {power, SET_RAMP_200U};
    write_command(SX126X_CMD_SET_PA_CONFIG, pa_conf, sizeof(pa_conf));
    write_command(SX126X_CMD_SET_TX_PARAMS, tx_params, sizeof(tx_params));

    return true;
}

static void sx126x_setup(enum uwan_sf sf, enum uwan_bw bw, enum uwan_cr cr)
{
    uint8_t mod_param[4];

    mod_param[0] = sf_table[sf];
    mod_param[1] = bw_table[bw];
    mod_param[2] = cr_table[cr];
    if (sf >= UWAN_SF_11)
        mod_param[3] = LORA_MOD_PARAM4_LOW_DR_OPTIMIZE_ON;
    else
        mod_param[3] = LORA_MOD_PARAM4_LOW_DR_OPTIMIZE_OFF;
    write_command(SX126X_CMD_SET_MODULATION_PARAMS, mod_param, sizeof(mod_param));

    preamble_length = (sf >= UWAN_SF_10) ? 0x05 : 0x08;
}

static void sx126x_tx(const uint8_t *buf, uint8_t len)
{
    set_packet_params(preamble_length, true, false, len);

    const uint8_t addr[] = {0x0, 0x0};
    write_command(SX126X_CMD_SET_BUFFER_BASE_ADDRESS, addr, sizeof(addr));

    write_buffer(0x0, buf, len);

    if (hal->ant_sw_ctrl)
        hal->ant_sw_ctrl(false);

    uint8_t timeout[] = {0x0, 0x0, 0x0};
    write_command(SX126X_CMD_SET_TX, timeout, sizeof(timeout));
}

static void sx126x_rx(bool continous)
{
    set_packet_params(preamble_length, true, true, 0xff);
    uint8_t symb_num = preamble_length;
    write_command(SX126X_CMD_SET_LORA_SYMB_NUM_TIMEOUT, &symb_num, sizeof(symb_num));

    const uint8_t addr[] = {0x0, 0x0};
    write_command(SX126X_CMD_SET_BUFFER_BASE_ADDRESS, addr, sizeof(addr));

    if (hal->ant_sw_ctrl)
        hal->ant_sw_ctrl(true);

    uint8_t timeout[3];
    if (continous) {
        timeout[0] = 0xff;
        timeout[1] = 0xff;
        timeout[2] = 0xff;
    }
    else {
        uint32_t tm = 3000 << 6; // TODO
        timeout[0] = tm >> 16;
        timeout[1] = tm >> 8;
        timeout[2] = tm;
    }
    write_command(SX126X_CMD_SET_RX, timeout, sizeof(timeout));
}

static uint8_t sx126x_read_fifo(uint8_t *buf, uint8_t buf_size)
{
    uint8_t buf_status[2];

    read_command(SX126X_CMD_GET_RX_BUFFER_STATUS, buf_status, sizeof(buf_status));

    uint8_t actual_len = buf_size > buf_status[0] ? buf_status[0] : buf_size;
    uint8_t offset = buf_status[1];

    read_buffer(offset, buf, actual_len);

    return actual_len;
}

static uint32_t sx126x_rand()
{
    return 0; // TODO
}

static uint8_t sx126x_handle_dio(int dio_num)
{
    uint8_t buf[2];

    read_command(SX126X_CMD_GET_IRQ_STATUS, buf, sizeof(buf));
    write_command(SX126X_CMD_CLEAR_IRQ_STATUS, buf, sizeof(buf));

    uint16_t flags = buf[0] << 8 | buf[1];
    uint8_t result = 0;

    if (flags & IRQ_MASK_TX_DONE) {
        result |= RADIO_IRQF_TX_DONE;
    }

    if (flags & IRQ_MASK_RX_DONE) {
        result |= RADIO_IRQF_RX_DONE;
    }

    if (flags & IRQ_MASK_TIMEOUT) {
        result |= RADIO_IRQF_RX_TIMEOUT;
    }

    // TODO CRC Error

    return result;
}
