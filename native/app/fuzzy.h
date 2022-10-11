#ifndef FUZZY_H
#define FUZZY_H

// Only For GoogleTest
#ifdef __cplusplus
extern "C" {
#endif

#include "search_helper.h"

typedef struct search_data_t {
    char cmd[32];
    char value[255];
    char list_cmd[255];
    char mru_path[PATH_MAX];
    char yank_path[PATH_MAX];
    file_info_t *file_info;
    size_t file_info_len;
} search_data_t;

size_t start_fuzzy_response(const char *search_keyword, const char *cmd, file_info_t *files, size_t len);
void toggle_cancel(int val);
void init_cancel_mutex();
void deinit_cancel_mutex();

#ifdef __cplusplus
}
#endif

#endif // FUZZY_H
