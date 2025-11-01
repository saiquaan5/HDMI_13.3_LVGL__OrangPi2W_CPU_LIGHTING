#include "uart_test.h"
#include "uartx.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static uartx_handle_t *g_uart3 = NULL; /* /dev/ttyS2 theo image hiện tại */
static uartx_handle_t *g_uart4 = NULL; /* /dev/ttyS3 trên H618 */
static char g_dev3[128];
static char g_dev4[128];
static int  g_baud3 = 115200;
static int  g_baud4 = 115200;
static int  g_log   = 1;

static void uart3_rx(const uint8_t *data, uint32_t len, void *user)
{
    (void)user;
    if(!data || len == 0) return;
    if(g_log) { printf("[uart3] RX %u bytes (self-echo)\n", (unsigned)len); fflush(stdout); }
    if(g_uart3) (void)uartx_write(g_uart3, data, len);
}

static void uart4_rx(const uint8_t *data, uint32_t len, void *user)
{
    (void)user;
    if(!data || len == 0) return;
    if(g_log) { printf("[uart4] RX %u bytes (self-echo)\n", (unsigned)len); fflush(stdout); }
    if(g_uart4) (void)uartx_write(g_uart4, data, len);
}

static void open_if_needed(void)
{
    if(!g_uart3) {
        fprintf(stderr, "[uart-test] opening %s @ %d\n", g_dev3, g_baud3);
        g_uart3 = uartx_open(g_dev3, g_baud3);
        if(g_uart3) {
            uartx_set_callback(g_uart3, uart3_rx, NULL);
            (void)uartx_start(g_uart3);
            if(g_log) { printf("[uart3] opened %s\n", g_dev3); fflush(stdout); }
        } else {
            fprintf(stderr, "[uart-test] open failed %s\n", g_dev3);
        }
    }
    if(!g_uart4) {
        fprintf(stderr, "[uart-test] opening %s @ %d\n", g_dev4, g_baud4);
        g_uart4 = uartx_open(g_dev4, g_baud4);
        if(g_uart4) {
            uartx_set_callback(g_uart4, uart4_rx, NULL);
            (void)uartx_start(g_uart4);
            if(g_log) { printf("[uart4] opened %s\n", g_dev4); fflush(stdout); }
        } else {
            fprintf(stderr, "[uart-test] open failed %s\n", g_dev4);
            /* Fallback: UART4_ALT, sau đó thử /dev/ttyS4 rồi /dev/ttyS5 (một lần) */
            const char *alt = getenv("UART4_ALT");
            static int tried_alt4 = 0;
            if(!tried_alt4) {
                tried_alt4 = 1;
                const char *candidates[3];
                int n = 0;
                if(alt && *alt) candidates[n++] = alt;
                candidates[n++] = "/dev/ttyS4";
                candidates[n++] = "/dev/ttyS5";
                for(int i=0;i<n && !g_uart4;i++) {
                    const char *try_dev = candidates[i];
                    if(strcmp(try_dev, g_dev4) == 0) continue;
                    fprintf(stderr, "[uart-test] trying fallback %s @ %d\n", try_dev, g_baud4);
                    uartx_handle_t *h = uartx_open(try_dev, g_baud4);
                    if(h) {
                        g_uart4 = h;
                        strncpy(g_dev4, try_dev, sizeof(g_dev4)-1);
                        uartx_set_callback(g_uart4, uart4_rx, NULL);
                        (void)uartx_start(g_uart4);
                        if(g_log) { printf("[uart4] opened %s (fallback)\n", g_dev4); fflush(stdout); }
                    }
                }
                if(!g_uart4) fprintf(stderr, "[uart-test] all fallbacks failed for UART4\n");
            }
        }
    }
}

void uart_test_init(void)
{
    const char *dev3 = getenv("UART3_DEV");
    const char *dev4 = getenv("UART4_DEV");
    const char *b3   = getenv("UART3_BAUD");
    const char *b4   = getenv("UART4_BAUD");
    const char *loge = getenv("UART_LOG");

    if(!dev3 || !*dev3) dev3 = "/dev/ttyS2";
    if(!dev4 || !*dev4) dev4 = "/dev/ttyS3"; /* H618: pi-uart4 -> ttyS3 */
    memset(g_dev3, 0, sizeof(g_dev3)); strncpy(g_dev3, dev3, sizeof(g_dev3)-1);
    memset(g_dev4, 0, sizeof(g_dev4)); strncpy(g_dev4, dev4, sizeof(g_dev4)-1);
    g_baud3 = b3 && *b3 ? (int)strtol(b3, NULL, 10) : 115200;
    g_baud4 = b4 && *b4 ? (int)strtol(b4, NULL, 10) : 115200;
    if(loge && *loge) g_log = (int)strtol(loge, NULL, 10) != 0;

    printf("[uart-test] SELF-ECHO: %s@%d and %s@%d (log=%d)\n", g_dev3, g_baud3, g_dev4, g_baud4, g_log);
    fflush(stdout);
    open_if_needed();
}

void uart_test_suspend(void)
{
    if(g_log) { printf("[uart-test] suspend: closing %s and %s\n", g_dev3, g_dev4); fflush(stdout); }
    if(g_uart3) { uartx_close(g_uart3); g_uart3 = NULL; }
    if(g_uart4) { uartx_close(g_uart4); g_uart4 = NULL; }
}

void uart_test_resume(void)
{
    open_if_needed();
}

void uart_test_try_reopen_periodic(unsigned int now_ms)
{
    static unsigned int last_try = 0;
    if(now_ms - last_try < 1000) return;
    last_try = now_ms;
    if(!g_uart3 || !g_uart4) open_if_needed();
}


