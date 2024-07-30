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

uint8_t test_link_check_margin;
uint8_t test_link_check_gw_cnt;
uint32_t test_device_time_sec;
uint8_t test_device_time_fraq;

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

int8_t get_snr()
{
    return -10;
}

static uint8_t get_battery_level(void)
{
    return 0x64;
}

static void link_check_result(uint8_t margin, uint8_t gw_cnt)
{
    test_link_check_margin = margin;
    test_link_check_gw_cnt = gw_cnt;
}

static void device_time_result(uint32_t sec, uint8_t fraq)
{
    test_device_time_sec = sec;
    test_device_time_fraq = fraq;
}

int main()
{
    mac_init();
    channels_init();

    struct uwan_mac_callbacks cbs = {
        .get_battery_level = get_battery_level,
        .link_check_result = link_check_result,
        .device_time_result = device_time_result,
    };

    uwan_mac_set_handlers(&cbs);

    const uint8_t mac_down_pld[] = {
        CID_LINK_CHECK, 0x0a, 0x01,
        CID_LINK_ADR, 0x31, 0x07, 0x00, 0x01,
        CID_DUTY_CYCLE, 0x00,
        CID_RX_PARAM_SETUP, 0x12, 0x40, 0x72, 0x84,
        CID_DEV_STATUS,
        CID_NEW_CHANNEL, 0x03, 0x40, 0x72, 0x84, 0x50,
        CID_NEW_CHANNEL, 0x04, 0x40, 0x72, 0x84, 0x41,
        CID_RX_TIMING_SETUP, 0x00,
        CID_TX_PARAM_SETUP, 0x00,
        CID_DI_CHANNEL, 0x00, 0x40, 0x72, 0x84,
        CID_DEVICE_TIME, 0x04, 0x03, 0x02, 0x01, 0xaa,
    };
    mac_handle_commands(mac_down_pld, sizeof(mac_down_pld));

    assert(rx1_dr_offset == 1);
    assert(rx1_delay == 1);
    assert(rx2_freq == 868000000);
    assert(rx2_dr == UWAN_DR_2);

    assert(test_link_check_margin == 0x0a);
    assert(test_link_check_gw_cnt == 0x01);
    assert(test_device_time_sec == 0x01020304);
    assert(test_device_time_fraq == 0xaa);

    assert(uwan_mac_link_check_req());
    assert(uwan_mac_device_time_req());

    const uint8_t mac_up_pld[] = {
        // CID_LINK_ADR, 0x07,
        CID_DUTY_CYCLE,
        CID_RX_PARAM_SETUP, 0x07,
        CID_DEV_STATUS, 0x64, 0x36,
        CID_NEW_CHANNEL, 0x03,
        CID_NEW_CHANNEL, 0x01,
        CID_RX_TIMING_SETUP,
        CID_LINK_CHECK,
        CID_DEVICE_TIME,
    };
    assert(mac_get_payload_size() == sizeof(mac_up_pld));

    uint8_t mac_buf[15];
    mac_get_payload(mac_buf, sizeof(mac_buf));
    assert(memcmp(mac_up_pld, mac_buf, sizeof(mac_up_pld)) == 0);

    return 0;
}
