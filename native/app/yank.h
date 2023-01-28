#ifndef YANK_H
#define YANK_H

#include <uv.h>
// Only For GoogleTest
#ifdef __cplusplus
extern "C" {
#endif

int init_yank(const char *yank_path);
void deinit_yank(void);
int queue_yank_search(uv_loop_t *loop, const char *value, const char *yank_path);
int is_yank_search_ongoing(void);
void init_yank_mutex(void);
void deinit_yank_mutex(void);

#ifdef __cplusplus
}
#endif

#endif // YANK_H
