#include "yank.h"
#include "fuzzy.h"
#include "json_msg_handler.h"
#include "search_helper.h"
#include "str_pool.h"
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define INITIAL_CACHE_SIZE 100
#define RESULT_LEN_MAX (PATH_MAX * 2)
static file_info_t *g_yank_cache;
static size_t g_yank_len;
static str_pool_t **g_str_pool;

static uv_mutex_t yank_init_mutex;
static int yank_init = 0;

static void toggle_yank_init(int val) {
    uv_mutex_lock(&yank_init_mutex);
    yank_init = val;
    uv_mutex_unlock(&yank_init_mutex);
}

int is_yank_search_ongoing(void) {
    return yank_init;
}

void init_yank_mutex(void) {
    uv_mutex_init(&yank_init_mutex);
}

void deinit_yank_mutex(void) {
    uv_mutex_destroy(&yank_init_mutex);
}

static int yank_score_cmp(const void *s1, const void *s2) {
    int v1 = ((file_info_t *)s1)->yank_score;
    int v2 = ((file_info_t *)s2)->yank_score;

    return v1 == v2 ? 0 : v1 > v2 ? -1 : 1;
}

static size_t load_yank_to_file_info(str_pool_t ***str_pool, file_info_t **file_info, size_t *result_len, const char *yank_dir) {
    static size_t current_size = INITIAL_CACHE_SIZE;
    if (*file_info == NULL) {
        size_t initial_sz = sizeof(file_info_t) * current_size;
        *file_info = malloc(initial_sz);
        memset(*file_info, 0, initial_sz);
    }
    size_t line_counter = 0;
    DIR *dir = opendir(yank_dir);
    if (dir) {
        struct dirent *dir_entry;
        while ((dir_entry = readdir(dir)) != NULL) {
            if (line_counter >= current_size) {
                current_size = current_size * 2;
                *file_info = realloc(*file_info, sizeof(file_info_t) * current_size);
            }
            char path[RESULT_LEN_MAX] = { 0 };
            snprintf(path, RESULT_LEN_MAX, "%s/%s", yank_dir, dir_entry->d_name);
            if (dir_entry->d_type == DT_REG) {
                FILE *yank_fp = fopen(path, "r");
                if (yank_fp) {
                    int yank_fd = fileno(yank_fp);
                    struct stat st;
                    fstat(yank_fd, &st);
                    long mtime = st.st_mtime;
                    char *yank_line = NULL;
                    size_t yank_line_len;
                    getline(&yank_line, &yank_line_len, yank_fp);
                    yank_line[--yank_line_len] = '\0';
                    if (yank_line_len > 200) {
                        yank_line[200] = '\0';
                        yank_line_len = 200;
                    }
                    size_t escaped_len;
                    char *escaped_line = json_escape(yank_line, yank_line_len, &escaped_len);
                    escaped_line[escaped_len - 2] = '\0';
                    // yank_line[yank_line_len] = '\0';
                    char result_line[RESULT_LEN_MAX] = { 0 };
                    int result_line_len = snprintf(result_line, RESULT_LEN_MAX, "%s| %s", dir_entry->d_name, escaped_line);
                    free(escaped_line);
                    // printf("%s\n", result_line);
                    if (result_line_len > 0) {
                        (*file_info)[line_counter].file_path = pool_str(str_pool, result_line);
                        (*file_info)[line_counter].file_name = pool_str(str_pool, result_line);
                        (*file_info)[line_counter].f_len = result_line_len;
                        (*file_info)[line_counter].yank_score = mtime;
                    }
                    free(yank_line);
                    fclose(yank_fp);
                    line_counter++;
                }
            }
        }
        closedir(dir);
    }
    qsort(*file_info, line_counter - 1, sizeof(file_info_t), yank_score_cmp);
    *result_len = line_counter;
    return line_counter;
}

void deinit_yank(void) {
    if (g_yank_cache != NULL) {
        free(g_yank_cache);
        g_yank_cache = NULL;
        g_yank_len = 0;
    }
    if (g_str_pool != NULL) {
        deinit_str_pool(g_str_pool);
    }
}

int init_yank(const char *yank_path) {
    deinit_yank();
    g_str_pool = init_str_pool(10240);
    int ret = -1;
    ret = load_yank_to_file_info(&g_str_pool, &g_yank_cache, &g_yank_len, yank_path);
    if (ret > 0) {
        send_res_from_file_info("yank", g_yank_cache, g_yank_len);
    }
    return ret;
}

static void after_fuzzy_yank_search(uv_work_t *req, int status) {
    (void)status;
    search_data_t *search_data = (search_data_t *)req->data;
    free(search_data);
    free(req);
}

static void fuzzy_yank_search(uv_work_t *req) {
    search_data_t *search_data = (search_data_t *)req->data;
    if (strlen(search_data->value) == 0) {
        init_yank(search_data->yank_path);
    } else {
        start_fuzzy_response(search_data->value, "yank", search_data->file_info, search_data->file_info_len);
    }
    toggle_yank_init(0);
}

int queue_yank_search(uv_loop_t *loop, const char *value, const char *yank_path) {
    toggle_yank_init(1);
    uv_work_t *req = malloc(sizeof(uv_work_t));
    search_data_t *search_data = malloc(sizeof(search_data_t));
    strcpy(search_data->value, value);
    search_data->value[strlen(value) + 1] = '\0';
    search_data->file_info_len = g_yank_len;
    search_data->file_info = g_yank_cache;
    strcpy(search_data->yank_path, yank_path);
    req->data = search_data;
    int ret = uv_queue_work(loop, req, fuzzy_yank_search, after_fuzzy_yank_search);
    if (ret != 0) {
        free(search_data);
        free(req);
        return -1;
    }
    return 0;
}
