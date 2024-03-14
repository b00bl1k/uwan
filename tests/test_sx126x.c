/**
 * MIT License
 *
 * Copyright (c) 2023 Alexey Ryabov
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
#include <uwan/device/sx126x.h>

#define OPCODE_LIST_SIZE 64

uint8_t radio_opcodes[OPCODE_LIST_SIZE];
unsigned int radio_opcodes_count;
struct {
    uint8_t cmd;
    uint8_t pos;
    bool is_selected;
    bool is_busy;
} sx126x_state;

uint16_t dev_errors;

uint8_t hal_spi_xfer(uint8_t data)
{
    uint8_t return_val = 0xff;

    if (sx126x_state.pos == 0) {
        assert(radio_opcodes_count < sizeof(radio_opcodes));
        radio_opcodes[radio_opcodes_count++] = data;
        sx126x_state.cmd = data;
    }
    else {
        switch (sx126x_state.cmd) {
        case SX126X_CMD_GET_DEVICE_ERRORS:
            if (sx126x_state.pos == 2)
                return_val = dev_errors >> 8;
            else if (sx126x_state.pos == 3)
                return_val = dev_errors & 0xff;
            break;
        }
    }

    sx126x_state.pos++;

    return return_val;
}

void hal_select(bool enable)
{
    sx126x_state.is_selected = enable;
    if (enable)
        sx126x_state.pos = 0;
}

void hal_radio_reset(bool en)
{
}

void hal_delay_us(uint32_t us)
{
}

bool hal_is_busy()
{
    return sx126x_state.is_busy;
}

const struct radio_hal my_hal = {
    .spi_xfer = hal_spi_xfer,
    .reset = hal_radio_reset,
    .select = hal_select,
    .is_busy = hal_is_busy,
    .delay_us = hal_delay_us,
};

unsigned int get_opcode_pos(uint8_t opcode)
{
    for (unsigned int pos = 0; pos < sizeof(radio_opcodes); pos++)
        if (radio_opcodes[pos] == opcode)
            return pos;

    assert(false);
    return 0;
}

const struct sx126x_opts opts = {
    .is_hp = true,
    .use_dcdc = true,
    .use_tcxo = true,
    .tcxo_timeout = 10,
    .tcxo_voltage = TCXO_VOLTAGE_1_8V,
};

int main()
{
    uint8_t payload[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    struct uwan_packet_params pkt_params;

    dev_errors = 0x0;
    assert(sx126x_dev.init(&my_hal, &opts));

    sx126x_dev.set_frequency(868900000);
    sx126x_dev.set_power(14);

    pkt_params.sf = UWAN_SF_12;
    pkt_params.bw = UWAN_BW_125;
    pkt_params.cr = UWAN_CR_4_5;
    pkt_params.preamble_len = 8;
    pkt_params.crc_on = true;
    pkt_params.inverted_iq = false;
    pkt_params.implicit_header = false;
    sx126x_dev.setup(&pkt_params);
    sx126x_dev.tx(payload, sizeof(payload));

    // Check for issuing commands in the right order
    // sx1261-2_v1.2.pdf chapter 14.4
    assert(get_opcode_pos(SX126X_CMD_SET_PACKET_TYPE) < get_opcode_pos(SX126X_CMD_SET_MODULATION_PARAMS));
    assert(get_opcode_pos(SX126X_CMD_SET_MODULATION_PARAMS) < get_opcode_pos(SX126X_CMD_SET_PACKET_PARAMS));
}
