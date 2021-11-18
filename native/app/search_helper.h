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
    int fuzzy_score;
    int mru_score;
} __attribute__((aligned(ALIGN32))) file_info_t;

void init_file(char *root_dir);
void deinit_file();
const char *get_file_name(const char *path, str_pool_t ***pool);
int queue_search(uv_loop_t *loop, const char *value, const char *root_dir);
int is_file_search_ongoing();
void init_file_mutex();
void deinit_file_mutex();

#ifdef __cplusplus
}
#endif
#endif /* SEARCH_HELPER_H */
