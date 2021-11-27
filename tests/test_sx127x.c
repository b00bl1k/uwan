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
#include <uwan/device/sx127x.h>

uint8_t sx_registers[] = {
    0x00,
    0x01,
    0x1a,
    0x0b,
    0x00,
    0x52,
    0x6c,
    0x80,
    0x00,
    0x4f,
    0x09,
    0x2b,
    0x20,
    0x08,
    0x02,
    0x0a,
    0xff, // 0x10
    0x00, // n/a
    0x15,
    0x0b,
    0x28,
    0x0c,
    0x12,
    0x47,
    0x32,
    0x3e,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x40,
    0x00, // 0x20
    0x00,
    0x00,
    0x00,
    0x05,
    0x00,
    0x03,
    0x93,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x90, // 0x30
    0x40,
    0x40,
    0x00,
    0x00,
    0x0f,
    0x00,
    0x00,
    0x00,
    0xf5,
    0x20,
    0x82,
    0x00, // reg temp
    0x02,
    0x80,
    0x40,
    0x00, // 0x40
    0x00,
    0x12,
    0x00, // ---
    0x2d,
    // ...
};

void hal_radio_reset(bool en)
{
}

void hal_delay_us(uint32_t us)
{
}

const struct radio_hal my_hal = {
    .reset = hal_radio_reset,
    .delay_us = hal_delay_us,
};

void radio_write_reg(const struct radio_hal *hal, uint8_t addr, uint8_t data)
{
    if (addr < sizeof(sx_registers))
        sx_registers[addr] = data;
}

uint8_t radio_read_reg(const struct radio_hal *hal, uint8_t addr)
{
    if (addr < sizeof(sx_registers))
        return sx_registers[addr];

    return 0xff;
}

void radio_write_array(const struct radio_hal *hal, uint8_t addr,
    const uint8_t *buf, uint8_t size)
{

}

void radio_read_array(const struct radio_hal *hal, uint8_t addr, uint8_t *buf,
    uint8_t size)
{

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
    assert((sx_registers[SX127X_REG_OP_MODE] & 0x87) == 0x83);
    assert((sx_registers[SX127X_REG_PA_RAMP] & 0x0f) == 0x08);
    assert(sx_registers[SX127X_REG_LR_MODEM_CONFIG1] == 0x72);
    assert((sx_registers[SX127X_REG_LR_MODEM_CONFIG2] & 0xf4) == 0xc4);
    assert((sx_registers[SX127X_REG_LR_MODEM_CONFIG3] & 0x08) == 0x08);
    assert(sx_registers[SX127X_REG_LR_SYNC_WORD] == 0x34);
    assert(sx_registers[SX127X_REG_LR_INVERT_IQ] == 0x27);
    assert(sx_registers[SX127X_REG_LR_INVERT_IQ2] == 0x1d);
    assert(sx_registers[SX127X_REG_LR_PAYLOAD_LENGTH] == sizeof(payload));

    return 0;
}
