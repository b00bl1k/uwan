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

#ifndef __UWAN_DEVICE_SX127X_H__
#define __UWAN_DEVICE_SX127X_H__

#include <uwan/device/radio.h>

/* Common Registers */
#define SX127X_REG_FIFO                         0x00
#define SX127X_REG_OP_MODE                      0x01
#define SX127X_REG_FRF_MSB                      0x06
#define SX127X_REG_FRF_MID                      0x07
#define SX127X_REG_FRF_LSB                      0x08
#define SX127X_REG_PA_CONFIG                    0x09
#define SX127X_REG_PA_RAMP                      0x0a
#define SX127X_REG_LNA                          0x0c
#define SX127X_REG_VERSION                      0x42

/* LoRa Mode Registers */
#define SX127X_REG_LR_FIFO_ADDR_PTR             0x0d
#define SX127X_REG_LR_FIFO_TX_BASE_ADDR         0x0e
#define SX127X_REG_LR_IRQ_FLAGS_MASK            0x11
#define SX127X_REG_LR_IRQ_FLAGS                 0x12
#define SX127X_REG_LR_MODEM_CONFIG1             0x1d
#define SX127X_REG_LR_MODEM_CONFIG2             0x1e
#define SX127X_REG_LR_SYMB_TIMEOUT_LSB          0x1f
#define SX127X_REG_LR_PREAMBLE_MSB              0x20
#define SX127X_REG_LR_PREAMBLE_LSB              0x21
#define SX127X_REG_LR_PAYLOAD_LENGTH            0x22
#define SX127X_REG_LR_MODEM_CONFIG3             0x26
#define SX127X_REG_LR_INVERT_IQ                 0x33
#define SX127X_REG_LR_SYNC_WORD                 0x39
#define SX127X_REG_LR_INVERT_IQ2                0x3b

/* Description of Common Registers ------------------------------------------ */

/* RegOpMode */
#define OP_MODE_RESET_VALUE                     0x01

#define _OP_MODE_LONG_RANGE_MODE_MASK           0x1
#define _OP_MODE_LONG_RANGE_MODE_SHIFT          7
#define _OP_MODE_LONG_RANGE_MODE_ON             1
#define _OP_MODE_LONG_RANGE_MODE_OFF            0
#define OP_MODE_LONG_RANGE_MODE_ON              (_OP_MODE_LONG_RANGE_MODE_ON << _OP_MODE_LONG_RANGE_MODE_SHIFT)
#define OP_MODE_LONG_RANGE_MODE_OFF             (_OP_MODE_LONG_RANGE_MODE_OFF << _OP_MODE_LONG_RANGE_MODE_SHIFT)

#define _OP_MODE_MODULATION_TYPE_MASK           0x3
#define _OP_MODE_MODULATION_TYPE_SHIFT          5
#define _OP_MODE_MODULATION_TYPE_FSK            0
#define _OP_MODE_MODULATION_TYPE_OOK            1
#define OP_MODE_MODULATION_TYPE_FSK             (_OP_MODE_MODULATION_TYPE_FSK << _OP_MODE_MODULATION_TYPE_SHIFT)
#define OP_MODE_MODULATION_TYPE_OOK             (_OP_MODE_MODULATION_TYPE_OOK << _OP_MODE_MODULATION_TYPE_SHIFT)

#define _OP_MODE_LOW_FREQUENCY_MODE_ON_MASK     0x1
#define _OP_MODE_LOW_FREQUENCY_MODE_ON_SHIFT    3
#define _OP_MODE_LOW_FREQUENCY_MODE_ON_HF       0
#define _OP_MODE_LOW_FREQUENCY_MODE_ON_LF       1
#define OP_MODE_LOW_FREQUENCY_MODE_ON_HF        (_OP_MODE_LOW_FREQUENCY_MODE_ON_HF << _OP_MODE_LOW_FREQUENCY_MODE_ON_SHIFT)
#define OP_MODE_LOW_FREQUENCY_MODE_ON_LF        (_OP_MODE_LOW_FREQUENCY_MODE_ON_LF << _OP_MODE_LOW_FREQUENCY_MODE_ON_SHIFT)

#define _OP_MODE_MODE_MASK                      0x07
#define _OP_MODE_MODE_SHIFT                     0
#define _OP_MODE_MODE_SLEEP                     0
#define _OP_MODE_MODE_STDBY                     1
#define _OP_MODE_MODE_FSTX                      2
#define _OP_MODE_MODE_TX                        3
#define _OP_MODE_MODE_FSRX                      4
#define _OP_MODE_MODE_RX                        5
#define OP_MODE_MODE_SLEEP                      (_OP_MODE_MODE_SLEEP << _OP_MODE_MODE_SHIFT)
#define OP_MODE_MODE_STDBY                      (_OP_MODE_MODE_STDBY << _OP_MODE_MODE_SHIFT)
#define OP_MODE_MODE_FSTX                       (_OP_MODE_MODE_FSTX << _OP_MODE_MODE_SHIFT)
#define OP_MODE_MODE_TX                         (_OP_MODE_MODE_TX << _OP_MODE_MODE_SHIFT)
#define OP_MODE_MODE_FSRX                       (_OP_MODE_MODE_FSRX << _OP_MODE_MODE_SHIFT)
#define OP_MODE_MODE_RX                         (_OP_MODE_MODE_RX << _OP_MODE_MODE_SHIFT)

/* RegFrfMsb */
#define FRF_MSB_RESET_VALUE                     0x6c

/* RegFrfMid */
#define FRF_MID_RESET_VALUE                     0x80

/* RegFrfLsb */
#define FRF_LSB_RESET_VALUE                     0x00

/* RegPaConfig */
#define PA_CONFIG_RESET_VALUE                   0x4f

#define _PA_CONFIG_PA_SELECT_MASK               0x1
#define _PA_CONFIG_PA_SELECT_SHIFT              7
#define _PA_CONFIG_PA_SELECT_RFO                0
#define _PA_CONFIG_PA_SELECT_PA_BOOST           1
#define PA_CONFIG_PA_SELECT_RFO                 (_PA_CONFIG_PA_SELECT_RFO << _PA_CONFIG_PA_SELECT_SHIFT)
#define PA_CONFIG_PA_SELECT_PA_BOOST            (_PA_CONFIG_PA_SELECT_PA_BOOST << _PA_CONFIG_PA_SELECT_SHIFT)

#define _PA_CONFIG_MAX_POWER_MASK               0x7
#define _PA_CONFIG_MAX_POWER_SHIFT              4

#define _PA_CONFIG_OUTPUT_POWER_MASK            0xf
#define _PA_CONFIG_OUTPUT_POWER_SHIFT           0

/* RegPaRamp */
#define PA_RAMP_RESET_VALUE                     0x09

#define _PA_RAMP_PA_RAMP_MASK                   0xf
#define _PA_RAMP_PA_RAMP_SHIFT                  0
#define _PA_RAMP_PA_RAMP_3_4MS                  0
#define _PA_RAMP_PA_RAMP_2MS                    1
#define _PA_RAMP_PA_RAMP_1MS                    2
#define _PA_RAMP_PA_RAMP_500US                  3
#define _PA_RAMP_PA_RAMP_250US                  4
#define _PA_RAMP_PA_RAMP_125US                  5
#define _PA_RAMP_PA_RAMP_100US                  6
#define _PA_RAMP_PA_RAMP_62US                   7
#define _PA_RAMP_PA_RAMP_50US                   8
#define _PA_RAMP_PA_RAMP_40US                   9
#define _PA_RAMP_PA_RAMP_31US                   10
#define _PA_RAMP_PA_RAMP_25US                   11
#define _PA_RAMP_PA_RAMP_20US                   12
#define _PA_RAMP_PA_RAMP_15US                   13
#define _PA_RAMP_PA_RAMP_12US                   14
#define _PA_RAMP_PA_RAMP_10US                   15
#define PA_RAMP_PA_RAMP_3_4MS                   (_PA_RAMP_PA_RAMP_3_4MS << _PA_RAMP_PA_RAMP_SHIFT)
#define PA_RAMP_PA_RAMP_2MS                     (_PA_RAMP_PA_RAMP_2MS << _PA_RAMP_PA_RAMP_SHIFT)
#define PA_RAMP_PA_RAMP_1MS                     (_PA_RAMP_PA_RAMP_1MS << _PA_RAMP_PA_RAMP_SHIFT)
#define PA_RAMP_PA_RAMP_500US                   (_PA_RAMP_PA_RAMP_500US << _PA_RAMP_PA_RAMP_SHIFT)
#define PA_RAMP_PA_RAMP_250US                   (_PA_RAMP_PA_RAMP_250US << _PA_RAMP_PA_RAMP_SHIFT)
#define PA_RAMP_PA_RAMP_125US                   (_PA_RAMP_PA_RAMP_125US << _PA_RAMP_PA_RAMP_SHIFT)
#define PA_RAMP_PA_RAMP_100US                   (_PA_RAMP_PA_RAMP_100US << _PA_RAMP_PA_RAMP_SHIFT)
#define PA_RAMP_PA_RAMP_62US                    (_PA_RAMP_PA_RAMP_62US << _PA_RAMP_PA_RAMP_SHIFT)
#define PA_RAMP_PA_RAMP_50US                    (_PA_RAMP_PA_RAMP_50US << _PA_RAMP_PA_RAMP_SHIFT)
#define PA_RAMP_PA_RAMP_40US                    (_PA_RAMP_PA_RAMP_40US << _PA_RAMP_PA_RAMP_SHIFT)
#define PA_RAMP_PA_RAMP_31US                    (_PA_RAMP_PA_RAMP_31US << _PA_RAMP_PA_RAMP_SHIFT)
#define PA_RAMP_PA_RAMP_25US                    (_PA_RAMP_PA_RAMP_25US << _PA_RAMP_PA_RAMP_SHIFT)
#define PA_RAMP_PA_RAMP_20US                    (_PA_RAMP_PA_RAMP_20US << _PA_RAMP_PA_RAMP_SHIFT)
#define PA_RAMP_PA_RAMP_15US                    (_PA_RAMP_PA_RAMP_15US << _PA_RAMP_PA_RAMP_SHIFT)
#define PA_RAMP_PA_RAMP_12US                    (_PA_RAMP_PA_RAMP_12US << _PA_RAMP_PA_RAMP_SHIFT)
#define PA_RAMP_PA_RAMP_10US                    (_PA_RAMP_PA_RAMP_10US << _PA_RAMP_PA_RAMP_SHIFT)

/* RegLna */
#define LNA_RESET_VALUE                         0x20

/* RegVersion */
#define VERSION_RESET_VALUE                     0x12

/* Description of FSK/OOK Mode Registers ------------------------------------ */

/* RegRxConfig */
#define RX_CONFIG_RESET_VALUE                   0x08

/* RegRssiConfig */
#define RSSI_CONFIG_RESET_VALUE                 0x02

/* RegRssiValue */

/* RegRxBw */
#define RX_BW_RESET_VALUE                       0x15

/* RegFeiMsb */
#define FEI_MSB_RESET_VALUE                     0x00

/* RegFeiLsb */
#define FEI_LSB_RESET_VALUE                     0x00

/* RegPreambleDetect */
#define PREAMBLE_DETECT_RESET_VALUE             0x40

/* RegRxTimeout1 */
#define RX_TIMEOUT1_RESET_VALUE                 0x00

/* RegRxTimeout2 */
#define RX_TIMEOUT2_RESET_VALUE                 0x00

/* RegPreambleLsb */
#define PREAMBLE_LSB_RESET_VALUE                0x03

/* RegNodeAdrs */
#define NODE_ADRS_RESET_VALUE                   0x00

/* RegTimer1Coef */
#define TIMER1_COEF_RESET_VALUE                 0xF5

/* RegImageCal */
#define IMAGE_CAL_RESET_VALUE                   0x82

/* Description of LoRa Mode Registers --------------------------------------- */

/* RegFifoAddrPtr */
#define FIFO_ADDR_PTR_RESET_VALUE               0x00

/* RegFifoTxBaseAddr */
#define FIFO_TX_BASE_ADDR_RESET_VALUE           0x80

/* RegIrqFlagsMask */
#define IRQ_FLAGS_MASK_RESET_VALUE              0x00

/* RegIrqFlags */
#define IRQ_FLAGS_RESET_VALUE                   0x00

/* RegModemConfig1 */
#define MODEM_CONFIG1_RESET_VALUE               0x72

#define _MODEM_CONFIG1_BW_MASK                  0xf
#define _MODEM_CONFIG1_BW_SHIFT                 4
#define _MODEM_CONFIG1_BW_7_8KHZ                0
#define _MODEM_CONFIG1_BW_10_4KHZ               1
#define _MODEM_CONFIG1_BW_15_6KHZ               2
#define _MODEM_CONFIG1_BW_20_8KHZ               3
#define _MODEM_CONFIG1_BW_31_25KHZ              4
#define _MODEM_CONFIG1_BW_41_7KHZ               5
#define _MODEM_CONFIG1_BW_62_5KHZ               6
#define _MODEM_CONFIG1_BW_125KHZ                7
#define _MODEM_CONFIG1_BW_250KHZ                8
#define _MODEM_CONFIG1_BW_500KHZ                9
#define MODEM_CONFIG1_BW_7_8KHZ                 (_MODEM_CONFIG1_BW_7_8KHZ << _MODEM_CONFIG1_BW_SHIFT)
#define MODEM_CONFIG1_BW_10_4KHZ                (_MODEM_CONFIG1_BW_10_4KHZ << _MODEM_CONFIG1_BW_SHIFT)
#define MODEM_CONFIG1_BW_15_6KHZ                (_MODEM_CONFIG1_BW_15_6KHZ << _MODEM_CONFIG1_BW_SHIFT)
#define MODEM_CONFIG1_BW_20_8KHZ                (_MODEM_CONFIG1_BW_20_8KHZ << _MODEM_CONFIG1_BW_SHIFT)
#define MODEM_CONFIG1_BW_31_25KHZ               (_MODEM_CONFIG1_BW_31_25KHZ << _MODEM_CONFIG1_BW_SHIFT)
#define MODEM_CONFIG1_BW_41_7KHZ                (_MODEM_CONFIG1_BW_41_7KHZ << _MODEM_CONFIG1_BW_SHIFT)
#define MODEM_CONFIG1_BW_62_5KHZ                (_MODEM_CONFIG1_BW_62_5KHZ << _MODEM_CONFIG1_BW_SHIFT)
#define MODEM_CONFIG1_BW_125KHZ                 (_MODEM_CONFIG1_BW_125KHZ << _MODEM_CONFIG1_BW_SHIFT)
#define MODEM_CONFIG1_BW_250KHZ                 (_MODEM_CONFIG1_BW_250KHZ << _MODEM_CONFIG1_BW_SHIFT)
#define MODEM_CONFIG1_BW_500KHZ                 (_MODEM_CONFIG1_BW_500KHZ << _MODEM_CONFIG1_BW_SHIFT)

#define _MODEM_CONFIG1_CODING_RATE_MASK         0x7
#define _MODEM_CONFIG1_CODING_RATE_SHIFT        1
#define _MODEM_CONFIG1_CODING_RATE_4_5          1
#define _MODEM_CONFIG1_CODING_RATE_4_6          2
#define _MODEM_CONFIG1_CODING_RATE_4_7          3
#define _MODEM_CONFIG1_CODING_RATE_4_8          4
#define MODEM_CONFIG1_CODING_RATE_4_5           (_MODEM_CONFIG1_CODING_RATE_4_5 << _MODEM_CONFIG1_CODING_RATE_SHIFT)
#define MODEM_CONFIG1_CODING_RATE_4_6           (_MODEM_CONFIG1_CODING_RATE_4_6 << _MODEM_CONFIG1_CODING_RATE_SHIFT)
#define MODEM_CONFIG1_CODING_RATE_4_7           (_MODEM_CONFIG1_CODING_RATE_4_7 << _MODEM_CONFIG1_CODING_RATE_SHIFT)
#define MODEM_CONFIG1_CODING_RATE_4_8           (_MODEM_CONFIG1_CODING_RATE_4_8 << _MODEM_CONFIG1_CODING_RATE_SHIFT)

#define _MODEM_CONFIG1_IMPICIT_HEADER_MODE_ON_MASK  0x1
#define _MODEM_CONFIG1_IMPICIT_HEADER_MODE_ON_SHIFT 0
#define _MODEM_CONFIG1_IMPICIT_HEADER_MODE_OFF      0
#define _MODEM_CONFIG1_IMPICIT_HEADER_MODE_ON       1
#define MODEM_CONFIG1_IMPICIT_HEADER_MODE_OFF       (_MODEM_CONFIG1_IMPICIT_HEADER_MODE_OFF << _MODEM_CONFIG1_IMPICIT_HEADER_MODE_ON_SHIFT)
#define MODEM_CONFIG1_IMPICIT_HEADER_MODE_ON        (_MODEM_CONFIG1_IMPICIT_HEADER_MODE_ON << _MODEM_CONFIG1_IMPICIT_HEADER_MODE_ON_SHIFT)

/* RegModemConfig2 */
#define MODEM_CONFIG2_RESET_VALUE               0x70

#define _MODEM_CONFIG2_SPREADING_FACTOR_MASK    0xf
#define _MODEM_CONFIG2_SPREADING_FACTOR_SHIFT   4
#define _MODEM_CONFIG2_SPREADING_FACTOR_6       6
#define _MODEM_CONFIG2_SPREADING_FACTOR_7       7
#define _MODEM_CONFIG2_SPREADING_FACTOR_8       8
#define _MODEM_CONFIG2_SPREADING_FACTOR_9       9
#define _MODEM_CONFIG2_SPREADING_FACTOR_10      10
#define _MODEM_CONFIG2_SPREADING_FACTOR_11      11
#define _MODEM_CONFIG2_SPREADING_FACTOR_12      12
#define MODEM_CONFIG2_SPREADING_FACTOR_6        (_MODEM_CONFIG2_SPREADING_FACTOR_6 << _MODEM_CONFIG2_SPREADING_FACTOR_SHIFT)
#define MODEM_CONFIG2_SPREADING_FACTOR_7        (_MODEM_CONFIG2_SPREADING_FACTOR_7 << _MODEM_CONFIG2_SPREADING_FACTOR_SHIFT)
#define MODEM_CONFIG2_SPREADING_FACTOR_8        (_MODEM_CONFIG2_SPREADING_FACTOR_8 << _MODEM_CONFIG2_SPREADING_FACTOR_SHIFT)
#define MODEM_CONFIG2_SPREADING_FACTOR_9        (_MODEM_CONFIG2_SPREADING_FACTOR_9 << _MODEM_CONFIG2_SPREADING_FACTOR_SHIFT)
#define MODEM_CONFIG2_SPREADING_FACTOR_10       (_MODEM_CONFIG2_SPREADING_FACTOR_10 << _MODEM_CONFIG2_SPREADING_FACTOR_SHIFT)
#define MODEM_CONFIG2_SPREADING_FACTOR_11       (_MODEM_CONFIG2_SPREADING_FACTOR_11 << _MODEM_CONFIG2_SPREADING_FACTOR_SHIFT)
#define MODEM_CONFIG2_SPREADING_FACTOR_12       (_MODEM_CONFIG2_SPREADING_FACTOR_12 << _MODEM_CONFIG2_SPREADING_FACTOR_SHIFT)

#define _MODEM_CONFIG2_TX_CONTINUOUS_MODE_MASK  0x1
#define _MODEM_CONFIG2_TX_CONTINUOUS_MODE_SHIFT 3
#define _MODEM_CONFIG2_TX_CONTINUOUS_MODE_OFF   0
#define _MODEM_CONFIG2_TX_CONTINUOUS_MODE_ON    1
#define MODEM_CONFIG2_TX_CONTINUOUS_MODE_OFF    (_MODEM_CONFIG2_TX_CONTINUOUS_MODE_OFF << _MODEM_CONFIG2_TX_CONTINUOUS_MODE_SHIFT)
#define MODEM_CONFIG2_TX_CONTINUOUS_MODE_ON     (_MODEM_CONFIG2_TX_CONTINUOUS_MODE_ON << _MODEM_CONFIG2_TX_CONTINUOUS_MODE_SHIFT)

#define _MODEM_CONFIG2_RX_PAYLOAD_CRC_ON_MASK   0x1
#define _MODEM_CONFIG2_RX_PAYLOAD_CRC_ON_SHIFT  2
#define _MODEM_CONFIG2_RX_PAYLOAD_CRC_OFF       0
#define _MODEM_CONFIG2_RX_PAYLOAD_CRC_ON        1
#define MODEM_CONFIG2_RX_PAYLOAD_CRC_OFF        (_MODEM_CONFIG2_RX_PAYLOAD_CRC_OFF << _MODEM_CONFIG2_RX_PAYLOAD_CRC_ON_SHIFT)
#define MODEM_CONFIG2_RX_PAYLOAD_CRC_ON         (_MODEM_CONFIG2_RX_PAYLOAD_CRC_ON << _MODEM_CONFIG2_RX_PAYLOAD_CRC_ON_SHIFT)

#define _MODEM_CONFIG2_SYMB_TIMEOUT_MASK        0x3
#define _MODEM_CONFIG2_SYMB_TIMEOUT_SHIFT       0

/* RegSymbTimeoutLsb */
#define SYMB_TIMEOUT_LSB_RESET_VALUE            0x64

/* LoRa RegPreambleMsb */
#define LR_PREAMBLE_MSB_RESET_VALUE             0x00

/* LoRa RegPreambleLsb */
#define LR_PREAMBLE_LSB_RESET_VALUE             0x08

/* LoRa RegPayloadLength */
#define LR_PAYLOAD_LENGTH_RESET_VALUE           0x01

/* RegModemConfig3 */
#define MODEM_CONFIG3_RESET_VALUE               0x00

#define _MODEM_CONFIG3_LOW_DATA_RATE_OPTIMIZE_MASK  0x1
#define _MODEM_CONFIG3_LOW_DATA_RATE_OPTIMIZE_SHIFT 3
#define _MODEM_CONFIG3_LOW_DATA_RATE_OPTIMIZE_OFF   0
#define _MODEM_CONFIG3_LOW_DATA_RATE_OPTIMIZE_ON    1
#define MODEM_CONFIG3_LOW_DATA_RATE_OPTIMIZE_OFF    (_MODEM_CONFIG3_LOW_DATA_RATE_OPTIMIZE_OFF << _MODEM_CONFIG3_LOW_DATA_RATE_OPTIMIZE_SHIFT)
#define MODEM_CONFIG3_LOW_DATA_RATE_OPTIMIZE_ON     (_MODEM_CONFIG3_LOW_DATA_RATE_OPTIMIZE_ON << _MODEM_CONFIG3_LOW_DATA_RATE_OPTIMIZE_SHIFT)

#define _MODEM_CONFIG3_AGC_AUTO_ON_MASK         0x1
#define _MODEM_CONFIG3_AGC_AUTO_ON_SHIFT        2
#define _MODEM_CONFIG3_AGC_AUTO_OFF             0
#define _MODEM_CONFIG3_AGC_AUTO_ON              1
#define MODEM_CONFIG3_AGC_AUTO_OFF              (_MODEM_CONFIG3_AGC_AUTO_OFF << _MODEM_CONFIG3_AGC_AUTO_ON_SHIFT)
#define MODEM_CONFIG3_AGC_AUTO_ON               (_MODEM_CONFIG3_AGC_AUTO_ON << _MODEM_CONFIG3_AGC_AUTO_ON_SHIFT)

/* RegInvertIQ */
#define INVERT_IQ_RESET_VALUE                   0x13

/* RegSyncWord */
#define SYNC_WORD_RESET_VALUE                   0x12

/* RegInvertIQ2 */
#define INVERT_IQ2_RESET_VALUE                  0x1D
#define INVERT_IQ2_ON                           0x19
#define INVERT_IQ2_OFF                          0x1D

extern const struct radio_dev sx127x_dev;

#endif /* ~__UWAN_DEVICE_SX127X_H__ */
