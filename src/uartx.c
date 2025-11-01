#include "uartx.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>

struct uartx_handle {
    int fd;
    pthread_t thread;
    volatile bool run;
    uartx_data_cb_t cb;
    void *user;
    char device[128];
    int baud;
};

static speed_t map_baud(int baud)
{
    switch(baud) {
        case 9600: return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
        case 57600: return B57600;
        case 115200: return B115200;
#ifdef B230400
        case 230400: return B230400;
#endif
#ifdef B460800
        case 460800: return B460800;
#endif
#ifdef B921600
        case 921600: return B921600;
#endif
        default: return B115200;
    }
}

static int uartx_configure(int fd, int baud)
{
    struct termios tio;
    if(tcgetattr(fd, &tio) != 0) return -1;
    cfmakeraw(&tio);
    tio.c_cflag |= (CLOCAL | CREAD);
    tio.c_cflag &= ~PARENB;   /* 8N1 */
    tio.c_cflag &= ~CSTOPB;
    tio.c_cflag &= ~CSIZE;
    tio.c_cflag |= CS8;
    tio.c_iflag &= ~(IXON | IXOFF | IXANY);
    speed_t sp = map_baud(baud);
    cfsetispeed(&tio, sp);
    cfsetospeed(&tio, sp);
    if(tcsetattr(fd, TCSANOW, &tio) != 0) return -1;
    return 0;
}

int uartx_set_line(uartx_handle_t *handle, int databits, int parity, int stopbits, int rtscts)
{
    if(!handle) return -1;
    struct termios tio;
    if(tcgetattr(handle->fd, &tio) != 0) return -1;
    tio.c_cflag &= ~CSIZE;
    switch(databits) {
        case 5: tio.c_cflag |= CS5; break;
        case 6: tio.c_cflag |= CS6; break;
        case 7: tio.c_cflag |= CS7; break;
        default: tio.c_cflag |= CS8; break;
    }
    tio.c_cflag &= ~(PARENB | PARODD);
    if(parity == 1) { /* odd */
        tio.c_cflag |= PARENB | PARODD;
    } else if(parity == 2) { /* even */
        tio.c_cflag |= PARENB;
    }
    if(stopbits == 2) tio.c_cflag |= CSTOPB; else tio.c_cflag &= ~CSTOPB;
    if(rtscts) tio.c_cflag |= CRTSCTS; else tio.c_cflag &= ~CRTSCTS;
    return tcsetattr(handle->fd, TCSANOW, &tio);
}

int uartx_enable_rs485(uartx_handle_t *handle, int enable, int rts_on_tx, int delay_before_us, int delay_after_us)
{
    (void)handle; (void)enable; (void)rts_on_tx; (void)delay_before_us; (void)delay_after_us;
    /* RS485 ioctl optional: not implemented on this toolchain */
    return 0;
}

static void *reader_thread(void *arg)
{
    uartx_handle_t *h = (uartx_handle_t *)arg;
    uint8_t buf[512];
    while(h->run) {
        int n = (int)read(h->fd, buf, sizeof(buf));
        if(n > 0) {
            if(h->cb) h->cb(buf, (uint32_t)n, h->user);
        } else if(n < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(2000);
                continue;
            } else {
                usleep(10000);
            }
        } else {
            usleep(2000);
        }
    }
    return NULL;
}

uartx_handle_t *uartx_open(const char *device_path, int baud_rate)
{
    const char *dev = device_path && *device_path ? device_path : "/dev/ttyS0"; /* default placeholder */
    int fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if(fd < 0) {
        /* log lỗi mở cổng */
        fprintf(stderr, "[uartx] open failed: %s (errno=%d)\n", dev, errno);
        return NULL;
    }
    if(uartx_configure(fd, baud_rate) != 0) {
        fprintf(stderr, "[uartx] tcsetattr failed: %s (errno=%d)\n", dev, errno);
        close(fd);
        return NULL;
    }

    uartx_handle_t *h = (uartx_handle_t *)calloc(1, sizeof(*h));
    if(!h) { close(fd); return NULL; }
    h->fd = fd;
    h->run = false;
    h->cb = NULL;
    h->user = NULL;
    h->baud = baud_rate;
    strncpy(h->device, dev, sizeof(h->device)-1);
    return h;
}

void uartx_set_callback(uartx_handle_t *handle, uartx_data_cb_t cb, void *user_data)
{
    if(!handle) return;
    handle->cb = cb;
    handle->user = user_data;
}

int uartx_start(uartx_handle_t *handle)
{
    if(!handle) return -1;
    if(handle->run) return 0;
    handle->run = true;
    int rc = pthread_create(&handle->thread, NULL, reader_thread, handle);
    if(rc != 0) { handle->run = false; return -1; }
    return 0;
}

void uartx_stop(uartx_handle_t *handle)
{
    if(!handle) return;
    if(!handle->run) return;
    handle->run = false;
    pthread_join(handle->thread, NULL);
}

int uartx_write(uartx_handle_t *handle, const void *buf, uint32_t len)
{
    if(!handle || handle->fd < 0 || !buf || len == 0) return -1;
    int n = (int)write(handle->fd, buf, len);
    if(n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return 0;
    return n;
}

int uartx_flush(uartx_handle_t *handle)
{
    if(!handle) return -1;
    return tcdrain(handle->fd);
}

void uartx_close(uartx_handle_t *handle)
{
    if(!handle) return;
    uartx_stop(handle);
    if(handle->fd >= 0) close(handle->fd);
    free(handle);
}


