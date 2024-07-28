#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <uwan/stack.h>
#include "adr.h"
#include "channels.h"
#include "mac.h"

uint8_t rx1_delay;
uint8_t rx1_dr_offset;
uint32_t rx2_freq;
enum uwan_dr rx2_dr;

bool adr_handle_link_req(uint8_t dr_txpow,uint16_t ch_mask, uint8_t redundancy)
{
    return true;
}

bool is_valid_dr(uint8_t dr)
{
    return true;
}

bool is_valid_frequency(uint32_t freq)
{
    return true;
}

bool uwan_set_rx1_dr_offset(uint8_t offset)
{
    rx1_dr_offset = offset;
    return true;
}

bool uwan_set_rx1_delay(uint8_t delay)
{
    rx1_delay = delay;
    return true;
}

enum uwan_errs uwan_set_rx2(uint32_t frequency, enum uwan_dr dr)
{
    rx2_freq = frequency;
    rx2_dr = dr;
    return true;
}

int main()
{
    mac_init();
    channels_init();

    const uint8_t mac_rq_pld[] = {
        CID_LINK_ADR, 0x31, 0x07, 0x00, 0x01,
        CID_DUTY_CYCLE, 0x00,
        CID_RX_PARAM_SETUP, 0x12, 0x40, 0x72, 0x84,
        CID_DEV_STATUS,
        CID_NEW_CHANNEL, 0x03, 0x40, 0x72, 0x84, 0x50,
        CID_NEW_CHANNEL, 0x04, 0x40, 0x72, 0x84, 0x41,
        CID_RX_TIMING_SETUP, 0x00,
        CID_TX_PARAM_SETUP, 0x00,
        CID_DI_CHANNEL, 0x00, 0x40, 0x72, 0x84,
    };
    mac_handle_commands(mac_rq_pld, sizeof(mac_rq_pld), -10);

    assert(rx1_dr_offset == 1);
    assert(rx1_delay == 1);
    assert(rx2_freq == 868000000);
    assert(rx2_dr == UWAN_DR_2);

    const uint8_t mac_ans_pld[] = {
        // CID_LINK_ADR, 0x07,
        CID_DUTY_CYCLE,
        CID_RX_PARAM_SETUP, 0x07,
        CID_DEV_STATUS, 0xff, 0x36,
        CID_NEW_CHANNEL, 0x03,
        CID_NEW_CHANNEL, 0x01,
        CID_RX_TIMING_SETUP,
    };
    assert(mac_get_payload_size() == sizeof(mac_ans_pld));

    uint8_t mac_buf[15];
    mac_get_payload(mac_buf, sizeof(mac_buf));
    assert(memcmp(mac_ans_pld, mac_buf, sizeof(mac_ans_pld)) == 0);

    return 0;
}
