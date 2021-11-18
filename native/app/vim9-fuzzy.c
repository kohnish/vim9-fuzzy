#include "fuzzy.h"
#include "json_msg_handler.h"
#include "mru.h"
#include "search_helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

static void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    (void)handle;
    (void)suggested_size;
    static char buf_base[MAX_VIM_INPUT];
    memset(buf_base, 0, MAX_VIM_INPUT);
    *buf = uv_buf_init(buf_base, MAX_VIM_INPUT);
}

static void read_stdin(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    (void)nread;
    static char line[MAX_VIM_INPUT] = {0};
    memset(line, 0, MAX_VIM_INPUT);
    int counter = 0;
    for (size_t i = 0; i < strlen(buf->base); i++) {
        if (buf->base[i] == '\n') {
            handle_json_msg(stream->loop, line);
            memset(line, 0, MAX_VIM_INPUT);
            counter = 0;
        } else {
            line[counter] = buf->base[i];
            counter++;
        }
    }
}

static void on_signal(uv_signal_t *handle, int signum) {
    if (signum == SIGINT) {
        uv_stop(handle->loop);
    }
}

int main() {
    static uv_loop_t loop;
    static uv_tty_t tty_handle;
    static uv_signal_t sig_handle;

    uv_loop_init(&loop);
    init_handlers();

    uv_tty_init(&loop, &tty_handle, STDIN_FILENO, 1);
    uv_read_start((uv_stream_t *)&tty_handle, alloc_buffer, read_stdin);

    uv_signal_init(&loop, &sig_handle);
    uv_signal_start(&sig_handle, on_signal, SIGINT);

    uv_run(&loop, UV_RUN_DEFAULT);
    deinit_handlers();

    return 0;
}
