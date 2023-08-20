#include "grep.h"
#include "fuzzy.h"
#include "json_msg_handler.h"
#include "search_helper.h"
#include "str_pool.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

static uv_process_t *g_child_req = NULL;

static void handle_grep_out(proc_ctx_T *ctx, char *lines, size_t sz) {
    // ToDo: do this task in non-main thread
    if (is_cancel_requested()) {
        // fprintf(stderr, "read cancelled\n");
        return;
    }
    (void)sz;
    const char s[] = "\r\n";
    char *rest = NULL;
    char *token = strtok_r(lines, s, &rest);

    while (token != NULL) {
        // give up too long lines
        size_t line_len = strnlen(token, 600);
        if (line_len > 512) {
            token = strtok_r(NULL, s, &rest);
            continue;
        }
        if (ctx->file_res_size >= ctx->file_res_buf_size) {
            ctx->file_res_buf_size = ctx->file_res_buf_size * 2;
            ctx->file_res = realloc(ctx->file_res, sizeof(file_info_t) * ctx->file_res_buf_size);
        }
        size_t escaped_len;
        char *escaped_line = json_escape(token, line_len, &escaped_len);
        ctx->file_res[ctx->file_res_size].file_path = pool_str(&ctx->str_pool, escaped_line);
        free(escaped_line);
        ctx->file_res[ctx->file_res_size].match_pos_flag = 0;
        ctx->file_res_size = ctx->file_res_size + 1;
        token = strtok_r(NULL, s, &rest);
    }
}

char **cmd_to_str_arr(const char *cmdline, str_pool_t ***str_pool) {
    // ToDo: realloc later
    char **arr = malloc(sizeof(char *) * 32);
    size_t word_num = 0;
    size_t word_begin_pos = 0;
    const char *cmdline_ptr = cmdline;
    for (size_t i = 0; i < strlen(cmdline); i++) {
        if (*cmdline_ptr == ' ' || *cmdline == '\0') {
            arr[word_num] = pool_str_with_len(str_pool, &cmdline[word_begin_pos], i - word_begin_pos - 1);
            word_num++;
            word_begin_pos = i + 1;
        }
        cmdline_ptr++;
    }
    arr[word_num] = pool_str(str_pool, &cmdline[word_begin_pos]);
    arr[word_num + 1] = NULL;
    return arr;
}

void on_close(uv_handle_t *handle) {
    free(handle);
}

static void on_proc_exit(uv_process_t *req, int64_t exit_status, int term_signal) {
    (void)exit_status;
    (void)term_signal;
    // fprintf(stderr, "WIP: proc exit %llu %i\n", exit_status, term_signal);
    proc_ctx_T *ctx = (proc_ctx_T *)req->data;
    // fprintf(stderr, "on exit size %zu\n", ctx->file_res_size);
    if (term_signal != SIGINT) {
        send_res_from_file_info("grep", ctx->file_res, ctx->file_res_size, ctx->seq);
    } else {
        // fprintf(stderr, "sig int\n");
    }
    uv_close((uv_handle_t *)req, on_close);
    free(ctx->file_res);
    deinit_str_pool(ctx->str_pool);
    free(ctx);
    g_child_req = NULL;
    job_done();
}

void alloc_buffer(uv_handle_t *handle, size_t len, uv_buf_t *buf) {
    (void)handle;
    (void)len;
    // fprintf(stderr, "len %zu\n", len);
    static char buf_base[MAX_VIM_INPUT];
    memset(buf_base, 0, MAX_VIM_INPUT);
    *buf = uv_buf_init(buf_base, MAX_VIM_INPUT);
    // buf->base = (char *)malloc(len);
    // buf->len = len;
}

void read_pipe(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    proc_ctx_T *ctx = stream->data;
    // printf("WIP: handle event\n");
    if (nread > 0) {
        // fprintf(stderr, "suggested read size %zu\n", nread);
        handle_grep_out(ctx, buf->base, buf->len);
        // send_res_from_file_info("grep", ctx->file_res, ctx->file_res_size, ctx->seq);
    }
    // fprintf(stderr, "on read size %zu\n", ctx->file_res_size);
    // if (buf->base) {
    //     free(buf->base);
    // }
}

void cancel_grep(void) {
    if (g_child_req != NULL) {
        // fprintf(stderr, "pid %i\n", uv_process_get_pid(g_child_req));
        
        uv_kill(uv_process_get_pid(g_child_req), SIGINT);
        char buf[PATH_MAX] = {0};
        sprintf(buf, "kill -2 `pgrep -P %i` 2>/dev/null", uv_process_get_pid(g_child_req));
        // sprintf(buf, "kill %i", uv_process_get_pid(g_child_req));
        // sprintf(buf, "kill -9 %i", uv_process_get_pid(g_child_req));
        system(buf);
        // job_done();
        // uv_process_kill(g_child_req, SIGINT);
    }
}

int queue_grep(uv_loop_t *loop, const char *cmd, const char *list_cmd, int seq) {
    job_started();
    (void)cmd;
    // fprintf(stderr, "list_cmd %s\n", list_cmd);
    assert(g_child_req == NULL);
    g_child_req = (uv_process_t *)calloc(1, sizeof(uv_process_t));
    static uv_process_options_t options;
    static uv_pipe_t pipe;

    options.exit_cb = on_proc_exit;

    str_pool_t **str_pool = init_str_pool(10240);
    char *args[4];
    args[0] = "sh";
    args[1] = "-c";
    args[2] = (char *)list_cmd;
    args[3] = NULL;
    
    // char **args = cmd_to_str_arr(list_cmd, &str_pool);
    options.file = args[0];
    options.args = args;

    uv_pipe_init(loop, &pipe, 0);

    options.stdio_count = 3;
    static uv_stdio_container_t child_stdio[3];

    child_stdio[0].flags = UV_IGNORE;
    child_stdio[1].flags = (uv_stdio_flags)(UV_CREATE_PIPE | UV_WRITABLE_PIPE | UV_NONBLOCK_PIPE);
    child_stdio[2].flags = UV_IGNORE;
    child_stdio[1].data.stream = (uv_stream_t *)&pipe;
    options.stdio = child_stdio;

    proc_ctx_T *ctx = (proc_ctx_T *)calloc(1, sizeof(proc_ctx_T));
    ctx->seq = seq;
    ctx->str_pool = str_pool;
    ctx->file_res = (file_info_t *)malloc(sizeof(file_info_t) * 100);
    ctx->file_res_size = 0;
    ctx->file_res_buf_size = 100;

    g_child_req->data = ctx;
    pipe.data = ctx;
    int r;
    // fprintf(stderr, "uv_spawning  %s\n", list_cmd);
    if ((r = uv_spawn(loop, g_child_req, &options))) {
        fprintf(stderr, "uv_spawn %s\n", uv_strerror(r));
        deinit_str_pool(str_pool);
        free(ctx->file_res);
        free(g_child_req);
        free(ctx);
        g_child_req = NULL;
        // job_done();
        return r;
    }
    r = uv_read_start((uv_stream_t *)&pipe, alloc_buffer, read_pipe);
    if (r) {
        // job_done();
        g_child_req = NULL;
        fprintf(stderr, "uv_read_start %s\n", uv_strerror(r));
    }
    return r;
}
