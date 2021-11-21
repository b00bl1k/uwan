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

#include <uwan/device/radio.h>

#define WNR 0x80

void radio_write_reg(const struct radio_hal *hal, uint8_t addr, uint8_t data)
{
    hal->select(true);
    hal->spi_xfer(addr | WNR);
    hal->spi_xfer(data);
    hal->select(false);
}

uint8_t radio_read_reg(const struct radio_hal *hal, uint8_t addr)
{
    uint8_t data;

    hal->select(true);
    hal->spi_xfer(addr);
    data = hal->spi_xfer(0x0);
    hal->select(false);

    return data;
}

void radio_write_array(const struct radio_hal *hal, uint8_t addr,
    const uint8_t *buf, uint8_t size)
{
    hal->select(true);
    hal->spi_xfer(addr | WNR);
    for (int i = 0; i < size; i++)
        hal->spi_xfer(buf[i]);
    hal->select(false);
}

void radio_read_array(const struct radio_hal *hal, uint8_t addr, uint8_t *buf,
    uint8_t size)
{
    hal->select(true);
    hal->spi_xfer(addr);
    for (int i = 0; i < size; i++)
        buf[i] = hal->spi_xfer(0x0);
    hal->select(false);
}
