#include "msp_service.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "msp_ids.h"
#include "app_event_hub.h"

struct msp_service {
    msp_serial_t *ms;
    msp_event_cb_t ui_cb;
};

static void on_frame(uint8_t cmd, const uint8_t *payload, uint16_t len, void *user)
{
    struct msp_service *svc = (struct msp_service *)user;
    if(!svc || !svc->ui_cb) return;
    /* Đẩy lên UI callback (sẽ là hub ở phía gọi) */
    svc->ui_cb((int)cmd, payload, (int)len);
}

msp_service_t *msp_service_start(const char *device, int baud, msp_event_cb_t ui_callback)
{
    struct msp_service *svc = (struct msp_service *)calloc(1, sizeof(*svc));
    if(!svc) return NULL;
    svc->ui_cb = ui_callback;
    svc->ms = msp_serial_open(device, baud);
    if(!svc->ms) { free(svc); return NULL; }
    msp_serial_set_callback(svc->ms, on_frame, svc);
    return svc;
}

void msp_service_stop(msp_service_t *svc)
{
    if(!svc) return;
    if(svc->ms) msp_serial_close(svc->ms);
    free(svc);
}

int msp_service_send_byte(msp_service_t *svc, uint8_t cmd, uint8_t value)
{
    if(!svc || !svc->ms) return -1;
    return msp_serial_send_byte(svc->ms, cmd, value);
}

int msp_service_send(msp_service_t *svc, uint8_t cmd, const void *payload, uint16_t len)
{
    if(!svc || !svc->ms) return -1;
    return msp_serial_send(svc->ms, cmd, (const uint8_t *)payload, len);
}
