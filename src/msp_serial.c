#include "msp_serial.h"
#include "uartx.h"

#include <stdlib.h>
#include <string.h>

struct msp_serial {
    uartx_handle_t *uart;
    msp_frame_cb_t on_frame;
    void *user;
    /* Parser state */
    enum { MSP_IDLE, MSP_HDR1, MSP_HDR2, MSP_DIR, MSP_LEN, MSP_CMD, MSP_PAYLOAD, MSP_CSUM } state;
    uint8_t checksum;
    uint8_t cmd;
    uint8_t len;
    uint8_t idx;
    uint8_t buf[255];
};

static void parser_reset(struct msp_serial *ms)
{
    ms->state = MSP_IDLE;
    ms->checksum = 0;
    ms->cmd = 0;
    ms->len = 0;
    ms->idx = 0;
}

static void on_uart_bytes(const uint8_t *data, uint32_t n, void *user)
{
    struct msp_serial *ms = (struct msp_serial *)user;
    for(uint32_t i = 0; i < n; i++) {
        uint8_t c = data[i];
        switch(ms->state) {
            case MSP_IDLE:   ms->state = (c == '$') ? MSP_HDR1 : MSP_IDLE; break;
            case MSP_HDR1:   ms->state = (c == 'M') ? MSP_HDR2 : MSP_IDLE; break;
            case MSP_HDR2:   ms->state = (c == '>') ? MSP_DIR  : MSP_IDLE; break; /* chỉ xử lý trả lời từ CPU chính */
            case MSP_DIR:    ms->len = c; ms->checksum = 0; ms->checksum ^= c; ms->state = MSP_LEN; break;
            case MSP_LEN:    ms->cmd = c; ms->checksum ^= c; ms->idx = 0; ms->state = ms->len ? MSP_PAYLOAD : MSP_CSUM; break;
            case MSP_PAYLOAD:
                ms->buf[ms->idx++] = c; ms->checksum ^= c;
                if(ms->idx >= ms->len) ms->state = MSP_CSUM;
                break;
            case MSP_CSUM:
                if(ms->checksum == c) {
                    if(ms->on_frame) ms->on_frame(ms->cmd, ms->buf, ms->len, ms->user);
                }
                parser_reset(ms);
                break;
            default:
                parser_reset(ms);
                break;
        }
    }
}

msp_serial_t *msp_serial_open(const char *device, int baud)
{
    struct msp_serial *ms = (struct msp_serial *)calloc(1, sizeof(*ms));
    if(!ms) return NULL;
    parser_reset(ms);
    ms->uart = uartx_open(device, baud);
    if(!ms->uart) { free(ms); return NULL; }
    uartx_set_callback(ms->uart, on_uart_bytes, ms);
    if(uartx_start(ms->uart) != 0) { uartx_close(ms->uart); free(ms); return NULL; }
    return ms;
}

void msp_serial_set_callback(msp_serial_t *ms, msp_frame_cb_t cb, void *user)
{
    if(!ms) return;
    ms->on_frame = cb;
    ms->user = user;
}

void msp_serial_close(msp_serial_t *ms)
{
    if(!ms) return;
    if(ms->uart) uartx_close(ms->uart);
    free(ms);
}

int msp_serial_send(msp_serial_t *ms, uint8_t cmd, const uint8_t *payload, uint16_t len)
{
    if(!ms || !ms->uart) return -1;
    if(len > 255) return -1;
    uint8_t header[5];
    header[0] = '$'; header[1] = 'M'; header[2] = '<';
    header[3] = (uint8_t)len; header[4] = cmd;
    uint8_t csum = 0; csum ^= header[3]; csum ^= header[4];
    int n = 0;
    n += uartx_write(ms->uart, header, 5);
    for(uint16_t i=0;i<len;i++){ csum ^= payload[i]; }
    if(len) n += uartx_write(ms->uart, payload, len);
    n += uartx_write(ms->uart, &csum, 1);
    return n;
}

int msp_serial_send_byte(msp_serial_t *ms, uint8_t cmd, uint8_t value)
{
    return msp_serial_send(ms, cmd, &value, 1);
}


