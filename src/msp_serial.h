#ifndef MSP_SERIAL_H
#define MSP_SERIAL_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct msp_serial msp_serial_t;

/* Callback khi nhận được một frame MSP hợp lệ */
typedef void (*msp_frame_cb_t)(uint8_t cmd, const uint8_t *payload, uint16_t len, void *user);

/* Tạo và mở cổng UART theo device/baud. Không chiếm quyền nếu open thất bại (trả NULL) */
msp_serial_t *msp_serial_open(const char *device, int baud);

/* Đăng ký callback (có thể gọi nhiều lần để thay đổi). user là con trỏ ngữ cảnh của app. */
void msp_serial_set_callback(msp_serial_t *ms, msp_frame_cb_t cb, void *user);

/* Đóng cổng và giải phóng tài nguyên */
void msp_serial_close(msp_serial_t *ms);

/* Gửi một frame MSP: cmd + payload (len byte). Trả về số byte đã gửi (frame đầy đủ) hoặc -1 khi lỗi. */
int msp_serial_send(msp_serial_t *ms, uint8_t cmd, const uint8_t *payload, uint16_t len);

/* Tiện ích gửi một byte đơn */
int msp_serial_send_byte(msp_serial_t *ms, uint8_t cmd, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* MSP_SERIAL_H */


