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
#include <stddef.h>
#include <uwan/device/sx127x.h>

#define NA 0x0

#define LORA_PAGE_START 0x0d
#define LORA_PAGE_END 0x3f

static struct sx127x_reg *find_reg(uint8_t addr);
static uint8_t *get_ptr_to_reg_value(uint8_t addr);

struct sx127x_reg {
    uint8_t addr;
    uint8_t fsk_value;
    uint8_t lora_value;
};

struct {
    uint8_t cmd;
    uint8_t pos;
    bool is_selected;
} sx127x_state;

struct sx127x_reg sx127x_regs[] = {
    /* Common Registers */
    {SX127X_REG_FIFO, FIFO_RESET_VALUE, 0},
    {SX127X_REG_OP_MODE, OP_MODE_RESET_VALUE, 0},
    {SX127X_REG_FRF_MSB, FRF_MSB_RESET_VALUE, 0},
    {SX127X_REG_FRF_MID, FRF_MID_RESET_VALUE, 0},
    {SX127X_REG_FRF_LSB, FRF_LSB_RESET_VALUE, 0},
    {SX127X_REG_PA_CONFIG, PA_CONFIG_RESET_VALUE, 0},
    {SX127X_REG_PA_RAMP, PA_RAMP_RESET_VALUE, 0},
    {SX127X_REG_LNA, LNA_RESET_VALUE, 0},
    {SX127X_REG_DIO_MAPPING1, DIO_MAPPING1_RESET_VALUE, 0},
    {SX127X_REG_DIO_MAPPING2, DIO_MAPPING2_RESET_VALUE, 0},
    {SX127X_REG_VERSION, VERSION_RESET_VALUE, 0},

    {SX127X_REG_FSK_RX_CONFIG, RX_CONFIG_RESET_VALUE, FIFO_ADDR_PTR_RESET_VALUE},
    {SX127X_REG_FSK_RSSI_CONFIG, RSSI_CONFIG_RESET_VALUE, FIFO_TX_BASE_ADDR_RESET_VALUE},
    {SX127X_REG_FSK_RSSI_COLLISION, RSSI_COLLISION_RESET_VALUE, FIFO_RX_BASE_ADDR_RESET_VALUE},
    {SX127X_REG_FSK_RSSI_THRESH, RSSI_THRESH_RESET_VALUE, NA},
    {SX127X_REG_FSK_RSSI_VALUE, NA, IRQ_FLAGS_MASK_RESET_VALUE},
    {SX127X_REG_FSK_RX_BW, RX_BW_RESET_VALUE, IRQ_FLAGS_RESET_VALUE},
    {SX127X_REG_FSK_AFC_BW, AFC_BW_RESET_VALUE, NA},
    {SX127X_REG_FSK_FEI_MSB, FEI_MSB_RESET_VALUE, MODEM_CONFIG1_RESET_VALUE},
    {SX127X_REG_FSK_FEI_LSB, FEI_LSB_RESET_VALUE, MODEM_CONFIG2_RESET_VALUE},
    {SX127X_REG_FSK_PREAMBLE_DETECT, PREAMBLE_DETECT_RESET_VALUE, SYMB_TIMEOUT_LSB_RESET_VALUE},
    {SX127X_REG_FSK_RX_TIMEOUT1, RX_TIMEOUT1_RESET_VALUE, LR_PREAMBLE_MSB_RESET_VALUE},
    {SX127X_REG_FSK_RX_TIMEOUT2, RX_TIMEOUT2_RESET_VALUE, LR_PREAMBLE_LSB_RESET_VALUE},
    {SX127X_REG_FSK_RX_TIMEOUT3, RX_TIMEOUT3_RESET_VALUE, LR_PAYLOAD_LENGTH_RESET_VALUE},
    {SX127X_REG_FSK_RX_DELAY, RX_DELAY_RESET_VALUE, LR_MAX_PAYLOAD_LENGTH_RESET_VALUE},
    {SX127X_REG_FSK_PREAMBLE_LSB, PREAMBLE_LSB_RESET_VALUE, MODEM_CONFIG3_RESET_VALUE},
    {SX127X_REG_FSK_SYNC_VALUE5, SYNC_VALUE5_RESET_VALUE, NA},
    {SX127X_REG_FSK_NODE_ADRS, NODE_ADRS_RESET_VALUE, INVERT_IQ_RESET_VALUE},
    {SX127X_REG_FSK_TIMER1_COEF, TIMER1_COEF_RESET_VALUE, SYNC_WORD_RESET_VALUE},
    {SX127X_REG_FSK_IMAGE_CAL, IMAGE_CAL_RESET_VALUE, INVERT_IQ2_RESET_VALUE},
};

uint8_t hal_spi_xfer(uint8_t data)
{
    uint8_t return_val = 0xff;

    if (!sx127x_state.is_selected)
        return return_val;

    if (sx127x_state.pos == 0) {
        sx127x_state.cmd = data;
    }
    else {
        bool is_write = (sx127x_state.cmd & SX127X_WNR) != 0;

        uint8_t reg_addr = sx127x_state.cmd & ~SX127X_WNR;
        if (reg_addr != SX127X_REG_FIFO)
            reg_addr += sx127x_state.pos - 1;

        uint8_t *reg_ptr = get_ptr_to_reg_value(reg_addr);
        if (is_write)
            *reg_ptr = data;
        else
            return_val = *reg_ptr;
    }

    sx127x_state.pos++;

    return return_val;
}

void hal_select(bool enable)
{
    sx127x_state.is_selected = enable;
    if (enable)
        sx127x_state.pos = 0;
}

void hal_radio_reset(bool en)
{
}

void hal_delay_us(uint32_t us)
{
}

const struct radio_hal my_hal = {
    .spi_xfer = hal_spi_xfer,
    .reset = hal_radio_reset,
    .select = hal_select,
    .delay_us = hal_delay_us,
};

static struct sx127x_reg *find_reg(uint8_t addr)
{
    for (int i = 0; i < sizeof(sx127x_regs) / sizeof(sx127x_regs[0]); i++)
        if (sx127x_regs[i].addr == addr)
            return &sx127x_regs[i];

    return NULL;
}

static uint8_t *get_ptr_to_reg_value(uint8_t addr)
{
    struct sx127x_reg *reg = find_reg(addr);
    struct sx127x_reg *reg_op_mode = find_reg(SX127X_REG_OP_MODE);

    assert(reg);
    assert(reg_op_mode);

    bool is_lora_reg = (reg_op_mode->fsk_value & OP_MODE_LONG_RANGE_MODE_ON)
        && ((reg_op_mode->fsk_value & OP_MODE_ACCESS_SHARED_REG_FSK) == 0)
        && (addr >= LORA_PAGE_START)
        && (addr <= LORA_PAGE_END);

    if (is_lora_reg)
        return &reg->lora_value;

    return &reg->fsk_value;
}

int main()
{
    uint8_t payload[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    assert(sx127x_dev.init(&my_hal));

    sx127x_dev.set_frequency(868900000);
    sx127x_dev.set_power(14);
    sx127x_dev.setup(UWAN_SF_12, UWAN_BW_125, UWAN_CR_4_5);
    sx127x_dev.tx(payload, sizeof(payload));

    // see Semtech AN1200.24 p2.1
    assert((get_ptr_to_reg_value(SX127X_REG_OP_MODE)[0] & 0x87) == 0x83);
    assert((get_ptr_to_reg_value(SX127X_REG_PA_RAMP)[0] & 0x0f) == 0x08);
    assert(get_ptr_to_reg_value(SX127X_REG_LR_MODEM_CONFIG1)[0] == 0x72);
    assert((get_ptr_to_reg_value(SX127X_REG_LR_MODEM_CONFIG2)[0] & 0xf4) == 0xc4);
    assert((get_ptr_to_reg_value(SX127X_REG_LR_MODEM_CONFIG3)[0] & 0x08) == 0x08);
    assert(get_ptr_to_reg_value(SX127X_REG_LR_SYNC_WORD)[0] == 0x34);
    assert(get_ptr_to_reg_value(SX127X_REG_LR_INVERT_IQ)[0] == 0x27);
    assert(get_ptr_to_reg_value(SX127X_REG_LR_INVERT_IQ2)[0] == 0x1d);
    assert(get_ptr_to_reg_value(SX127X_REG_LR_PAYLOAD_LENGTH)[0] == sizeof(payload));

    return 0;
}
