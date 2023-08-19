#include <uv.h>
#include <stdlib.h>

typedef int (*cond_cb_T)(void *data);
typedef void (*timer_cb_T)(void *data);

typedef struct timer_container_T {
    cond_cb_T cond_cb;
    timer_cb_T timer_cb;
    void *cond_arg;
    void *timer_arg;
} timer_container_T;

void timer_cb_internal(uv_timer_t *handle) {
    timer_container_T *container = (timer_container_T *)handle->data;
    if (container->cond_cb(container->cond_arg)) {
        container->timer_cb(container->timer_arg);
        free(container);
        uv_timer_stop(handle);
        free(handle);
    } else {
        uv_timer_again(handle);
    }
}

void timer_cond_schedule(uv_loop_t *loop, cond_cb_T cond_cb, timer_cb_T timer_cb, void *cond_arg, void *timer_arg, uint64_t timeout) {
    uv_timer_t *handle = (uv_timer_t *)malloc(sizeof(uv_timer_t));
    timer_container_T *container = (timer_container_T*)malloc(sizeof(timer_container_T));
    container->cond_cb = cond_cb;
    container->timer_cb = timer_cb;
    container->cond_arg = cond_arg;
    container->timer_arg = timer_arg;
    handle->data = container;
    uv_timer_init(loop, handle);
    uv_timer_start(handle, timer_cb_internal, timeout, timeout);
}
