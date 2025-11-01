/*******************************************************************
 *
 * main.c - LVGL simulator for GNU/Linux
 *
 * Based on the original file from the repository
 *
 * @note eventually this file won't contain a main function and will
 * become a library supporting all major operating systems
 *
 * To see how each driver is initialized check the
 * 'src/lib/display_backends' directory
 *
 * - Clean up
 * - Support for multiple backends at once
 *   2025 EDGEMTech Ltd.
 *
 * Author: EDGEMTech Ltd, Erik Tagirov (erik.tagirov@edgemtech.ch)
 *
 ******************************************************************/
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"

#include "src/lib/driver_backends.h"
#include "src/lib/simulator_util.h"
#include "src/lib/simulator_settings.h"
#include <pthread.h>
#include "ui/ui.h"
#include "ui/actions.h"
#include "msp_service.h"
#include "common/buzzer_api.h"
#include "uartx.h"
#include "msp_service.h"

/* Internal functions */
static void configure_simulator(int argc, char **argv);
static void print_lvgl_version(void);
static void print_usage(void);
static void cleanup_and_black_screen(void);
static void on_signal(int signum);
static void *lvgl_thread(void *arg);
static int write_fb_blank(int value);
static void clear_framebuffer(void);
static void indev_wake_event_cb(lv_event_t * e);
/* MSP service (giao tiếp CPU chính) */
static struct msp_service *s_msp = NULL;


/* contains the name of the selected backend if user
 * has specified one on the command line */
static char *selected_backend;
/* MSP serial config (override được bằng -C/-U hoặc env MSP_SERIAL/MSP_BAUD) */
static char s_msp_serial_dev[64] = "/dev/ttyS2"; /* COM3 mặc định → ttyS2 */
static int  s_msp_serial_baud = 115200;

/* Global simulator settings, defined in lv_linux_backend.c */
extern simulator_settings_t settings;
/* Global flag để kiểm soát việc thoát */
static volatile int exit_flag = 0;

/* Idle blanking config */
static volatile int s_is_blank = 0;
/* Cửa sổ hủy blank nếu vừa có hoạt động trong khoảng rất ngắn */
static const unsigned int s_cancel_window_ms = 300;
/* Beep khi đánh thức: cấu hình qua env BUZZER_PIN/FREQ/MS */
static int s_beep_pin = -1;         /* -1: disable */
static int s_beep_freq = 1800;
static int s_beep_ms   = 80;
/* Wake coordination */
static volatile int s_wake_requested = 0;
static volatile int s_refresh_frames = 0;
static unsigned int s_idle_timeout_ms = 2*60000; /* default 60s, có thể override bằng SCREEN_IDLE_MS */

/* UART test glue: removed */

/**
 * @brief Print LVGL version
 */
static void print_lvgl_version(void)
{
    fprintf(stdout, "%d.%d.%d-%s\n",
            LVGL_VERSION_MAJOR,
            LVGL_VERSION_MINOR,
            LVGL_VERSION_PATCH,
            LVGL_VERSION_INFO);
}

/**
 * @brief Print usage information
 */
static void print_usage(void)
{
    fprintf(stdout, "\nlvglsim [-V] [-B] [-b backend_name] [-W window_width] [-H window_height]\n\n");
    fprintf(stdout, "-V print LVGL version\n");
    fprintf(stdout, "-B list supported backends\n");
    fprintf(stdout, "-C serial (COM3|COM4|/dev/ttySx) [default COM3→/dev/ttyS2]\n");
    fprintf(stdout, "-U baudrate [default 115200]\n");
}

/**
 * @brief Configure simulator
 * @description process arguments recieved by the program to select
 * appropriate options
 * @param argc the count of arguments in argv
 * @param argv The arguments
 */
static void configure_simulator(int argc, char **argv)
{
    int opt = 0;

    selected_backend = NULL;
    driver_backends_register();

    const char *env_w = getenv("LV_SIM_WINDOW_WIDTH");
    const char *env_h = getenv("LV_SIM_WINDOW_HEIGHT");
    /* Default values */
    settings.window_width = atoi(env_w ? env_w : "1920");
    settings.window_height = atoi(env_h ? env_h : "1060");

    /* Parse the command-line options. */
    while ((opt = getopt (argc, argv, "b:fmW:H:C:U:BVh")) != -1) {
        switch (opt) {
        case 'h':
            print_usage();
            exit(EXIT_SUCCESS);
            break;
        case 'V':
            print_lvgl_version();
            exit(EXIT_SUCCESS);
            break;
        case 'B':
            driver_backends_print_supported();
            exit(EXIT_SUCCESS);
            break;
        case 'b':
            if (driver_backends_is_supported(optarg) == 0) {
                die("error no such backend: %s\n", optarg);
            }
            selected_backend = strdup(optarg);
            break;
        case 'W':
            settings.window_width = atoi(optarg);
            break;
        case 'H':
            settings.window_height = atoi(optarg);
            break;
        case 'C':
            /* Cho phép: COM3, COM4, hoặc đường dẫn /dev/ttySx */
            if (strncasecmp(optarg, "COM", 3) == 0) {
                int com = atoi(optarg + 3);
                /* Quy ước: COM3→ttyS2, COM4→ttyS3 */
                if (com == 3) snprintf(s_msp_serial_dev, sizeof(s_msp_serial_dev), "/dev/ttyS2");
                else if (com == 4) snprintf(s_msp_serial_dev, sizeof(s_msp_serial_dev), "/dev/ttyS3");
                else snprintf(s_msp_serial_dev, sizeof(s_msp_serial_dev), "/dev/ttyS2");
            } else if (strncmp(optarg, "/dev/", 5) == 0) {
                snprintf(s_msp_serial_dev, sizeof(s_msp_serial_dev), "%s", optarg);
            }
            break;
        case 'U':
            s_msp_serial_baud = atoi(optarg);
            if (s_msp_serial_baud <= 0) s_msp_serial_baud = 115200;
            break;
        case ':':
            print_usage();
            die("Option -%c requires an argument.\n", optopt);
            break;
        case '?':
            print_usage();
            die("Unknown option -%c.\n", optopt);
        }
    }

    /* Env override: nếu MSP_SERIAL/MSP_BAUD chưa được set từ ngoài thì set theo lựa chọn ở trên */
    const char *env_dev = getenv("MSP_SERIAL");
    if (!env_dev || !*env_dev) {
        setenv("MSP_SERIAL", s_msp_serial_dev, 1);
    } else {
        snprintf(s_msp_serial_dev, sizeof(s_msp_serial_dev), "%s", env_dev);
    }
    const char *env_baud = getenv("MSP_BAUD");
    if (!env_baud || !*env_baud) {
        char tmp[16]; snprintf(tmp, sizeof(tmp), "%d", s_msp_serial_baud);
        setenv("MSP_BAUD", tmp, 1);
    } else {
        int b = atoi(env_baud); if (b > 0) s_msp_serial_baud = b;
    }
}

/**
 * @brief entry point
 * @description start a demo
 * @param argc the count of arguments in argv
 * @param argv The arguments
 */
int main(int argc, char **argv)
{
    /* Do not force hiding the cursor; allow showing cursor for touch/mouse */
    configure_simulator(argc, argv);

    /* Ensure framebuffer is cleared on exit or termination */
    atexit(cleanup_and_black_screen);
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    /* Initialize LVGL. */
    lv_init();

    /* Initialize the configured backend */
    if (driver_backends_init_backend(selected_backend) == -1) {
        die("Failed to initialize display backend");
    }

    /* Enable for EVDEV support */
#if LV_USE_EVDEV
    if (driver_backends_init_backend("EVDEV") == -1) {
        die("Failed to initialize evdev");
    }
#endif

    /* EEZ UI: khởi tạo trước khi bắt đầu vòng lặp tick */
    ui_init();

    /* Khởi tạo MSP service (CPU chính) */
    s_msp = create_msp_service();

    /* UART test bridge removed */

    /* Gắn callback cho tất cả indev: khi đang sleep, chạm đầu tiên chỉ dùng để đánh thức */
    {
        lv_indev_t * indev = NULL;
        while((indev = lv_indev_get_next(indev)) != NULL) {
            lv_indev_add_event_cb(indev, indev_wake_event_cb, LV_EVENT_PRESSED, NULL);
        }
    }

    /* Đọc cấu hình beep từ env (tùy phần cứng) */
    {
        const char *p = getenv("BUZZER_PIN");
        if(p && *p) s_beep_pin = (int)strtol(p, NULL, 10);
        p = getenv("BUZZER_FREQ");
        if(p && *p) s_beep_freq = (int)strtol(p, NULL, 10);
        p = getenv("BUZZER_MS");
        if(p && *p) s_beep_ms = (int)strtol(p, NULL, 10);
    }

    /* Tự tạo thread tick để vừa xử lý LVGL vừa gọi ui_tick();
     * Không dùng driver_backends_run_loop vì nó chạy lv_timer_handler() vô hạn
     * và không gọi ui_tick() của EEZ. */
    pthread_t th_ui;
    if(pthread_create(&th_ui, NULL, lvgl_thread, NULL) != 0) {
        die("Failed to create LVGL thread\n");
    }
    /* Đọc timeout từ env nếu có */
    {
        const char *idle_env = getenv("SCREEN_IDLE_MS");
        if(idle_env && *idle_env) {
            unsigned long v = strtoul(idle_env, NULL, 10);
            if(v >= 1000 && v <= 24UL*60*60*1000UL) s_idle_timeout_ms = (unsigned int)v;
        }
    }

    /* Chờ thread LVGL kết thúc (sẽ kết thúc khi nhận tín hiệu) */
    pthread_join(th_ui, NULL);

    /* Đảm bảo dừng UART trước khi thoát bình thường */
    /* UART test bridge removed */
    
    return 0;
}

static void *lvgl_thread(void *arg)
{
    (void)arg;
    uint32_t last_dim_check = 0;
    while(!exit_flag) {
        ui_tick();
        /* lv_timer_handler trả về thời gian tới lần gọi tiếp theo (ms) */
        uint32_t idle = lv_timer_handler();
        /* Yêu cầu refresh sau unblank sẽ invalidate trong thread an toàn */
        if(s_refresh_frames > 0) {
            lv_display_t *disp = lv_display_get_default();
            if(disp == NULL) disp = lv_display_get_next(NULL);
            if(disp) {
                lv_obj_t *scr = lv_display_get_screen_active(disp);
                if(scr) lv_obj_invalidate(scr);
            }
            s_refresh_frames--;
        }

        /* Kiểm tra inactivity/blank mỗi ~200ms trong cùng thread để tránh race */
        uint32_t now = lv_tick_get();
        if(now - last_dim_check >= 200) {
            last_dim_check = now;
            uint32_t inactive = lv_display_get_inactive_time(NULL);

            /* Yêu cầu blank */
            if(!s_is_blank && inactive > s_idle_timeout_ms) {
                /* Kiểm tra lần nữa ngay trước khi tắt để hủy nếu vừa có tương tác */
                uint32_t recheck = lv_display_get_inactive_time(NULL);
                if(recheck + s_cancel_window_ms >= s_idle_timeout_ms) {
                    /* Dùng 1 (blank) thay vì 4 (powerdown) để tránh rủi ro với mmap của fbdev */
                    if(write_fb_blank(1) == 0) {
                        s_is_blank = 1;
                        /* UART test bridge removed */
                        /* screen blanked */
                    }
                }
            }

            /* Đánh thức theo yêu cầu từ callback hoặc nếu LVGL báo có hoạt động */
            if(s_is_blank && (s_wake_requested || inactive < 200)) {
                if(write_fb_blank(0) == 0) {
                    s_is_blank = 0;
                    s_wake_requested = 0;
                    /* Làm tươi lại framebuffer/screen để tránh vùng đen */
                    clear_framebuffer();
                    lv_display_trigger_activity(NULL);
                    s_refresh_frames = 3; /* sẽ invalidate ở trên */
                    /* UART test bridge removed */
                    /* screen unblanked */
                }
            }
            /* UART test bridge removed */
        }
        usleep(idle * 1000);
    }
    return NULL;
}

/* dim_thread đã được hợp nhất vào lvgl_thread để đảm bảo thread-safety cho các API LVGL */

/* Ghi vào sysfs để blank/unblank fb0: 0=unblank, 4=powerdown */
static int write_fb_blank(int value)
{
    int fd = open("/sys/class/graphics/fb0/blank", O_WRONLY);
    if(fd < 0) {
        return -1;
    }
    char buf[8];
    int len = snprintf(buf, sizeof(buf), "%d", value);
    int rc = (int)write(fd, buf, (size_t)len);
    close(fd);
    return rc < 0 ? -1 : 0;
}

/* Xoá nội dung fb về 0 để tránh lưu ảnh cũ khi bật lại */
static void clear_framebuffer(void)
{
    int fb = open("/dev/fb0", O_RDWR);
    if(fb < 0) return;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    if(ioctl(fb, FBIOGET_FSCREENINFO, &finfo) < 0 || ioctl(fb, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        close(fb);
        return;
    }
    size_t screen_size = (size_t)finfo.line_length * (size_t)vinfo.yres;
    void *zero_buf = calloc(1, screen_size);
    if(zero_buf) {
        (void)write(fb, zero_buf, screen_size);
        free(zero_buf);
    }
    close(fb);
}

/* Chặn PRESS đầu tiên để đánh thức: không bung xuống widget khi đang ngủ */
static void indev_wake_event_cb(lv_event_t * e)
{
    if(!s_is_blank) return; /* màn hình đang bật → không can thiệp */

    /* Chỉ đặt cờ yêu cầu wake, xử lý toàn bộ trong thread LVGL để an toàn */
    s_wake_requested = 1;

    /* Không dừng xử lý/không reset input trong callback để tránh crash */

}

static void on_signal(int signum)
{
    (void)signum;
    cleanup_and_black_screen();
    exit_flag = 1;
    //pthread_join(th_ui, NULL);
    _exit(0);
}

static void cleanup_and_black_screen(void)
{
    const char *fb_path = getenv("LV_LINUX_FBDEV_DEVICE");
    if(!fb_path || fb_path[0] == '\0') fb_path = "/dev/fb0";

    int fb = open(fb_path, O_RDWR);
    if(fb < 0) {
        return;
    }

    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    if(ioctl(fb, FBIOGET_FSCREENINFO, &finfo) < 0 || ioctl(fb, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        close(fb);
        return;
    }

    size_t screen_size = (size_t)finfo.line_length * (size_t)vinfo.yres;
    void *zero_buf = calloc(1, screen_size);
    if(!zero_buf) {
        close(fb);
        return;
    }

    ssize_t written = write(fb, zero_buf, screen_size);
    (void)written;
    free(zero_buf);
    close(fb);
}
