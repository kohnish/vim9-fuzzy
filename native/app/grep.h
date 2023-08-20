#ifndef GREP_H
#define GREP_H
#include "search_helper.h"
#include "str_pool.h"
#include <limits.h>
#include <stddef.h>
#include <uv.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct proc_ctx_T {
    int seq;
    file_info_t *file_res;
    size_t file_res_size;
    size_t file_res_buf_size;
    str_pool_t **str_pool;
} proc_ctx_T;

int queue_grep(uv_loop_t *loop, const char *cmd, const char *list_cmd, int seq);
void cancel_grep(void);

#ifdef __cplusplus
}
#endif
#endif /* GREP_H */
