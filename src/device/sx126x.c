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

#include <uwan/device/sx126x.h>

/* export funcs for radio driver struct */
static bool sx126x_init(const struct radio_hal *r_hal);

/* private pointer to actual HAL */
static const struct radio_hal *hal;

/* export radio driver */
const struct radio_dev sx126x_dev = {
    .init = sx126x_init,
};

static uint8_t read_register(uint16_t addr)
{
    uint8_t status;
    uint8_t result;

    hal->select(true);
    hal->spi_xfer(SX126X_CMD_READ_REGISTER);
    hal->spi_xfer(addr >> 8);
    hal->spi_xfer(addr & 0xff);
    hal->spi_xfer(0xff);
    result = hal->spi_xfer(0xff);
    hal->select(false);

    return result;
}

static bool sx126x_init(const struct radio_hal *r_hal)
{
    hal = r_hal;

    hal->reset(true);
    hal->delay_us(150); // >100us

    hal->reset(false);
    hal->delay_us(10000); // >5ms

    if (hal->io_deinit)
        hal->io_deinit();

    return true;
}
