#ifndef JSON_MSG_HANDLER_H
#define JSON_MSG_HANDLER_H

#include "logger.h"
#include "search_helper.h"
#include <uv.h>

#define MAX_VIM_INPUT 65536
#define MAX_RESPONSE_LINES 100
#define MAX_CMD 32

typedef void (*res_cb_t)(void *userdata);

typedef struct response_t {
    char path_list[MAX_RESPONSE_LINES][PATH_MAX];
    size_t path_num;
    char cmd[MAX_CMD];
} response_t;

void deinit_handlers(void);
void handle_json_msg(uv_loop_t *loop, const char *json_str);
void send_res_from_file_info(const char *cmd, file_info_t *file_info, size_t size, int seq);
void init_handlers(void);
char *json_escape(const char *json, size_t len, size_t *result_len);

#endif /* JSON_MSG_HANDLER_H */
