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
static bool sx126x_init(const struct radio_hal *r_hal, const void *opts);
static void sx126x_sleep(void);
static void sx126x_set_freq(uint32_t freq);
static bool sx126x_set_power(int8_t power);
static void sx126x_set_public_network(bool is_public);
static void sx126x_setup(const struct uwan_packet_params *params);
static void sx126x_tx(const uint8_t *buf, uint8_t len);
static void sx126x_rx(uint8_t len, uint16_t symb_timeout, uint32_t timeout);
static void sx126x_read_packet(struct uwan_dl_packet *pkt);
static uint32_t sx126x_rand(void);
static void sx126x_irq_handler(void);
static void sx126x_set_evt_handler(void (*handler)(uint8_t evt_mask));

/* private pointer to actual HAL */
static const struct radio_hal *hal;

static const struct sx126x_opts *dev_opts;

static bool is_sleep;
static void (*user_evt_handler)(uint8_t evt_mask);
static struct uwan_packet_params pkt_params;

/* export radio driver */
const struct radio_dev sx126x_dev = {
    .init = sx126x_init,
    .sleep = sx126x_sleep,
    .set_frequency = sx126x_set_freq,
    .set_power = sx126x_set_power,
    .set_public_network = sx126x_set_public_network,
    .setup = sx126x_setup,
    .tx = sx126x_tx,
    .rx = sx126x_rx,
    .read_packet = sx126x_read_packet,
    .rand = sx126x_rand,
    .irq_handler = sx126x_irq_handler,
    .set_evt_handler = sx126x_set_evt_handler,
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
    check_device();

    hal->select(true);
    hal->spi_xfer(SX126X_CMD_READ_REGISTER);
    hal->spi_xfer(addr >> 8);
    hal->spi_xfer(addr & 0xff);
    hal->spi_xfer(0xff); // skip status
    uint8_t result = hal->spi_xfer(0xff);
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
    check_device();

    hal->select(true);
    hal->spi_xfer(cmd);
    uint8_t status = hal->spi_xfer(0xff);
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

static uint8_t read_registers(uint16_t addr, uint8_t *values, uint16_t count)
{
    check_device();

    hal->select(true);
    hal->spi_xfer(SX126X_CMD_READ_REGISTER);
    hal->spi_xfer(addr >> 8);
    hal->spi_xfer(addr & 0xff);
    uint8_t status = hal->spi_xfer(0xff);
    for (uint16_t i = 0; i < count; i++)
        values[i] = hal->spi_xfer(0xff);
    hal->select(false);

    return status;
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
    check_device();

    hal->select(true);
    hal->spi_xfer(SX126X_CMD_READ_BUFFER);
    hal->spi_xfer(offset);
    uint8_t status = hal->spi_xfer(0xff);
    for (uint16_t i = 0; i < size; i++)
        ((uint8_t *)buf)[i] = hal->spi_xfer(0xff);
    hal->select(false);

    return status;
}

static void setup_tcxo(uint8_t voltage, uint32_t timeout)
{
    timeout = (timeout << 6);
    uint8_t tcxo[4] = {
        voltage,
        (uint8_t)(timeout >> 16),
        (uint8_t)(timeout >> 8),
        (uint8_t)timeout,
    };
    write_command(SX126X_CMD_SET_DIO3_AS_TCXO_CTRL, &tcxo, sizeof(tcxo));
    write_register(REG_XTA_TRIM, 0);
}

static void set_packet_params(uint16_t preamb_len, bool crc_on, bool inverted_iq,
    bool implicit_header, uint8_t len)
{
    // sx1261-2_v1.2.pdf chapter 15.4
    uint8_t reg = read_register(REG_IQ_POL_FIX);
    if (inverted_iq)
        reg &= ~0x04;
    else
        reg |= 0x04;
    write_register(REG_IQ_POL_FIX, reg);

    uint8_t params[6];
    params[0] = preamb_len >> 8;
    params[1] = preamb_len & 0xff;
    params[2] = implicit_header ? 0x01 : 0x0;
    params[3] = len;
    params[4] = crc_on ? 0x01 : 0x00;
    params[5] = inverted_iq ? 0x01 : 0x00;
    write_command(SX126X_CMD_SET_PACKET_PARAMS, params, sizeof(params));
}

static bool sx126x_init(const struct radio_hal *r_hal, const void *opts)
{
    hal = r_hal;
    dev_opts = (const struct sx126x_opts *)opts;

    if (!hal || !dev_opts)
        return false;

    hal->reset(true);
    hal->delay_us(1000); // >100us

    hal->reset(false);
    hal->delay_us(20000); // >5ms

    if (hal->io_deinit)
        hal->io_deinit();

    is_sleep = true;

    uint16_t op_clear = 0;
    write_command(SX126X_CMD_CLEAR_DEVICE_ERRORS, &op_clear, sizeof(op_clear));

    if (dev_opts->use_tcxo)
        setup_tcxo(dev_opts->tcxo_voltage, dev_opts->tcxo_timeout);

    const uint8_t calib_cfg = 0x7f;
    write_command(SX126X_CMD_CALIBRATE, &calib_cfg, sizeof(calib_cfg));

    uint16_t op_error;
    read_command(SX126X_CMD_GET_DEVICE_ERRORS, &op_error, sizeof(op_error));
    if (op_error)
        return false;

    const uint8_t mode = dev_opts->use_dcdc ? 1 : 0;
    write_command(SX126X_CMD_SET_REGULATOR_MODE, &mode, sizeof(mode));

    sx126x_sleep();

    const uint8_t packet_type = PACKET_TYPE_LORA;
    write_command(SX126X_CMD_SET_PACKET_TYPE, &packet_type, sizeof(packet_type));

    sx126x_set_public_network(true);

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

    if (hal->io_deinit)
        hal->io_deinit();

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
    uint8_t pa_conf[] = {0x00, 0x00, 0x00, 0x01};

    if (dev_opts->is_hp == false) {
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

    uint8_t tx_params[] = {power, SET_RAMP_40U};
    write_command(SX126X_CMD_SET_PA_CONFIG, pa_conf, sizeof(pa_conf));
    write_command(SX126X_CMD_SET_TX_PARAMS, tx_params, sizeof(tx_params));

    return true;
}

static void sx126x_set_public_network(bool is_public)
{
    uint8_t sync_word[2];

    if (is_public) {
        sync_word[0] = LORAWAN_PUBLIC_SYNC_WORD_MSB;
        sync_word[1] = LORAWAN_PUBLIC_SYNC_WORD_LSB;
    }
    else {
        sync_word[0] = LORAWAN_PRIVATE_SYNC_WORD_MSB;
        sync_word[1] = LORAWAN_PRIVATE_SYNC_WORD_LSB;
    }

    write_registers(REG_LORA_SYNC_WORD_MSB, sync_word, sizeof(sync_word));
}

static void sx126x_setup(const struct uwan_packet_params *params)
{
    uint8_t mod_param[4];

    mod_param[0] = sf_table[params->sf];
    mod_param[1] = bw_table[params->bw];
    mod_param[2] = cr_table[params->cr];
    if (params->sf >= UWAN_SF_11)
        mod_param[3] = LORA_MOD_PARAM4_LOW_DR_OPTIMIZE_ON;
    else
        mod_param[3] = LORA_MOD_PARAM4_LOW_DR_OPTIMIZE_OFF;
    write_command(SX126X_CMD_SET_MODULATION_PARAMS, mod_param, sizeof(mod_param));

    pkt_params = *params;

    if (hal->io_init)
        hal->io_init();
}

static void sx126x_tx(const uint8_t *buf, uint8_t len)
{
    set_packet_params(pkt_params.preamble_len, pkt_params.crc_on,
        pkt_params.inverted_iq, pkt_params.implicit_header, len);

    const uint8_t addr[] = {0x0, 0x0};
    write_command(SX126X_CMD_SET_BUFFER_BASE_ADDRESS, addr, sizeof(addr));

    write_buffer(0x0, buf, len);

    if (hal->ant_sw_ctrl)
        hal->ant_sw_ctrl(false);

    uint8_t timeout[] = {0x0, 0x0, 0x0};
    write_command(SX126X_CMD_SET_TX, timeout, sizeof(timeout));
}

static void sx126x_rx(uint8_t len, uint16_t symb_timeout, uint32_t timeout)
{
    set_packet_params(pkt_params.preamble_len, pkt_params.crc_on,
        pkt_params.inverted_iq, pkt_params.implicit_header, len);

    write_command(SX126X_CMD_SET_LORA_SYMB_NUM_TIMEOUT, &symb_timeout, 1);

    const uint8_t addr[] = {0x0, 0x0};
    write_command(SX126X_CMD_SET_BUFFER_BASE_ADDRESS, addr, sizeof(addr));

    if (hal->ant_sw_ctrl)
        hal->ant_sw_ctrl(true);

    uint8_t tmo[3];
    if (timeout != UWAN_RX_NO_TIMEOUT && timeout != UWAN_RX_INFINITE) {
        timeout <<= 6;
    }
    tmo[0] = timeout >> 16;
    tmo[1] = timeout >> 8;
    tmo[2] = timeout;
    write_command(SX126X_CMD_SET_RX, tmo, sizeof(tmo));
}

static void sx126x_read_packet(struct uwan_dl_packet *pkt)
{
    uint8_t buf_status[2];
    read_command(SX126X_CMD_GET_RX_BUFFER_STATUS, buf_status, sizeof(buf_status));

    uint16_t actual_len = pkt->size > buf_status[0] ? buf_status[0] : pkt->size;
    uint8_t offset = buf_status[1];
    read_buffer(offset, pkt->data, actual_len);

    uint8_t pkt_status[3];
    read_command(SX126X_CMD_GET_PACKET_STATUS, pkt_status, sizeof(pkt_status));

    pkt->size = actual_len;
    pkt->rssi = -pkt_status[0] / 2;
    pkt->snr = (int8_t)pkt_status[1] / 4;
}

static uint32_t sx126x_rand()
{
    uint32_t result;

    sx126x_rx(0xff, 0x00, UWAN_RX_INFINITE);
    read_registers(REG_RANDOM_NUMBER_GEN_0, (uint8_t *)&result, sizeof(result));
    sx126x_sleep();

    return result;
}

static void sx126x_irq_handler()
{
    uint8_t buf[2];

    read_command(SX126X_CMD_GET_IRQ_STATUS, buf, sizeof(buf));
    write_command(SX126X_CMD_CLEAR_IRQ_STATUS, buf, sizeof(buf));

    uint16_t flags = buf[0] << 8 | buf[1];
    uint8_t result = 0;

    if (flags & IRQ_MASK_TX_DONE)
        result |= RADIO_IRQF_TX_DONE;

    if (flags & IRQ_MASK_RX_DONE)
        result |= RADIO_IRQF_RX_DONE;

    if (flags & IRQ_MASK_TIMEOUT)
        result |= RADIO_IRQF_RX_TIMEOUT;

    if (flags & IRQ_MASK_CRC_ERR)
        result |= RADIO_IRQF_CRC_ERROR;

    if (user_evt_handler)
        user_evt_handler(result);
}

static void sx126x_set_evt_handler(void (*handler)(uint8_t evt_mask))
{
    user_evt_handler = handler;
}
