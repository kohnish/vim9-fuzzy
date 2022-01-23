#include "mru.h"
#include "fuzzy.h"
#include "json_msg_handler.h"
#include "search_helper.h"
#include "str_pool.h"
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define INITIAL_CACHE_SIZE 100
static file_info_t *g_mru_cache;
static size_t g_mru_len;
static str_pool_t **g_str_pool;

static uv_mutex_t mru_init_mutex;
static int mru_init = 0;

static void toggle_mru_init(int val) {
    uv_mutex_lock(&mru_init_mutex);
    mru_init = val;
    uv_mutex_unlock(&mru_init_mutex);
}

int is_mru_search_ongoing() {
    return mru_init;
}

void init_mru_mutex() {
    uv_mutex_init(&mru_init_mutex);
}

void deinit_mru_mutex() {
    uv_mutex_destroy(&mru_init_mutex);
}

static size_t load_mru_to_file_info(str_pool_t ***str_pool, file_info_t **file_info, size_t *result_len, FILE *fp) {
    static size_t current_size = INITIAL_CACHE_SIZE;
    if (*file_info == NULL) {
        *file_info = malloc(sizeof(file_info_t) * current_size);
    }
    int current_mode = 0;
    int key_counter = 0;
    int val_counter = 0;
    size_t line_counter = 0;
    char key_buf[PATH_MAX] = {0};
    char val_buf[PATH_MAX] = {0};
    while (1) {
        char c = (char)fgetc(fp);
        if (c == ':') {
            current_mode = 1;
            if (line_counter >= current_size) {
                current_size = current_size * 2;
                *file_info = realloc(*file_info, sizeof(file_info_t) * current_size);
            }
            key_buf[strlen(key_buf) + 1] = '\0';
            (*file_info)[line_counter].file_path = str(str_pool, key_buf);
            (*file_info)[line_counter].file_name = get_file_name(key_buf, str_pool);
            (*file_info)[line_counter].f_len = strlen((*file_info)[line_counter].file_name);
            memset(key_buf, 0, PATH_MAX);
            key_counter = 0;
        } else if (c == '\n') {
            current_mode = 0;
            (*file_info)[line_counter].mru_score = atoi(val_buf);
            memset(val_buf, 0, PATH_MAX);
            val_counter = 0;
            line_counter++;
        } else if (c == EOF) {
            break;
        } else {
            if (current_mode == 0) {
                key_buf[key_counter] = c;
                key_counter++;
            } else {
                val_buf[key_counter] = c;
                val_counter++;
            }
        }
    }
    *result_len = line_counter;
    return line_counter;
}

static int mru_score_cmp(const void *s1, const void *s2) {
    int v1 = ((file_info_t *)s1)->mru_score;
    int v2 = ((file_info_t *)s2)->mru_score;

    return v1 == v2 ? 0 : v1 > v2 ? -1 : 1;
}

size_t write_mru(const char *mru_path, const char *path) {
    str_pool_t **pool = init_str_pool(10240);
    file_info_t *file_info = NULL;
    size_t mru_entries_num = 0;
    int found = 0;
    // not for appending but for atomic create and read
    FILE *fp = fopen(mru_path, "a+");
    if (fp == NULL) {
        deinit_str_pool(pool);
        return 0;
    }
    fseek(fp, 0, SEEK_SET);
    load_mru_to_file_info(&pool, &file_info, &mru_entries_num, fp);
    fclose(fp);

    for (size_t i = 0; i < mru_entries_num; i++) {
        if (strcmp(file_info[i].file_path, path) == 0) {
            file_info[i].mru_score++;
            found = 1;
        }
    }
    if (found == 0) {
        file_info = realloc(file_info, sizeof(file_info_t) * (mru_entries_num + 1));
        file_info[mru_entries_num].mru_score = 1;
        file_info[mru_entries_num].file_path = str(&pool, path);
        mru_entries_num++;
    } else {
        qsort(file_info, mru_entries_num, sizeof(file_info_t), mru_score_cmp);
    }

    // For writing file from scratch
    FILE *wfp = fopen(mru_path, "w");
    if (wfp == NULL) {
        free(file_info);
        deinit_str_pool(pool);
        return 0;
    }
    for (size_t i = 0; i < mru_entries_num; i++) {
        fprintf(wfp, "%s:%i\n", file_info[i].file_path, file_info[i].mru_score);
    }
    free(file_info);
    deinit_str_pool(pool);
    fclose(wfp);
    return mru_entries_num;
}

void deinit_mru() {
    if (g_mru_cache != NULL) {
        free(g_mru_cache);
        g_mru_cache = NULL;
        g_mru_len = 0;
    }
    if (g_str_pool != NULL) {
        deinit_str_pool(g_str_pool);
    }
}

int init_mru(const char *mru_path) {
    deinit_mru();
    g_str_pool = init_str_pool(10240);
    int ret = -1;
    if (access(mru_path, F_OK) != 0) {
        send_res_from_file_info("mru", NULL, 0);
        return ret;
    }
    FILE *fp = fopen(mru_path, "r");
    if (fp != NULL) {
        ret = load_mru_to_file_info(&g_str_pool, &g_mru_cache, &g_mru_len, fp);
        if (ret > 0) {
            send_res_from_file_info("mru", g_mru_cache, g_mru_len);
        }
        fclose(fp);
    }
    return ret;
}

static void after_fuzzy_mru_search(uv_work_t *req, int status) {
    (void)status;
    search_data_t *search_data = (search_data_t *)req->data;
    free(search_data);
    free(req);
}

static void fuzzy_mru_search(uv_work_t *req) {
    search_data_t *search_data = (search_data_t *)req->data;
    if (strlen(search_data->value) == 0) {
        init_mru(search_data->mru_path);
    } else {
        start_fuzzy_response(search_data->value, "mru", search_data->file_info, search_data->file_info_len);
    }
    toggle_mru_init(0);
}

int queue_mru_search(uv_loop_t *loop, const char *value, const char *mru_path) {
    toggle_mru_init(1);
    uv_work_t *req = malloc(sizeof(uv_work_t));
    search_data_t *search_data = malloc(sizeof(search_data_t));
    strcpy(search_data->value, value);
    search_data->value[strlen(value) + 1] = '\0';
    search_data->file_info_len = g_mru_len;
    search_data->file_info = g_mru_cache;
    strcpy(search_data->mru_path, mru_path);
    req->data = search_data;
    int ret = uv_queue_work(loop, req, fuzzy_mru_search, after_fuzzy_mru_search);
    if (ret != 0) {
        free(search_data);
        free(req);
        return -1;
    }
    return 0;
}
