#include <lvgl.h>
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "actions.h"
#include "common/buzzer_api.h"
#include "screens.h"
//#include "thread/uart5.h"
#include "common/buzzer_api.h"
#include <stdint.h>
#include "msp_service.h"
#include "msp_ids.h"
#include "app_event_hub.h"
#include "app_event_ids.h"
#include "app_controller.h"

//extern  uart5_handle_t *Serial;
/* Lưu handle MSP để gửi dữ liệu từ UI */
static msp_service_t *s_msp_ui = NULL;

void action_read_txt_user_console(lv_event_t * e){
    (void)e;
    const char *console = lv_textarea_get_text(objects.txt_log);
    if(!console) console = "";
    size_t len = strlen(console);
    /* UI không gửi thẳng MSP nữa → đẩy vào hub để controller xử lý */
    app_event_dispatch_c(APP_EVT_USER_CONSOLE, console, (int)len);
    printf("[UI] console posted to hub %zu bytes\n", len);
}
void action_beep_buzzer(lv_event_t * e) {
    /* Phù hợp test thực tế: wPi 22 (PI12/PWM2), 800 Hz trong 200 ms */
    printf("action_beep_buzzer\n");
    beep_buzzer(22, 8000, 50);
}

/* Handler nhận sự kiện từ hub (MSP/UI/Screen/Dim) */
static void on_event_from_hub(int event_id, const void *data, int len)
{
    switch(event_id) {
        case MSP_RTC_TIME:
            printf("[HUB][RTC_TIME] len=%d\n", len);
            break;
        case MSP_DEVICE_STATUS:
            printf("[HUB][DEVICE_STATUS] len=%d\n", len);
            break;
        case MSP_MAIN_DISPLAY_CHANGE:
            printf("[HUB][MAIN_DISPLAY_CHANGE] len=%d\n", len);
            break;
        case MSP_SETTING_RTC:
            printf("[HUB][SETTING_RTC] len=%d\n", len);
            break;
        case MSP_DEVICE_CONFIG:
            printf("[HUB][DEVICE_CONFIG] len=%d\n", len);
            break;
        case MSP_DEVICE_HARDWARE_ID:
            printf("[HUB][DEVICE_HW_ID] len=%d\n", len);
            break;
        case MSP_NETWORK_INFO:
            printf("[HUB][NETWORK_INFO] len=%d\n", len);
            break;
        case MSP_TIMER_DATA:
            printf("[HUB][TIMER_DATA] len=%d\n", len);
            break;
        case MSP_DETAIL_PIN_SCHEDULE:
            printf("[HUB][DETAIL_PIN_SCHEDULE] len=%d\n", len);
            break;
        case MSP_DISPLAY_CHAGE_PAGE:
            printf("[HUB][DISPLAY_CHANGE_PAGE] len=%d\n", len);
            break;
        default:
            printf("[HUB][UNKNOWN %d] len=%d\n", event_id, len);
            break;
    }
}

/* Hàm tạo service; bạn có thể gọi từ nơi khởi tạo app */
extern "C" msp_service_t *create_msp_service()
{
    /* Cho phép cấu hình qua env (đã được set từ main qua -C/-U nếu có) */
    const char *dev = getenv("MSP_SERIAL");
    if(!dev || !*dev) dev = "/dev/ttyS2"; /* COM3 mặc định */
    const char *baud_env = getenv("MSP_BAUD");
    int baud = baud_env && *baud_env ? atoi(baud_env) : 115200;
    /* Khởi tạo hub và đăng ký handler UI */
    app_event_hub_init();
    app_event_hub_register_handler(on_event_from_hub);
    /* MSP → đổ sự kiện vào hub; Controller cài đặt để gửi MSP từ UI */
    s_msp_ui = msp_service_start(dev, baud, [](int id, const void *d, int l){ app_event_dispatch_c(id, d, l); });
    app_controller_init(s_msp_ui);
    return s_msp_ui;
}