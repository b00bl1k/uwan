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

#include <stdlib.h>
#include "utils.h"

#define UNIX_GPS_EPOCH_OFFSET 315964800
#define UNIX_LEAP_SECONDS 18

void utils_random_init(uint32_t seed)
{
    srand(seed);
}

uint32_t utils_get_random(uint32_t max)
{
    return rand() % max;
}

uint8_t utils_checksum(const void *src, size_t size)
{
    uint8_t cs = 0xff;
    const uint8_t *buf = src;
    for (; size > 0; size--)
        cs += *buf++;
    return cs;
}

uint32_t utils_unix_to_gps(uint32_t timestamp)
{
    uint32_t gps_time = timestamp - UNIX_GPS_EPOCH_OFFSET;
    gps_time += UNIX_LEAP_SECONDS;
    return gps_time;
}

uint32_t utils_gps_to_unix(uint32_t timestamp)
{
    uint32_t unix_time = timestamp + UNIX_GPS_EPOCH_OFFSET;
    unix_time -= UNIX_LEAP_SECONDS;
    return unix_time;
}
