/**
 * Central event hub: gom mọi callback về một nơi và phân phối lại
 */

#ifndef APP_EVENT_HUB_H
#define APP_EVENT_HUB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*app_event_cb_t)(int event_id, const void *data, int len);

/* Khởi tạo hub (idempotent) */
void app_event_hub_init(void);

/* Đăng ký/huỷ đăng ký handler */
void app_event_hub_register_handler(app_event_cb_t handler);
void app_event_hub_unregister_handler(app_event_cb_t handler);
void app_event_hub_clear(void);

/* Điểm vào C cho mọi nguồn callback (MSP/UI/Screen/Dim) */
void app_event_dispatch_c(int event_id, const void *data, int len);

#ifdef __cplusplus
}
#endif

#endif /* APP_EVENT_HUB_H */


