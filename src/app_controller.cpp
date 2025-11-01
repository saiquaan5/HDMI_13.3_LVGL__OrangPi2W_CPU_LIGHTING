#include "app_controller.h"
#include "app_event_hub.h"
#include "app_event_ids.h"
#include "msp_ids.h"
#include <stdio.h>
#include <string.h>

static msp_service_t *s_msp = NULL;

void app_controller_init(msp_service_t *msp)
{
    s_msp = msp;
    app_event_hub_register_handler(app_controller_on_event);
}

static void send_struct_safe(uint8_t cmd, const void *payload, int len)
{
    if(!s_msp) { printf("[CTRL] MSP not ready for cmd %u\n", cmd); return; }
    if(payload && len > 0) (void)msp_service_send(s_msp, cmd, payload, (uint16_t)len);
    else (void)msp_service_send(s_msp, cmd, NULL, 0);
}

static void send_byte_safe(uint8_t cmd, uint8_t value)
{
    if(!s_msp) { printf("[CTRL] MSP not ready for cmd %u\n", cmd); return; }
    (void)msp_service_send_byte(s_msp, cmd, value);
}

void app_controller_on_event(int event_id, const void *data, int len)
{
    switch(event_id) {
        /* MSP → UI: hiện giữ nguyên route qua hub, UI có thể đăng ký thêm handler khác nếu cần */
        case MSP_RTC_TIME:
        case MSP_DEVICE_STATUS:
        case MSP_MAIN_DISPLAY_CHANGE:
        case MSP_SETTING_RTC:
        case MSP_DEVICE_CONFIG:
        case MSP_DEVICE_HARDWARE_ID:
        case MSP_NETWORK_INFO:
        case MSP_TIMER_DATA:
        case MSP_DETAIL_PIN_SCHEDULE:
        case MSP_DISPLAY_CHAGE_PAGE:
            /* Controller có thể can thiệp nếu cần mapping thêm, tạm thời pass-through */
            break;

        /* UI → MSP: hợp nhất luồng gửi theo code map */
        case APP_EVT_DISPLAY_CHANGE_PAGE:
            if(len == (int)sizeof(uint8_t) && data) {
                uint8_t screen_id = *(const uint8_t*)data;
                send_byte_safe(MSP_DISPLAY_CHAGE_PAGE, screen_id);
            }
            break;
        case APP_EVT_APP_STARTED:
            /* Theo flow trong main.c case 2001 */
            send_byte_safe(MSP_DISPLAY_SLEEP, 0);
            send_byte_safe(MSP_DISPLAY_CHAGE_PAGE, (uint8_t)0); /* SCREEN_ID_LOCK_SCREEN nếu cần map */
            break;
        case APP_EVT_USER_CONSOLE:
            if(data && len > 0) send_struct_safe(MSP_USER_CONSOLE, data, len);
            break;
        default:
            /* Không biết: bỏ qua */
            break;
    }
}


