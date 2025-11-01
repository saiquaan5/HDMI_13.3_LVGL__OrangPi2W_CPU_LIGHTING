#include "app_event_hub.h"
#include <pthread.h>
#include <stddef.h>

/* Thread-safe registry các handler (không dùng STL để tránh phụ thuộc libstdc++) */
#define APP_EVENT_MAX_HANDLERS 16
static app_event_cb_t s_handlers[APP_EVENT_MAX_HANDLERS];
static int s_handler_count = 0;
static pthread_mutex_t s_mu = PTHREAD_MUTEX_INITIALIZER;
static int s_inited = 0;

void app_event_hub_init(void)
{
    pthread_mutex_lock(&s_mu);
    if(!s_inited) {
        for(int i=0;i<APP_EVENT_MAX_HANDLERS;i++) s_handlers[i] = NULL;
        s_handler_count = 0;
        s_inited = 1;
    }
    pthread_mutex_unlock(&s_mu);
}

void app_event_hub_register_handler(app_event_cb_t handler)
{
    if(!handler) return;
    pthread_mutex_lock(&s_mu);
    for(int i=0;i<s_handler_count;i++) {
        if(s_handlers[i] == handler) { pthread_mutex_unlock(&s_mu); return; }
    }
    if(s_handler_count < APP_EVENT_MAX_HANDLERS) {
        s_handlers[s_handler_count++] = handler;
    }
    pthread_mutex_unlock(&s_mu);
}

void app_event_hub_unregister_handler(app_event_cb_t handler)
{
    pthread_mutex_lock(&s_mu);
    for(int i=0;i<s_handler_count;i++) {
        if(s_handlers[i] == handler) {
            for(int j=i+1;j<s_handler_count;j++) s_handlers[j-1] = s_handlers[j];
            s_handlers[--s_handler_count] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&s_mu);
}

void app_event_hub_clear(void)
{
    pthread_mutex_lock(&s_mu);
    for(int i=0;i<s_handler_count;i++) s_handlers[i] = NULL;
    s_handler_count = 0;
    pthread_mutex_unlock(&s_mu);
}

void app_event_dispatch_c(int event_id, const void *data, int len)
{
    app_event_cb_t snapshot[APP_EVENT_MAX_HANDLERS];
    int count = 0;
    pthread_mutex_lock(&s_mu);
    for(int i=0;i<s_handler_count;i++) snapshot[i] = s_handlers[i];
    count = s_handler_count;
    pthread_mutex_unlock(&s_mu);

    for(int i=0;i<count;i++) {
        if(snapshot[i]) snapshot[i](event_id, data, len);
    }
}


