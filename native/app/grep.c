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

static void after_grep_task(uv_work_t *req, int status) {
    (void)status;
    search_data_t *search_data = (search_data_t *)req->data;
    free(search_data);
    free(req);
}

typedef struct search_result_t {
    size_t score;
    int matches[256];
    char *match_pos;
} search_result_t;


static size_t start_grep(const char *list_cmd, const char *cmd, int seq) {
    // use fork and kill it on cancel
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
        if (is_cancel_requested()) {
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
    job_done();
}

char **cmd_to_str_arr(const char *cmdline, str_pool_t ***str_pool, size_t *result_len) {
    char **arr = malloc(sizeof(char *) * 32);
    size_t word_num = 0;
    size_t word_begin_pos = 0;
    const char *cmdline_ptr = cmdline;
    for (size_t i = 0; i < strlen(cmdline); i++) {
        if (*cmdline_ptr == ' ' || *cmdline == '\0') {
            arr[word_num] = pool_str_with_len(str_pool, &cmdline[word_begin_pos], i - word_begin_pos);
            word_num++;
            word_begin_pos = i + 1;
        }
        cmdline_ptr++;
    }
    arr[word_num] = pool_str(str_pool, &cmdline[word_begin_pos]);
    *result_len = word_num;
    return arr;
}

int queue_grep(uv_loop_t *loop, const char *cmd, const char *list_cmd, int seq) {
    job_started();
    uv_work_t *req = malloc(sizeof(uv_work_t));
    search_data_t *search_data = malloc(sizeof(search_data_t));
    search_data->seq_ = seq;
    strcpy(search_data->cmd, cmd);
    strcpy(search_data->list_cmd, list_cmd);
    req->data = search_data;
    int ret = uv_queue_work(loop, req, grep_task, after_grep_task);
    if (ret != 0) {
        free(search_data);
        free(req);
        job_done();
        return -1;
    }
    return 0;
}
