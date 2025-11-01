#ifndef MSP_SERVICE_H
#define MSP_SERVICE_H

#include <stdint.h>
#include <stddef.h>
#include "msp_serial.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*msp_event_cb_t)(int event_id, const void *data, int len);

typedef struct msp_service msp_service_t;

/* Khởi tạo dịch vụ MSP trên thiết bị UART chỉ định */
msp_service_t *msp_service_start(const char *device, int baud, msp_event_cb_t ui_callback);

/* Dừng dịch vụ và giải phóng tài nguyên */
void msp_service_stop(msp_service_t *svc);

/* API gửi tiện ích */
int msp_service_send_byte(msp_service_t *svc, uint8_t cmd, uint8_t value);
int msp_service_send(msp_service_t *svc, uint8_t cmd, const void *payload, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* MSP_SERVICE_H */
