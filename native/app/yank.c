#include "yank.h"
#include "b64.h"
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

static size_t load_yank_to_file_info(str_pool_t ***str_pool, file_info_t **file_info, size_t *result_len, FILE *fp) {
    static size_t current_size = INITIAL_CACHE_SIZE;
    if (*file_info == NULL) {
        *file_info = malloc(sizeof(file_info_t) * current_size);
    }
    int current_mode = 0;
    int key_counter = 0;
    size_t line_counter = 0;
    size_t value_counter = 0;
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
            size_t len = strlen(key_buf) + 1;
            key_buf[len] = '\0';
            size_t b_len;
            unsigned char *orig_line = base64_decode((unsigned char *)key_buf, len, &b_len);
            (*file_info)[line_counter].file_path = pool_str_with_len(str_pool, (char *)orig_line, b_len - 1);
            free(orig_line);
            // size_t b_len;
            // unsigned char *orig_line = base64_decode((unsigned char *)key_buf, len, &b_len);
            // size_t j_len;
            // char *j = json_escape((const char *)orig_line, b_len, &j_len);
            // (*file_info)[line_counter].file_path = pool_str(str_pool, (char *)j);
            // free(orig_line);
            // free(j);
            (*file_info)[line_counter].file_name = (*file_info)[line_counter].file_path;
            (*file_info)[line_counter].f_len = strlen((*file_info)[line_counter].file_name);
            memset(key_buf, 0, PATH_MAX);
            key_counter = 0;
        } else if (c == '\n') {
            current_mode = 0;
            (*file_info)[line_counter].yank_score = atoi(val_buf);
            memset(val_buf, 0, PATH_MAX);
            line_counter++;
            value_counter = 0;
        } else if (c == EOF) {
            break;
        } else {
            if (current_mode == 0) {
                key_buf[key_counter] = c;
                key_counter++;
            } else {
                val_buf[value_counter] = c;
                value_counter++;
            }
        }
    }
    *result_len = line_counter;
    return line_counter;
}

static int yank_score_cmp(const void *s1, const void *s2) {
    int v1 = ((file_info_t *)s1)->yank_score;
    int v2 = ((file_info_t *)s2)->yank_score;

    return v1 == v2 ? 0 : v1 > v2 ? -1 : 1;
}

size_t write_yank(const char *yank_path, const char *path) {
    str_pool_t **pool = init_str_pool(10240);
    file_info_t *file_info = NULL;
    size_t yank_entries_num = 0;
    int found = 0;
    // not for appending but for atomic create and read
    FILE *fp = fopen(yank_path, "a+");
    if (fp == NULL) {
        deinit_str_pool(pool);
        return 0;
    }
    fseek(fp, 0, SEEK_SET);
    load_yank_to_file_info(&pool, &file_info, &yank_entries_num, fp);
    fclose(fp);

    for (size_t i = 0; i < yank_entries_num; i++) {
        if (strcmp(file_info[i].file_path, path) == 0) {
            file_info[i].yank_score = time(NULL);
            found = 1;
        }
    }
    if (found == 0) {
        file_info = realloc(file_info, sizeof(file_info_t) * (yank_entries_num + 1));
        file_info[yank_entries_num].yank_score = time(NULL);
        file_info[yank_entries_num].file_path = pool_str(&pool, path);
        yank_entries_num++;
    }
    qsort(file_info, yank_entries_num, sizeof(file_info_t), yank_score_cmp);

    // For writing file from scratch
    FILE *wfp = fopen(yank_path, "w");
    if (wfp == NULL) {
        free(file_info);
        deinit_str_pool(pool);
        return 0;
    }
    for (size_t i = 0; i < yank_entries_num; i++) {
        size_t l = 0;
        unsigned char *base64_yank = base64_encode((unsigned char *)file_info[i].file_path, strlen(file_info[i].file_path), &l);
        if (l > 0) {
            fprintf(wfp, "%s:%zu\n", (const char *)base64_yank, file_info[i].yank_score);
        }
        free(base64_yank);
    }
    free(file_info);
    deinit_str_pool(pool);
    fclose(wfp);
    return yank_entries_num;
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
    if (access(yank_path, F_OK) != 0) {
        send_res_from_file_info("yank", NULL, 0);
        return ret;
    }
    FILE *fp = fopen(yank_path, "r");
    if (fp != NULL) {
        ret = load_yank_to_file_info(&g_str_pool, &g_yank_cache, &g_yank_len, fp);
        if (ret > 0) {
            send_res_from_file_info("yank", g_yank_cache, g_yank_len);
        }
        fclose(fp);
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
