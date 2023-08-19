#include "json_msg_handler.h"
#include "fuzzy.h"
#include "mru.h"
#include "search_helper.h"
#include <jsmn.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>
#include "timer.h"
#ifndef _WIN32
#include "yank.h"
#include "grep.h"
#endif


static int cancel = 0;
static int job = 0;

int is_cancel_requested(void) {
    return cancel;
}

static int is_job_ongoing(void) {
    return job;
}

static void request_cancel(void) {
#ifndef _WIN32
    cancel_grep();
#endif
    cancel = 1;
}

static void reset_cancel(void) {
    cancel = 0;
}

void job_started(void) {
    job = 1;
}

void job_done(void) {
    job = 0;
}

#define MAX_JSON_TOKENS 128
#define MAX_JSON_ELM_SIZE PATH_MAX
#define MAX_RESPONSE_SIZE (MAX_RESPONSE_LINES * PATH_MAX)
#define MAX_REAL_RESPONSE_SIZE (MAX_RESPONSE_SIZE * 2)

char *json_escape(const char *json, size_t len, size_t *result_len) {
    char *buf = malloc(len * 2);
    size_t counter = 0;
    for (size_t i = 0; i < len; i++) {
        // only ascii is supported
        if ((json[i] & 192) >= 192) {
            continue;
        } else if (json[i] == '\n') {
            buf[counter] = '\\';
            buf[++counter] = 'n';
        } else if (json[i] == '\r') {
            buf[counter] = '\\';
            buf[++counter] = 'r';
        } else if (json[i] == '\t') {
            buf[counter] = '\\';
            buf[++counter] = 't';
        } else if (json[i] == '"') {
            buf[counter] = '\\';
            buf[++counter] = '"';
        } else if (json[i] == '\\') {
            buf[counter] = '\\';
            buf[++counter] = '\\';
        } else {
            buf[counter] = json[i];
        }
        counter++;
    }
    buf[counter] = '\0';
    *result_len = counter;
    return buf;
}

static int create_res_json(const char *cmd, file_info_t *file_info, size_t size, char *buf, int seq) {
    if (size == 0) {
        return sprintf(buf, "{\"id\": %i, \"cmd\": \"%s\", \"result\": []}\n", seq, cmd);
    }
    int json_size = sprintf(buf, "{\"id\": %i, \"cmd\": \"%s\", \"result\": [", seq, cmd);
    for (size_t i = 0; i < size; i++) {
        int add_size = sprintf(&buf[json_size], "{\"name\": \"%s\", \"match_pos\": %lu},", file_info[i].file_path, file_info[i].match_pos_flag);
        json_size += add_size;
    }
    return sprintf(&buf[json_size - 1], "]}\n");
}

void send_res_from_file_info(const char *cmd, file_info_t *file_info, size_t size, int seq) {
    static char json_res_str[MAX_REAL_RESPONSE_SIZE];
    memset(json_res_str, 0, (((long)MAX_REAL_RESPONSE_SIZE)));

    size_t send_sz = size;
    if (size > MAX_RESPONSE_LINES) {
        send_sz = MAX_RESPONSE_LINES;
    }
    int j_res = create_res_json(cmd, file_info, send_sz, json_res_str, seq);
    if (j_res >= 0) {
        static char stdout_buf[MAX_REAL_RESPONSE_SIZE];
        memset(stdout_buf, 0, (long)MAX_REAL_RESPONSE_SIZE);
        setvbuf(stdout, stdout_buf, _IOFBF, (long)MAX_REAL_RESPONSE_SIZE);
        fprintf(stdout, "Content-Length: %i\r\n\r\n%s\n", j_res, json_res_str);
        fflush(stdout);
    }
}

static int json_eq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start && strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

typedef struct handle_json_msg_arg_T {
    uv_loop_t *loop;
    char json_msg[MAX_VIM_INPUT];
} handle_json_msg_arg_T;

static void handle_json_msg_again_cb(void *data) {
    // fprintf(stderr, "again\n");
    handle_json_msg_arg_T *timer_arg = (handle_json_msg_arg_T *)data;
    reset_cancel();
    handle_json_msg(timer_arg->loop, timer_arg->json_msg);
    free(timer_arg);
}

static int job_ongoing_cb(void *data) {
    (void)data;
    // request_cancel();
    return is_job_ongoing() == 0;
}

static int current_seq = 0;
void handle_json_msg(uv_loop_t *loop, const char *json_str) {
    jsmn_parser j_parser;
    jsmn_init(&j_parser);
    jsmntok_t j_tokens[MAX_JSON_TOKENS];
    memset(j_tokens, 0, sizeof(jsmntok_t) * MAX_JSON_TOKENS);
    char cmd[MAX_JSON_ELM_SIZE] = {0};
    char value[MAX_JSON_ELM_SIZE] = {0};
    char mru_path[PATH_MAX] = {0};
    char yank_path[PATH_MAX] = {0};
    char list_cmd[PATH_MAX] = {0};
    int seq = 0;
    int ret = jsmn_parse(&j_parser, json_str, strlen(json_str) + 1, j_tokens, sizeof(j_tokens) / sizeof(j_tokens[0]));
    if (ret == 0) {
        return;
    }
    // easy parsing. No depth guarantee.
    for (int i = 0; i < MAX_JSON_TOKENS; i++) {
        if (json_eq(json_str, &j_tokens[i], "cmd") == 0) {
            jsmntok_t *next_tok = &j_tokens[++i];
            strncpy(cmd, json_str + next_tok->start, next_tok->end - next_tok->start);
        } else if (json_eq(json_str, &j_tokens[i], "value") == 0) {
            jsmntok_t *next_tok = &j_tokens[++i];
            strncpy(value, json_str + next_tok->start, next_tok->end - next_tok->start);
        } else if (json_eq(json_str, &j_tokens[i], "mru_path") == 0) {
            jsmntok_t *next_tok = &j_tokens[++i];
            strncpy(mru_path, json_str + next_tok->start, next_tok->end - next_tok->start);
        } else if (json_eq(json_str, &j_tokens[i], "yank_path") == 0) {
            jsmntok_t *next_tok = &j_tokens[++i];
            strncpy(yank_path, json_str + next_tok->start, next_tok->end - next_tok->start);
        } else if (json_eq(json_str, &j_tokens[i], "list_cmd") == 0) {
            jsmntok_t *next_tok = &j_tokens[++i];
            strncpy(list_cmd, json_str + next_tok->start, next_tok->end - next_tok->start);
        } else if (j_tokens[i].type == JSMN_PRIMITIVE) {
            jsmntok_t *next_tok = &j_tokens[i];
            char int_str[100] = {0};
            strncpy(int_str, json_str + next_tok->start, next_tok->end - next_tok->start);
            seq = atoi(int_str);
        }
    }

    if (seq >= current_seq) {
        current_seq = seq;
        // fprintf(stderr, "not skip seq %i %s\n", seq, list_cmd);
    } else {
        // fprintf(stderr, "skip seq %i %s\n", seq, list_cmd);
        return;
    }
    // fprintf(stderr, "seq %i\n", seq);
    if (is_job_ongoing()) {
        // fprintf(stderr, "timer seq %i %s\n", seq, list_cmd);
        request_cancel();
        handle_json_msg_arg_T *timer_arg = calloc(1, sizeof(handle_json_msg_arg_T));
        strcpy(timer_arg->json_msg, json_str);
        timer_arg->loop = loop;
        timer_cond_schedule(loop, job_ongoing_cb, handle_json_msg_again_cb, NULL, timer_arg, 10);
        return;
    }




    // No safety here as well, vimscript must set it correctly
    if (strcmp(cmd, "init_file") == 0 || strcmp(cmd, "file") == 0 || strcmp(cmd, "init_path") == 0 || strcmp(cmd, "path") == 0) {
        queue_search(loop, cmd, value, list_cmd, seq);
    } else if (strcmp(cmd, "init_mru") == 0) {
        queue_mru_search(loop, "", mru_path, seq);
    } else if (strcmp(cmd, "write_mru") == 0) {
        write_mru(mru_path, value);
    } else if (strcmp(cmd, "mru") == 0) {
        queue_mru_search(loop, value, mru_path, seq);
#ifndef _WIN32
    } else if (strcmp(cmd, "grep") == 0) {
        queue_grep(loop, cmd, list_cmd, seq);
    } else if (strcmp(cmd, "init_yank") == 0) {
        queue_yank_search(loop, "", yank_path, seq);
    } else if (strcmp(cmd, "yank") == 0) {
        queue_yank_search(loop, value, yank_path, seq);
#endif
    }
}

