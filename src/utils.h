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

#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>

#define BITS_PER_BYTE 8
#define BYTES_FOR_BITS(bits) ( \
    (bits % BITS_PER_BYTE) ? \
    (bits / BITS_PER_BYTE + 1) : \
    (bits / BITS_PER_BYTE))

#define BIT_IS_SET(x, i) (x[i / BITS_PER_BYTE] & (1 << (i % BITS_PER_BYTE)))
#define BIT_SET(x, i) x[i / BITS_PER_BYTE] |= (1 << (i % BITS_PER_BYTE))
#define BIT_CLEAR(x, i) x[i / BITS_PER_BYTE] &= ~(1 << (i % BITS_PER_BYTE))

void utils_random_init(uint32_t seed);
uint32_t utils_get_random(uint32_t max);

#endif /* ~__UTILS_H__ */
