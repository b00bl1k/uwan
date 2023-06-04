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

#ifndef __UWAN_DEVICE_SX126X_H__
#define __UWAN_DEVICE_SX126X_H__

#include <uwan/stack.h>

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
#define SX126X_CMD_READ_BUFFER                  0x1e
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

#define SLEEP_WAKE_UP_ON_RTC                    0x01
#define SLEEP_WARM_START                        0x04

#define STANDBY_CFG_RC                          0x00
#define STANDBY_CFG_XOSC                        0x01

/* PacketType Definition */
#define PACKET_TYPE_GFSK                        0x0
#define PACKET_TYPE_LORA                        0x1

/* RampTime Definition */
#define SET_RAMP_10U                            0x00
#define SET_RAMP_20U                            0x01
#define SET_RAMP_40U                            0x02
#define SET_RAMP_80U                            0x03
#define SET_RAMP_200U                           0x04
#define SET_RAMP_800U                           0x05
#define SET_RAMP_1700U                          0x06
#define SET_RAMP_3400U                          0x07

#define SET_PA_CONFIG_DEV_SEL_SX1262            0x0
#define SET_PA_CONFIG_DEV_SEL_SX1261            0x1

/* LoRa® ModParam1 - SF */
#define LORA_MOD_PARAM1_SF5                     0x05
#define LORA_MOD_PARAM1_SF6                     0x06
#define LORA_MOD_PARAM1_SF7                     0x07
#define LORA_MOD_PARAM1_SF8                     0x08
#define LORA_MOD_PARAM1_SF9                     0x09
#define LORA_MOD_PARAM1_SF10                    0x0a
#define LORA_MOD_PARAM1_SF11                    0x0b
#define LORA_MOD_PARAM1_SF12                    0x0c

/* LoRa® ModParam2 - BW */
#define LORA_MOD_PARAM1_BW_7                    0x00
#define LORA_MOD_PARAM2_BW_10                   0x08
#define LORA_MOD_PARAM2_BW_15                   0x01
#define LORA_MOD_PARAM2_BW_20                   0x09
#define LORA_MOD_PARAM2_BW_31                   0x02
#define LORA_MOD_PARAM2_BW_41                   0x0A
#define LORA_MOD_PARAM2_BW_62                   0x03
#define LORA_MOD_PARAM2_BW_125                  0x04
#define LORA_MOD_PARAM2_BW_250                  0x05
#define LORA_MOD_PARAM2_BW_500                  0x06

/* LoRa® ModParam3 - CR */
#define LORA_MOD_PARAM3_CR_4_5                  0x01
#define LORA_MOD_PARAM3_CR_4_6                  0x02
#define LORA_MOD_PARAM3_CR_4_7                  0x03
#define LORA_MOD_PARAM3_CR_4_8                  0x04

/* LoRa® ModParam4 - LowDataRateOptimize */
#define LORA_MOD_PARAM4_LOW_DR_OPTIMIZE_OFF     0x00
#define LORA_MOD_PARAM4_LOW_DR_OPTIMIZE_ON      0x01

/* List of Registers */
#define REG_LORA_SYNC_WORD_MSB                  0x0740
#define REG_LORA_SYNC_WORD_LSB                  0x0741
#define REG_OCP                                 0x08E7
#define REG_XTA_TRIM                            0x0911
#define REG_XTB_TRIM                            0x0912
/* Workaround registers sx1261-2_v1.2.pdf chapter 15 */
#define REG_IQ_POL_FIX                          0x0736
#define REG_TX_CLAMP_CONFIG                     0x08D8
#define REG_TIMER_CTRL                          0x0902 // TODO 0x0920 ?
#define REG_TIMER_EVENT                         0x0944

/* tcxoVoltage Configuration Definition */
#define TCXO_VOLTAGE_1_6V                       0x00
#define TCXO_VOLTAGE_1_7V                       0x01
#define TCXO_VOLTAGE_1_8V                       0x02
#define TCXO_VOLTAGE_2_2V                       0x03
#define TCXO_VOLTAGE_2_4V                       0x04
#define TCXO_VOLTAGE_2_7V                       0x05
#define TCXO_VOLTAGE_3_0V                       0x06
#define TCXO_VOLTAGE_3_3V                       0x07

#define IRQ_MASK_TX_DONE                        0x0001 // All
#define IRQ_MASK_RX_DONE                        0x0002 // All
#define IRQ_MASK_PREAMBLE_DETECTED              0x0004 // All
#define IRQ_MASK_SYNC_WORD_VALID                0x0008 // FSK
#define IRQ_MASK_HEADER_VALID                   0x0010 // LoRa®
#define IRQ_MASK_HEADER_ERR                     0x0020 // LoRa®
#define IRQ_MASK_CRC_ERR                        0x0040 // All
#define IRQ_MASK_CAD_DONE                       0x0080 // LoRa®
#define IRQ_MASK_CAD_DETECTED                   0x0100 // LoRa®
#define IRQ_MASK_TIMEOUT                        0x0200 // All

extern const struct radio_dev sx126x_dev;

#endif /* ~__UWAN_DEVICE_SX126X_H__ */
