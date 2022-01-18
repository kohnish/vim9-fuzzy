#include "json_msg_handler.h"
#include "fuzzy.h"
#include "list.h"
#include "mru.h"
#include "search_helper.h"
#include <ctype.h>
#include <dirent.h>
#include <jsmn.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <uv.h>
#ifndef _WIN32
#include <fnmatch.h>
#endif

#define MAX_JSON_TOKENS 128
#define MAX_JSON_ELM_SIZE PATH_MAX
#define MAX_RESPONSE_SIZE (MAX_RESPONSE_LINES * PATH_MAX)
#define MAX_REAL_RESPONSE_SIZE (MAX_RESPONSE_SIZE * 2)
#define RES_SLEEP_SEC 1500

static int create_res_json(const char *cmd, file_info_t *file_info, size_t size, char *buf) {
    if (size == 0) {
        sprintf(buf, "{\"cmd\": \"%s\", \"result\": []}\n", cmd);
        return 0;
    }
    int json_size = sprintf(buf, "{\"cmd\": \"%s\", \"result\": [", cmd);
    size_t i = 0;
    for (i = 0; i < size; i++) {
        // ToDo: Fix getting nulled
        if (file_info[i].f_len != 0) {
            int add_size = sprintf(&buf[json_size], "\"%s\",", file_info[i].file_path);
            json_size += add_size;
        }
    }
    if (i == 0) {
        return 0;
    }
    sprintf(&buf[json_size - 1], "]}\n");
    return json_size;
}

void send_res_from_file_info(const char *cmd, file_info_t *file_info, size_t size) {
    static char json_res_str[MAX_REAL_RESPONSE_SIZE];
    memset(json_res_str, 0, MAX_REAL_RESPONSE_SIZE);

    size_t send_sz = size;
    if (size > MAX_RESPONSE_LINES) {
        send_sz = MAX_RESPONSE_LINES;
    }
    int j_res = create_res_json(cmd, file_info, send_sz, json_res_str);
    if (j_res >= 0) {
        static char stdout_buf[MAX_REAL_RESPONSE_SIZE];
        memset(stdout_buf, 0, MAX_REAL_RESPONSE_SIZE);
        setvbuf(stdout, stdout_buf, _IOFBF, MAX_REAL_RESPONSE_SIZE);
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

int handle_json_msg(uv_loop_t *loop, const char *json_str) {
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
    char list_cmd[PATH_MAX] = {0};
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
        } else if (json_eq(json_str, &j_tokens[i], "list_cmd") == 0) {
            jsmntok_t *next_tok = &j_tokens[i + 1];
            strncpy(list_cmd, json_str + next_tok->start, next_tok->end - next_tok->start);
            i++;
        }
    }

    // No safety here as well, vimscript must set it correctly
    if (strcmp(cmd, "init_file") == 0 || strcmp(cmd, "file") == 0 || strcmp(cmd, "init_path") == 0 || strcmp(cmd, "path") == 0) {
        return queue_search(loop, cmd, value, list_cmd);
    } else if (strcmp(cmd, "init_mru") == 0) {
        return queue_mru_search(loop, "", mru_path);
    } else if (strcmp(cmd, "write_mru") == 0) {
        return write_mru(mru_path, value);
    } else if (strcmp(cmd, "mru") == 0) {
        return queue_mru_search(loop, value, mru_path);
    }

    return -1;
}

void init_handlers() {
    init_file_mutex();
    init_mru_mutex();
    init_cancel_mutex();
}

void deinit_handlers() {
    deinit_file();
    deinit_mru();
    deinit_file_mutex();
    deinit_mru_mutex();
    deinit_cancel_mutex();
}
