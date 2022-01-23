#ifndef SEARCH_HELPER_H
#define SEARCH_HELPER_H
#include "str_pool.h"
#include <limits.h>
#include <stddef.h>
#include <uv.h>
#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LITERAL_MATCH_LEN 3
#define MAX_FILENAME_LEN PATH_MAX
#define ALIGN32 32

typedef struct file_info_t {
    const char *file_path;
    const char *file_name;
    size_t f_len;
    size_t fuzzy_score;
    int mru_score;
} file_info_t;

void deinit_file();
const char *get_file_name(const char *path, str_pool_t ***pool);
int queue_search(uv_loop_t *loop, const char *cmd, const char *value, const char *list_cmd);
int is_file_search_ongoing();
void init_file_mutex();
void deinit_file_mutex();

#ifdef __cplusplus
}
#endif
#endif /* SEARCH_HELPER_H */
