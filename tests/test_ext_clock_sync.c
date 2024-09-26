#include <assert.h>
#include <string.h>

#include <uwan/stack.h>
#include <uwan/ext/clock_sync.h>

#define CID_PACKAGE_VERSION 0
#define CID_APP_TIME 1
#define CID_TIME_PERIODICITY 2
#define CID_FORCE_DEVICE_RESYNC 3

uint8_t frame[255];
uint8_t frame_size;
uint8_t frame_port;
int force_device_resync_call_count;
uint32_t test_unixtime;
int32_t test_clock_correction;
uint32_t time_periodicity;

static void set_clock_correction(int32_t value)
{
    test_clock_correction = value;
}

static void handle_app_time_periodicity_req(uint32_t period_sec)
{
    time_periodicity = period_sec;
}

static uint32_t get_unixtime(void)
{
    return test_unixtime;
}

static void force_device_resync_req(void)
{
    force_device_resync_call_count++;
}

enum uwan_errs uwan_send_frame(uint8_t f_port, const uint8_t *payload,
    uint8_t pld_len, bool confirm)
{
    if (pld_len < sizeof(frame))
    {
        memcpy(frame, payload, pld_len);
        frame_size = pld_len;
        frame_port = f_port;
        return UWAN_ERR_NO;
    }
    return UWAN_ERR_MSG_LEN;
}

int main()
{
    struct uwan_clock_sync_callbacks cbs = {
        .set_clock_correction = set_clock_correction,
        .handle_app_time_periodicity_req = handle_app_time_periodicity_req,
        .get_unixtime = get_unixtime,
        .force_device_resync_req = force_device_resync_req,
    };

    uwan_clock_sync_init(&cbs);

    assert(uwan_clock_sync_send_answ() == UWAN_ERR_STATE);

    test_unixtime = 1722419302;
    assert(uwan_clock_sync_send_time_req(true) == UWAN_ERR_NO);

    uint8_t up_pld[] = {CID_APP_TIME, 0xf8, 0xca, 0xd4, 0x53, 0x10};
    assert(frame_size == sizeof(up_pld));
    assert(memcmp(frame, up_pld, sizeof(up_pld)) == 0);

    uint8_t down_pld[] = {
        CID_PACKAGE_VERSION,
        CID_APP_TIME, 0xfe, 0xff, 0xff, 0xff, 0x00,
        CID_TIME_PERIODICITY, 0x01,
        CID_FORCE_DEVICE_RESYNC, 0x03,
    };
    struct uwan_dl_packet pkt = {
        .f_port = UWAN_EXT_CLOCK_SYNC_PORT,
        .data = down_pld,
        .size = sizeof(down_pld),
    };

    uwan_clock_sync_handle_time_answ(UWAN_ERR_NO, UWAN_MTYPE_UNCONF_DATA_DOWN, &pkt);

    assert(test_clock_correction == -2);
    assert(force_device_resync_call_count == 1);
    assert(uwan_clock_sync_is_answ_pending());

    assert(uwan_clock_sync_send_answ() == UWAN_ERR_NO);

    uint8_t up_pld2[] = {
        CID_PACKAGE_VERSION, 0x01, 0x01,
        CID_TIME_PERIODICITY, 0x00, 0xf8, 0xca, 0xd4, 0x53,
    };
    assert(frame_size == sizeof(up_pld2));
    assert(memcmp(frame, up_pld2, sizeof(up_pld2)) == 0);

    assert(time_periodicity == 256);

    return 0;
}
