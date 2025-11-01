#ifndef UART_TEST_H
#define UART_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/* API test tự-echo cho UART3 (mặc định /dev/ttyS2) và UART4 (/dev/ttyS4) */

void uart_test_init(void);
void uart_test_suspend(void);
void uart_test_resume(void);
void uart_test_try_reopen_periodic(unsigned int now_ms);

#ifdef __cplusplus
}
#endif

#endif /* UART_TEST_H */


