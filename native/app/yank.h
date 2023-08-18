#ifndef YANK_H
#define YANK_H

#include <uv.h>
// Only For GoogleTest
#ifdef __cplusplus
extern "C" {
#endif

int init_yank(const char *yank_path, int seq);
int queue_yank_search(uv_loop_t *loop, const char *value, const char *yank_path, int seq);

#ifdef __cplusplus
}
#endif

#endif // YANK_H
