#ifndef JSON_MSG_HANDLER_H
#define JSON_MSG_HANDLER_H

#include "search_helper.h"
#include <uv.h>

#define MAX_VIM_INPUT 65536
#define MAX_RESPONSE_LINES 100
#define MAX_CMD 32

#ifdef __cplusplus
extern "C" {
#endif

void handle_json_msg(uv_loop_t *loop, const char *json_str);
void send_res_from_file_info(const char *cmd, file_info_t *file_info, size_t size, int seq);
char *json_escape(const char *json, size_t len, size_t *result_len);
int is_cancel_requested(void);
void job_started(void);
void job_done(void);

#ifdef __cplusplus
}
#endif
#endif /* JSON_MSG_HANDLER_H */
