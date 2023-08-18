#include "grep.h"
#include "search_helper.h"
#include "fuzzy.h"
#include "json_msg_handler.h"
#include "str_pool.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

static uv_mutex_t cancel_mutex;
static int cancel = 0;

void toggle_grep_cancel(int val) {
    uv_mutex_lock(&cancel_mutex);
    cancel = val;
    uv_mutex_unlock(&cancel_mutex);
}

static uv_mutex_t file_init_mutex;
static int file_init = 0;

static void toggle_grep_init(int val) {
    uv_mutex_lock(&file_init_mutex);
    file_init = val;
    uv_mutex_unlock(&file_init_mutex);
}

int is_grep_search_ongoing(void) {
    return file_init;
}

void init_grep_mutex(void) {
    uv_mutex_init(&file_init_mutex);
    uv_mutex_init(&cancel_mutex);
}

void deinit_grep_mutex(void) {
    uv_mutex_destroy(&file_init_mutex);
    uv_mutex_destroy(&cancel_mutex);
}

static void after_grep_task(uv_work_t *req, int status) {
    (void)status;
    search_data_t *search_data = (search_data_t *)req->data;
    free(search_data);
    free(req);
    toggle_grep_init(0);
}

typedef struct search_result_t {
    size_t score;
    int matches[256];
    char *match_pos;
} search_result_t;


static size_t start_grep(const char *list_cmd, const char *cmd, int seq) {
    FILE *fp = popen(list_cmd, "r");
    if (fp == NULL) {
        return 0;
    }
    size_t current_sz = 500;
    file_info_t *file_res AUTO_FREE_FILE_INFO = malloc(sizeof(file_info_t) * 500);
    char *line = NULL;
    size_t matched_len = 0;
    size_t len;
    ssize_t read;
    int broken = 0;
    str_pool_t **str_pool = init_str_pool(1024);
    while ((read = getline(&line, &len, fp)) != -1) {
        if (cancel == 1) {
            toggle_grep_cancel(0);
            broken = 1;
            break;
        }
        if (len > 512) {
            continue;
        }

        if (matched_len >= current_sz) {
            current_sz = current_sz * 2;
            file_res = realloc(file_res, sizeof(file_info_t) * current_sz);
        }
        size_t escaped_len;
        char *escaped_line = json_escape(line, strlen(line) - 1, &escaped_len);
        file_res[matched_len].file_path = pool_str(&str_pool, escaped_line);
        free(escaped_line);
        file_res[matched_len].match_pos_flag = 0;
        ++matched_len;
    }
    if (broken != 1) {
        send_res_from_file_info(cmd, file_res, matched_len, seq);
    }
    deinit_str_pool(str_pool);
    pclose(fp);
    return matched_len;
}

static void grep_task(uv_work_t *req) {
    search_data_t *search_data = (search_data_t *)req->data;
    start_grep(search_data->list_cmd, "grep", search_data->seq_);
}

int queue_grep(uv_loop_t *loop, const char *cmd, const char *list_cmd, int seq) {
    uv_work_t *req = malloc(sizeof(uv_work_t));
    search_data_t *search_data = malloc(sizeof(search_data_t));
    search_data->seq_ = seq;
    strcpy(search_data->cmd, cmd);
    strcpy(search_data->list_cmd, list_cmd);
    req->data = search_data;
    toggle_grep_init(1);
    int ret = uv_queue_work(loop, req, grep_task, after_grep_task);
    if (ret != 0) {
        toggle_grep_init(0);
        free(search_data);
        free(req);
        return -1;
    }
    return 0;
}
