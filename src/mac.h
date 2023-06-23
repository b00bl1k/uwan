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

#ifndef __MAC_H__
#define __MAC_H__

#include <stdbool.h>
#include <stdint.h>

#define CID_LINK_CHECK 0x02
#define CID_LINK_ADR 0x03
#define CID_DUTY_CYCLE 0x04
#define CID_RX_PARAM_SETUP 0x05
#define CID_DEV_STATUS 0x06
#define CID_NEW_CHANNEL 0x07
#define CID_RX_TIMING_SETUP 0x08
#define CID_TX_PARAM_SETUP 0x09
#define CID_DI_CHANNEL 0x0A
#define CID_DEVICE_TIME 0x0D

void mac_handle_commands(const uint8_t *buf, uint8_t len);

bool mac_enqueue_ans(uint8_t cid, const uint8_t *data, uint8_t size);

uint8_t mac_get_payload_size(void);

uint8_t mac_get_payload(uint8_t *buf, uint8_t buf_size);

#endif
