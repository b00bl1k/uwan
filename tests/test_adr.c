#include <assert.h>
#include <string.h>

#include <uwan/stack.h>
#include <uwan/region/ru864.h>
#include "region/common.h"
#include "adr.h"
#include "mac.h"
#include "stack.h"

struct node_session uw_session = {
    .dr = UWAN_DR_5,
};

const struct uwan_region *uw_region = &region_ru864;

uint8_t mac_buf[15];
uint8_t mac_buf_pld_len;
uint8_t tx_power;
uint8_t nb_trans;

bool is_valid_dr(uint8_t dr)
{
    return true;
}

bool is_valid_tx_power(uint8_t tx_power)
{
    return true;
}

bool is_valid_frequency(uint32_t freq)
{
    return true;
}

bool set_nb_trans(uint8_t nb)
{
    nb_trans = nb;
    return true;
}

void reset_nb_trans()
{
    nb_trans = 0;
}

bool set_tx_power(uint8_t power)
{
    tx_power = power;
    return true;
}

bool mac_enqueue(uint8_t cid, const uint8_t *data, uint8_t size)
{
    if ((mac_buf_pld_len + sizeof(cid) + size) <= sizeof(mac_buf)) {
        mac_buf[mac_buf_pld_len++] = cid;
        memcpy(mac_buf + mac_buf_pld_len, data, size);
        mac_buf_pld_len += size;
        return true;
    }
    return false;
}

bool uwan_set_rx1_delay(uint8_t delay)
{
    return true;
}

bool uwan_set_rx1_dr_offset(uint8_t rx1_dr_offset)
{
    return true;
}

enum uwan_errs uwan_set_rx2(uint32_t frequency, enum uwan_dr dr)
{
    return UWAN_ERR_NO;
}

int main()
{
    uw_region->init();

    uwan_adr_enable(true);
    uwan_adr_setup_ack(2, 2);

    assert(uw_session.dr == UWAN_DR_5);
    assert(adr_get_req_bit() == false);

    adr_handle_uplink();
    adr_handle_uplink();

    assert(adr_get_req_bit() == true);

    adr_handle_uplink();
    adr_handle_uplink();
    adr_handle_uplink();

    assert(uw_session.dr == UWAN_DR_4);
    assert(adr_get_req_bit() == false);

    adr_handle_uplink();
    adr_handle_uplink();

    assert(adr_get_req_bit() == true);
    adr_handle_downlink();

    assert(adr_get_req_bit() == false);

    uint8_t dr_txpow = 0x21;
    uint16_t ch_mask = 0x3;
    uint8_t redundancy = 0x03;
    assert(adr_handle_link_req(dr_txpow, ch_mask, redundancy));

    uint8_t mac_ans[] = {CID_LINK_ADR, 0x7};
    assert(mac_buf_pld_len == sizeof(mac_ans));
    assert(memcmp(mac_buf, mac_ans, sizeof(mac_ans)) == 0);

    assert(uw_session.dr == UWAN_DR_2);
    assert(tx_power == 1);
    assert(nb_trans == 3);

    return 0;
}
