#include <wiringPi.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include "buzzer_api.h"
#include <cstdio>

typedef struct {
	int pin;
	int frequencyHz;
	int durationMs;
} BeepParams;

/* 0: chưa init; 1: đã init; 2: bị vô hiệu (không đủ quyền/thiết bị) */
static volatile sig_atomic_t g_initialized = 0;
static volatile sig_atomic_t g_beeping = 0;

static void pwm_stop_and_quiet(int pin)
{
	/* Dừng PWM và kéo chân xuống LOW để im hoàn toàn */
	pwmToneWrite(pin, 0);
	pwmWrite(pin, 0);
	pinMode(pin, OUTPUT);
	digitalWrite(pin, LOW);
}

static void *beep_worker(void *arg)
{
	BeepParams *p = (BeepParams *)arg;
	int pin = p->pin;
	int freq = p->frequencyHz;
	int dur  = p->durationMs;

	/* Bắt đầu tone */
	pinMode(pin, PWM_OUTPUT);
	pwmToneWrite(pin, freq);
	delay((unsigned int)dur);

	/* Tắt tone và đảm bảo im */
	pwm_stop_and_quiet(pin);

	g_beeping = 0;
	free(p);
	return NULL;
}

static int has_gpio_access(void)
{
	/* Cho phép vô hiệu hóa qua biến môi trường khi mô phỏng */
    if (getenv("LV_SIMULATOR") != NULL || getenv("NO_GPIO") != NULL)
        return 0;

	/* Có quyền root hoặc có /dev/gpiomem hoặc /dev/mem với RW */
    if (geteuid() == 0)
        return 1;
	if (access("/dev/gpiomem", R_OK | W_OK) == 0)
		return 1;
	if (access("/dev/mem", R_OK | W_OK) == 0)
		return 1;
	return 0;
}

/* Khởi tạo (gọi tự động ở lần đầu nếu bạn quên gọi)
 * Không làm chương trình abort nếu thiếu quyền: sẽ "vô hiệu hoá" buzzer
 */
 static int beep_buzzer_init_if_needed(void)
 {
	 if (!g_initialized) {
		 if (!has_gpio_access()) {
             fprintf(stderr, "[buzzer] No GPIO access. Buzzer disabled.\n");
             /* In thêm ra stdout để dễ thấy trên console */
             printf("[buzzer] No GPIO access (run with sudo or ensure /dev/gpiomem).\n");
			 g_initialized = 2; /* vô hiệu */
			 return 0;         /* cho phép app tiếp tục */
		 }
		 if (wiringPiSetup() != 0) {
             fprintf(stderr, "[buzzer] wiringPiSetup failed. Buzzer disabled.\n");
             printf("[buzzer] wiringPiSetup failed.\n");
			 g_initialized = 2; /* vô hiệu */
			 return 0;         /* cho phép app tiếp tục */
		 }
		 g_initialized = 1;   /* đã init và dùng GPIO thật */
	 }
	 return 0;
 }

/* API: gọi không chặn. Nếu đang beep, lời gọi mới sẽ bị bỏ qua và trả về 1. */
int beep_buzzer(int pin, int frequencyHz, int durationMs)
{
	pthread_t th;
	BeepParams *p;

	if (beep_buzzer_init_if_needed() < 0)
		return -1;

	/* Nếu không init thật (1) thì coi như no-op thành công */
    if (g_initialized != 1) {
        printf("[buzzer] skip: init_state=%d (0:not_init,2:disabled)\n", g_initialized);
        fflush(stdout);
        return 0;
    }

	if (frequencyHz < 0) frequencyHz = 0;
	if (durationMs < 0) durationMs = 0;

    if (g_beeping) {
        printf("[buzzer] busy: previous beep still running\n");
        fflush(stdout);
        return 1; /* đang bận, bỏ qua */
    }

	g_beeping = 1;

	p = (BeepParams *)malloc(sizeof(BeepParams));
	if (!p) {
		g_beeping = 0;
		return -1;
	}
	p->pin = pin;
	p->frequencyHz = frequencyHz;
	p->durationMs = durationMs;

	if (pthread_create(&th, NULL, beep_worker, p) != 0) {
		free(p);
		g_beeping = 0;
        fprintf(stderr, "[buzzer] pthread_create failed\n");
		return -1;
	}
	(void)pthread_detach(th);
	printf("beep_buzzer: pin=%d, frequencyHz=%d, durationMs=%d\n", pin, frequencyHz, durationMs);
    fflush(stdout);
	return 0;
}

/* API phụ: đảm bảo tắt im nếu cần (gọi khi thoát chương trình) */
void beep_buzzer_shutdown(int pin)
{
	pwm_stop_and_quiet(pin);
}



