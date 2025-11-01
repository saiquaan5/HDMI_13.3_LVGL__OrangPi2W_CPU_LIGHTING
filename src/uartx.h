/**
 * Simple UART wrapper (POSIX termios) for echo/testing
 */

#ifndef UARTX_H
#define UARTX_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct uartx_handle uartx_handle_t;

typedef void (*uartx_data_cb_t)(const uint8_t *data, uint32_t len, void *user_data);

uartx_handle_t *uartx_open(const char *device_path, int baud_rate);
void uartx_set_callback(uartx_handle_t *handle, uartx_data_cb_t cb, void *user_data);
int  uartx_start(uartx_handle_t *handle);
void uartx_stop(uartx_handle_t *handle);
int  uartx_write(uartx_handle_t *handle, const void *buf, uint32_t len);
int  uartx_flush(uartx_handle_t *handle);
void uartx_close(uartx_handle_t *handle);

/* Advanced line configuration: databits: 5..8, parity: 0-none,1-odd,2-even, stopbits:1 or 2, rtscts:0/1 */
int  uartx_set_line(uartx_handle_t *handle, int databits, int parity, int stopbits, int rtscts);

/* Optional RS485 enable (requires kernel support): rts_on_tx: 1-high during TX, delays in usec */
int  uartx_enable_rs485(uartx_handle_t *handle, int enable, int rts_on_tx, int delay_before_us, int delay_after_us);

#ifdef __cplusplus
}
#endif

#endif /* UARTX_H */


