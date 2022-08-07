#include "search_helper.h"
#include "fuzzy.h"
#include "json_msg_handler.h"
#include "str_pool.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#define INITIAL_CACHE_SIZE 5000

static file_info_t *g_f_cache;
static size_t g_f_cache_len;
static str_pool_t **g_str_pool;
static uv_mutex_t file_init_mutex;
static int file_init = 0;

static void toggle_file_init(int val) {
    uv_mutex_lock(&file_init_mutex);
    file_init = val;
    uv_mutex_unlock(&file_init_mutex);
}

int is_file_search_ongoing() {
    return file_init;
}

void init_file_mutex() {
    uv_mutex_init(&file_init_mutex);
}

void deinit_file_mutex() {
    uv_mutex_destroy(&file_init_mutex);
}

void free_file_info(file_info_t **f) {
    free(*f);
}

void free_str(char **s) {
    free(*s);
}

const char *get_file_name(const char *path, str_pool_t ***pool) {
    char buf[PATH_MAX] = {0};
    size_t pos = 0;
    for (size_t i = 0; i < strlen(path); i++) {
        if (path[i] != '/') {
            buf[pos] = path[i];
            pos++;
        } else {
            memset(buf, 0, PATH_MAX);
            pos = 0;
        }
    }
    return str(pool, buf);
}

void deinit_file() {
    if (g_f_cache != NULL) {
        free(g_f_cache);
    }
    if (g_str_pool != NULL) {
        deinit_str_pool(g_str_pool);
    }
}

static size_t load_lines_from_fp_to_file_info(str_pool_t ***str_pool, size_t initial_sz, file_info_t **file_info, FILE *fp, char no_path_trim) {
    size_t counter = 0;
    while (1) {
        if (counter >= initial_sz) {
            initial_sz = initial_sz * 2;
            void *inc_fi = realloc(*file_info, sizeof(file_info_t) * initial_sz);
            *file_info = inc_fi;
        }

        char path_buf[PATH_MAX] = {0};
        if (fgets(path_buf, PATH_MAX, fp) == NULL) {
            break;
        }
        // Remove newline character.
        path_buf[strlen(path_buf) - 1] = '\0';
        (*file_info)[counter].file_path = str(str_pool, path_buf);
        if (no_path_trim == 1) {
            (*file_info)[counter].file_name = (*file_info)[counter].file_path;
        } else {
            (*file_info)[counter].file_name = get_file_name(path_buf, str_pool);
        }
        (*file_info)[counter].f_len = strlen((*file_info)[counter].file_name);
        counter++;
    }
    return counter;
}

static void init_file(const char *cmd, const char *list_cmd) {
    if (g_f_cache_len > 0) {
        deinit_file();
    }
    size_t current_size = INITIAL_CACHE_SIZE;
    g_f_cache = malloc(sizeof(file_info_t) * current_size);
    g_str_pool = init_str_pool(1024);

    FILE *fp = popen(list_cmd, "r");
    if (fp == NULL) {
        return;
    }
    memset(g_f_cache, 0, sizeof(file_info_t) * current_size);
    size_t loaded_sz;
    if (strcmp(cmd, "init_path") == 0) {
        loaded_sz = load_lines_from_fp_to_file_info(&g_str_pool, INITIAL_CACHE_SIZE, &g_f_cache, fp, 1);
    } else {
        loaded_sz = load_lines_from_fp_to_file_info(&g_str_pool, INITIAL_CACHE_SIZE, &g_f_cache, fp, 0);
    }
    pclose(fp);
    g_f_cache_len = loaded_sz;

    send_res_from_file_info("file", g_f_cache, g_f_cache_len);
}

static void after_fuzzy_file_search(uv_work_t *req, int status) {
    (void)status;
    search_data_t *search_data = (search_data_t *)req->data;
    free(search_data);
    free(req);
}

static void fuzzy_file_search(uv_work_t *req) {
    search_data_t *search_data = (search_data_t *)req->data;
    if (strlen(search_data->value) == 0) {
        init_file(search_data->cmd, search_data->list_cmd);
    } else {
        start_fuzzy_response(search_data->value, "file", search_data->file_info, search_data->file_info_len);
    }
    toggle_file_init(0);
}

int queue_search(uv_loop_t *loop, const char *cmd, const char *value, const char *list_cmd) {
    toggle_file_init(1);
    uv_work_t *req = malloc(sizeof(uv_work_t));
    search_data_t *search_data = malloc(sizeof(search_data_t));
    strcpy(search_data->value, value);
    search_data->value[strlen(value) + 1] = '\0';
    search_data->file_info_len = g_f_cache_len;
    search_data->file_info = g_f_cache;
    strcpy(search_data->cmd, cmd);
    strcpy(search_data->list_cmd, list_cmd);
    req->data = search_data;
    int ret = uv_queue_work(loop, req, fuzzy_file_search, after_fuzzy_file_search);
    if (ret != 0) {
        free(search_data);
        free(req);
        return -1;
    }
    return 0;
}
