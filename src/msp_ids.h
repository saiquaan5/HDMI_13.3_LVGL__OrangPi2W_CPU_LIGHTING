#ifndef MSP_IDS_H
#define MSP_IDS_H

/* Mã lệnh MSP tham chiếu từ code mẫu */
/* Sử dụng ID 8-bit để phù hợp parser (uint8_t) */
#define MSP_RTC_TIME               1
#define MSP_DEVICE_STATUS          2
#define MSP_MAIN_DISPLAY_CHANGE    3
#define MSP_SETTING_RTC            4
#define MSP_DEVICE_CONFIG          5
#define MSP_DEVICE_HARDWARE_ID     6
#define MSP_NETWORK_INFO           7
#define MSP_TIMER_DATA             8
#define MSP_DETAIL_PIN_SCHEDULE    9
#define MSP_DISPLAY_CHAGE_PAGE     10
#define MSP_SWICH_MANUAL_CONTROL   11
#define MSP_GPS_CONTROL            12
/* Gửi nội dung console từ UI sang MCU */
#define MSP_USER_CONSOLE           13
/* Bật/tắt sleep màn hình hiển thị */
#define MSP_DISPLAY_SLEEP          14

#endif /* MSP_IDS_H */

