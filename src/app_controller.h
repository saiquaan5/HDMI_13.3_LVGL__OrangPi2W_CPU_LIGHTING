#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include <stdint.h>
#include "msp_service.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Khởi tạo controller: cung cấp handle MSP để controller có thể gửi đi */
void app_controller_init(msp_service_t *msp);

/* Controller xử lý tất cả sự kiện từ hub (MSP/UI/Screen/Dim) */
void app_controller_on_event(int event_id, const void *data, int len);

#ifdef __cplusplus
}
#endif

#endif /* APP_CONTROLLER_H */


