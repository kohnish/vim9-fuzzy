#include "fuzzy.h"
#include "mru.h"
#include "search_helper.h"
#include "vim9_fuzzy_env.h"
#include <gtest/gtest.h>
#include <string>
#include "grep.h"
#include "json_msg_handler.h"

static void on_proc_exit(uv_process_t *req, int64_t exit_status, int term_signal) {
    (void)exit_status;
    (void)term_signal;
    uv_close((uv_handle_t *)req, NULL);
}

void alloc_buffer(uv_handle_t *handle, size_t len, uv_buf_t *buf) {
    (void)handle;
    // printf("alloc_buffer called, requesting a %lu byte buffer\n");
    buf->base = (char *)malloc(len);
    buf->len = len;
}

void read_apipe(uv_stream_t* stream, ssize_t nread, const  uv_buf_t *buf) {
    (void)stream;
    (void)nread;
    printf("read: |%s|", buf->base);
}

void read_pipe(uv_stream_t* stream, ssize_t nread, const  uv_buf_t *buf) {
    (void)stream;
    (void)nread;
    printf("read: |%s|", buf->base);
}



int my_main(uv_loop_t *loop, const char *cmd, const char *list_cmd, int seq) {
    (void)cmd;
    uv_process_t *child_req = (uv_process_t *)calloc(1, sizeof(uv_process_t));
    uv_process_options_t *options = (uv_process_options_t *)malloc(sizeof(uv_process_options_t));
    uv_pipe_t *pipe = (uv_pipe_t *)malloc(sizeof(uv_pipe_t));

    options->exit_cb = on_proc_exit;

    str_pool_t **str_pool = init_str_pool(1024);
    char **args = cmd_to_str_arr(list_cmd, &str_pool);
    options->file = args[0];
    options->args = args;

    uv_pipe_init(loop, pipe, 0);

    options->stdio_count = 3;
    uv_stdio_container_t *child_stdio = (uv_stdio_container_t *)malloc(sizeof(uv_stdio_container_t) * 3);

    child_stdio[0].flags = UV_IGNORE;
    child_stdio[1].flags = (uv_stdio_flags)(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
    child_stdio[2].flags = UV_IGNORE;
    child_stdio[1].data.stream = (uv_stream_t *)pipe;
    options->stdio = child_stdio;

    proc_ctx_T *ctx = (proc_ctx_T *)malloc(sizeof(proc_ctx_T));
    ctx->seq = seq;
    ctx->str_pool = str_pool;


    int r;
    if ((r = uv_spawn(loop, child_req, options))) {
        fprintf(stderr, "uv %s\n", uv_strerror(r));
        return 1;
    } else {
        fprintf(stderr, "Launched process with ID %d\n", child_req->pid);
    }
    uv_read_start((uv_stream_t*)pipe, alloc_buffer, read_apipe);

    return uv_run(loop, UV_RUN_DEFAULT);
}

int main2(uv_loop_t *loop, const char *cmd, const char *list_cmd, int seq) {
    (void)cmd;
    job_started();

    uv_process_t *child_req = (uv_process_t *)calloc(1, sizeof(uv_process_t));

    str_pool_t **str_pool = init_str_pool(1024);
    char **args = cmd_to_str_arr(list_cmd, &str_pool);

    uv_process_options_t *options = (uv_process_options_t *)calloc(1, sizeof(uv_process_options_t));
    options->file = args[0];
    options->args = args;
    options->exit_cb = on_proc_exit;

    uv_pipe_t *pipe = (uv_pipe_t *)calloc(1, sizeof(uv_pipe_t));
    uv_pipe_init(loop, pipe, 0);

    options->stdio_count = 3;
    uv_stdio_container_t *child_stdio = (uv_stdio_container_t *)calloc(1, sizeof(uv_stdio_container_t) * 3);
    child_stdio[0].flags = UV_IGNORE;
    child_stdio[1].flags = (uv_stdio_flags)(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
    child_stdio[2].flags = UV_IGNORE;
    child_stdio[1].data.stream = (uv_stream_t *)pipe;
    options->stdio = child_stdio;

    proc_ctx_T *ctx = (proc_ctx_T *)malloc(sizeof(proc_ctx_T));
    ctx->seq = seq;
    ctx->str_pool = str_pool;

    // child_req->data = ctx;
    int uv_ret = uv_spawn(loop, child_req, options);
    if (uv_ret != 0) {
        fprintf(stderr, "uv %s\n", uv_strerror(uv_ret));
        free(args);
        free(pipe);
        free(ctx);
        free(options);
        free(child_req);
        // free(child_stdio);
        deinit_str_pool(str_pool);
        job_done();
        return -1;
    }
    // uv_ret = uv_read_start((uv_stream_t*)pipe, alloc_buffer, read_pipe);
    uv_ret = uv_read_start((uv_stream_t*)pipe, alloc_buffer, read_pipe);
    // if (uv_ret != 0) {
    //     fprintf(stderr, "uv read %s\n", uv_strerror(uv_ret));
    // }
    return 0;
}

// TEST(fuzzy, start_fuzzy_response) {
//     // uv_loop_t *loop;
//     // uv_loop_t l;
//     // uv_loop_init(&l);
//     // loop = &l;
//     // my_main(loop, "grep", "ls -l", 1);
//     uv_loop_t loop;
//     uv_loop_init(&loop);
//     main2(&loop, "grep", "ls -l", 1);
// }

TEST(fuzzy, grep) {
    // str_pool_t **str_pool = init_str_pool(1024);
    // const char *cmd_line = "git grep -n 'test'";
    // auto **arr = cmd_to_str_arr(cmd_line, &str_pool);
    // printf("%s\n", arr[0]);
    // printf("%s\n", arr[1]);
    // printf("%s\n", arr[2]);
    static uv_loop_t loop;
    uv_loop_init(&loop);
    const char *cmd = "rg --max-depth=2 --max-filesize 1M --color=never -Hn --no-heading j ~/linux";
    int ret = queue_grep(&loop, "grep", cmd, 1);
    uv_run(&loop, UV_RUN_DEFAULT);
    ASSERT_EQ(ret, 0);
}

