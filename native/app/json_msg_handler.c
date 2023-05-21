#include "json_msg_handler.h"
#include "fuzzy.h"
#include "mru.h"
#include "search_helper.h"
#include "yank.h"
#include <jsmn.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#define MAX_JSON_TOKENS 128
#define MAX_JSON_ELM_SIZE PATH_MAX
#define MAX_RESPONSE_SIZE (MAX_RESPONSE_LINES * PATH_MAX)
#define MAX_REAL_RESPONSE_SIZE (MAX_RESPONSE_SIZE * 2)

char *json_escape(const char *json, size_t len, size_t *result_len) {
    char *buf = malloc(len * 2);
    size_t counter = 0;
    for (size_t i = 0; i < len; i++) {
        if (json[i] == '\n') {
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
        sprintf(buf, "[%i, {\"cmd\": \"%s\", \"result\": []}]\n", seq, cmd);
        return 0;
    }
    int json_size = sprintf(buf, "[%i, {\"cmd\": \"%s\", \"result\": [", seq, cmd);
    for (size_t i = 0; i < size; i++) {
        int add_size = sprintf(&buf[json_size], "{\"name\": \"%s\", \"match_pos\": %lu},", file_info[i].file_path, file_info[i].match_pos_flag);
        json_size += add_size;
    }
    sprintf(&buf[json_size - 1], "]}]\n");
    return json_size;
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
        fprintf(stdout, "%s\n", json_res_str);
        fflush(stdout);
    }
}

static int json_eq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start && strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

void handle_json_msg(uv_loop_t *loop, const char *json_str) {
    if (is_file_search_ongoing() == 1 || is_mru_search_ongoing() == 1) {
        toggle_cancel(1);
        while (is_file_search_ongoing() == 1 || is_mru_search_ongoing() == 1) {
            usleep(1000);
        }
    }

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
    jsmn_parse(&j_parser, json_str, strlen(json_str) + 1, j_tokens, sizeof(j_tokens) / sizeof(j_tokens[0]));
    // easy parsing. No depth guarantee.
    for (int i = 0; i < MAX_JSON_TOKENS; i++) {
        if (json_eq(json_str, &j_tokens[i], "cmd") == 0) {
            jsmntok_t *next_tok = &j_tokens[i + 1];
            strncpy(cmd, json_str + next_tok->start, next_tok->end - next_tok->start);
            i++;
        } else if (json_eq(json_str, &j_tokens[i], "value") == 0) {
            jsmntok_t *next_tok = &j_tokens[i + 1];
            strncpy(value, json_str + next_tok->start, next_tok->end - next_tok->start);
            i++;
        } else if (json_eq(json_str, &j_tokens[i], "mru_path") == 0) {
            jsmntok_t *next_tok = &j_tokens[i + 1];
            strncpy(mru_path, json_str + next_tok->start, next_tok->end - next_tok->start);
            i++;
        } else if (json_eq(json_str, &j_tokens[i], "yank_path") == 0) {
            jsmntok_t *next_tok = &j_tokens[i + 1];
            strncpy(yank_path, json_str + next_tok->start, next_tok->end - next_tok->start);
            i++;
        } else if (json_eq(json_str, &j_tokens[i], "list_cmd") == 0) {
            jsmntok_t *next_tok = &j_tokens[i + 1];
            strncpy(list_cmd, json_str + next_tok->start, next_tok->end - next_tok->start);
            i++;
        } else if (j_tokens[i].type == JSMN_PRIMITIVE) {
            jsmntok_t *next_tok = &j_tokens[i];
            char int_str[100] = { 0 };
            strncpy(int_str, json_str + next_tok->start, next_tok->end - next_tok->start);
            seq = atoi(int_str);
        }
    }

    // No safety here as well, vimscript must set it correctly
    if (strcmp(cmd, "init_file") == 0 || strcmp(cmd, "file") == 0 || strcmp(cmd, "init_path") == 0 || strcmp(cmd, "path") == 0) {
        queue_search(loop, cmd, value, list_cmd, seq);
    } else if (strcmp(cmd, "init_mru") == 0) {
        queue_mru_search(loop, "", mru_path, seq);
    } else if (strcmp(cmd, "write_mru") == 0) {
        write_mru(mru_path, value);
    } else if (strcmp(cmd, "init_yank") == 0) {
        queue_yank_search(loop, "", yank_path, seq);
    } else if (strcmp(cmd, "mru") == 0) {
        queue_mru_search(loop, value, mru_path, seq);
    } else if (strcmp(cmd, "yank") == 0) {
        queue_yank_search(loop, value, yank_path, seq);
    }
}

void init_handlers(void) {
    init_file_mutex();
    init_mru_mutex();
    init_yank_mutex();
    init_cancel_mutex();
}

void deinit_handlers(void) {
    deinit_file();
    deinit_mru();
    deinit_yank();
    deinit_file_mutex();
    deinit_mru_mutex();
    deinit_yank_mutex();
    deinit_cancel_mutex();
}
