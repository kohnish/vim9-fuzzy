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
#define MAX_REAL_RESPONSE_SIZE (MAX_RESPONSE_SIZE * 2)
#define RES_SLEEP_SEC 1500

// static uv_async_t g_async;
// static list_t *g_res_list;
// static uv_mutex_t g_mutex;

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

//static void respond_in_main_thread(uv_async_t *handle) {
//    list_t *data = (list_t *)handle->data;
//    static char stdout_buf[MAX_REAL_RESPONSE_SIZE];
//    while (data->entry != NULL) {
//        char *json_str = (char *)data->entry;
//        memset(stdout_buf, 0, MAX_REAL_RESPONSE_SIZE);
//        setvbuf(stdout, stdout_buf, _IOFBF, MAX_REAL_RESPONSE_SIZE);
//        fprintf(stdout, "%s\n", json_str);
//        fflush(stdout);
//        // uv_mutex_lock(&g_mutex);
//        // list_del_entry((list_t *)handle->data, json_str);
//        // uv_mutex_unlock(&g_mutex);
//        free(json_str);
//        // ToDo: Fix vim getting broken json without sleep.
//        // usleep(RES_SLEEP_SEC);
//    }
//}

void send_res_from_file_info(const char *cmd, file_info_t *file_info, size_t size) {
    char *json_res_str = malloc(MAX_REAL_RESPONSE_SIZE);
    memset(json_res_str, 0, MAX_REAL_RESPONSE_SIZE);

    size_t send_sz = size;
    if (size > MAX_RESPONSE_LINES) {
        send_sz = MAX_RESPONSE_LINES;
    }
    int j_res = create_res_json(cmd, file_info, send_sz, json_res_str);
    if (j_res >= 0) {
        // uv_mutex_lock(&g_mutex);
        // list_append(g_res_list, json_res_str);
        // uv_mutex_unlock(&g_mutex);
        // uv_async_send(&g_async);
        static char stdout_buf[MAX_REAL_RESPONSE_SIZE];
        memset(stdout_buf, 0, MAX_REAL_RESPONSE_SIZE);
        setvbuf(stdout, stdout_buf, _IOFBF, MAX_REAL_RESPONSE_SIZE);
        fprintf(stdout, "%s\n", json_res_str);
        fflush(stdout);
    }
    free(json_res_str);
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
    char root_dir[PATH_MAX] = {0};
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
        } else if (json_eq(json_str, &j_tokens[i], "root_dir") == 0) {
            jsmntok_t *next_tok = &j_tokens[i + 1];
            strncpy(root_dir, json_str + next_tok->start, next_tok->end - next_tok->start);
            i++;
        }
    }

    // No safety here as well, vimscript must set it correctly
    if (strcmp(cmd, "init_file") == 0) {
        return queue_search(loop, "", root_dir);
    } else if (strcmp(cmd, "file") == 0) {
        return queue_search(loop, value, root_dir);
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
    // uv_async_init(loop, &g_async, respond_in_main_thread);
    // uv_mutex_init(&g_mutex);
    // g_res_list = list_create();
    // g_async.data = g_res_list;
}

void deinit_handlers() {
    // uv_close((uv_handle_t *)&g_async, NULL);
    deinit_file();
    deinit_mru();
    deinit_file_mutex();
    deinit_mru_mutex();
    deinit_cancel_mutex();
    // list_t *data = g_res_list;
    // while (data->entry != NULL) {
    //     char *json_str = (char *)data->entry;
    //     list_del_entry(g_res_list, json_str);
    //     free(json_str);
    // }
    // free(g_res_list);
}
