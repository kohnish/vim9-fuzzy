#ifndef GREP_H
#define GREP_H
#include "str_pool.h"
#include <limits.h>
#include <stddef.h>
#include <uv.h>
#ifdef __cplusplus
extern "C" {
#endif

int queue_grep(uv_loop_t *loop, const char *cmd, const char *list_cmd, int seq);
void toggle_grep_cancel(int val);
int is_grep_search_ongoing(void);
void init_grep_mutex(void);
void deinit_grep_mutex(void);
char **cmd_to_str_arr(const char *cmdline, str_pool_t ***str_pool, size_t *result_len);

#ifdef __cplusplus
}
#endif
#endif /* GREP_H */
