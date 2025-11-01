#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void action_beep_buzzer(lv_event_t * e);
extern void action_read_txt_user_console(lv_event_t * e);

/* Expose MSP service factory to C callers (e.g., main.c) */
struct msp_service; /* fwd */
extern struct msp_service *create_msp_service(void);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/