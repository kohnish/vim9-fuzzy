#ifndef MRU_H
#define MRU_H

#include <uv.h>
// Only For GoogleTest
#ifdef __cplusplus
extern "C" {
#endif

int init_mru(const char *mru_path);
void deinit_mru(void);
int queue_mru_search(uv_loop_t *loop, const char *value, const char *mru_path);
size_t write_mru(const char *mru_path, const char *path);
int is_mru_search_ongoing(void);
void init_mru_mutex(void);
void deinit_mru_mutex(void);

#ifdef __cplusplus
}
#endif

#endif // MRU_H
