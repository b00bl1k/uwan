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

#ifndef __UWAN_DEVICE_SX126X_H__
#define __UWAN_DEVICE_SX126X_H__

#include <uwan/device/radio.h>

/* Commands Selecting the Operating Modes of the Radio */
#define SX126X_CMD_SET_SLEEP                    0x84
#define SX126X_CMD_SET_STANDBY                  0x80
#define SX126X_CMD_SET_FS                       0xc1
#define SX126X_CMD_SET_TX                       0x83
#define SX126X_CMD_SET_RX                       0x82
#define SX126X_CMD_STOP_TIMER_ON_PREAMBLE       0x9f
#define SX126X_CMD_SET_RX_DUTY_CYCE             0x94
#define SX126X_CMD_SET_CAD                      0xc5
#define SX126X_CMD_SET_TX_CONTINUOUS_WAVE       0xd1
#define SX126X_CMD_SET_TX_INFINITE_PREAMBLE     0xd2
#define SX126X_CMD_SET_REGULATOR_MODE           0x96
#define SX126X_CMD_CALIBRATE                    0x89
#define SX126X_CMD_CALIBRATE_IMAGE              0x98
#define SX126X_CMD_SET_PA_CONFIG                0x95
#define SX126X_CMD_SET_RX_TX_FALLBACK_MODE      0x93
/* Register and Buffer Access Commands */
#define SX126X_CMD_WRITE_REGISTER               0x0d
#define SX126X_CMD_READ_REGISTER                0x1d
#define SX126X_CMD_WRITE_BUFFER                 0x0e
#define SX126X_CMD_READ_BUFFER                  0x1d
/* DIO and IRQ Control */
#define SX126X_CMD_SET_DIO_IRQ_PARAMS           0x08
#define SX126X_CMD_GET_IRQ_STATUS               0x12
#define SX126X_CMD_CLEAR_IRQ_STATUS             0x02
#define SX126X_CMD_SET_DIO2_AS_RF_SWITCH_CTRL   0x9d
#define SX126X_CMD_SET_DIO3_AS_TCXO_CTRL        0x97
/* RF, Modulation and Packet Commands */
#define SX126X_CMD_SET_RF_FREQUENCY             0x86
#define SX126X_CMD_SET_PACKET_TYPE              0x8a
#define SX126X_CMD_GET_PACKET_TYPE              0x11
#define SX126X_CMD_SET_TX_PARAMS                0x8e
#define SX126X_CMD_SET_MODULATION_PARAMS        0x8b
/* Commands Controlling the RF and Packets Settings */
#define SX126X_CMD_SET_PACKET_PARAMS            0x8c
#define SX126X_CMD_SET_CAD_PARAMS               0x88
#define SX126X_CMD_SET_BUFFER_BASE_ADDRESS      0x8f
#define SX126X_CMD_SET_LORA_SYMB_NUM_TIMEOUT    0xa0
/* Commands Returning the Radio Status */
#define SX126X_CMD_GET_STATUS                   0xc0
#define SX126X_CMD_GET_RSSI_INST                0x15
#define SX126X_CMD_GET_RX_BUFFER_STATUS         0x13
#define SX126X_CMD_GET_PACKET_STATUS            0x14
#define SX126X_CMD_GET_DEVICE_ERRORS            0x17
#define SX126X_CMD_CLEAR_DEVICE_ERRORS          0x07
#define SX126X_CMD_GET_STATS                    0x10
#define SX126X_CMD_RESET_STATS                  0x00

extern const struct radio_dev sx126x_dev;

#endif /* ~__UWAN_DEVICE_SX126X_H__ */
