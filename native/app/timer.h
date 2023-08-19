#ifndef TIMER_H
#define TIMER_H

#include <uv.h>


typedef int (*cond_cb_T)(void *data);
typedef void (*timer_cb_T)(void *data);

void timer_cond_schedule(uv_loop_t *loop, cond_cb_T cond_cb, timer_cb_T timer_cb, void *cond_arg, void *timer_arg, uint64_t timeout);

#endif /* TIMER_H */
