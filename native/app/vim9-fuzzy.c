#include "fuzzy.h"
#include "json_msg_handler.h"
#include "mru.h"
#include "search_helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#define LENGTH_HEADER "Content-Length: "
#define LENGTH_HEADER_SZ (sizeof(LENGTH_HEADER) - 1)

static void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    (void)handle;
    (void)suggested_size;
    static char buf_base[MAX_VIM_INPUT];
    memset(buf_base, 0, MAX_VIM_INPUT);
    *buf = uv_buf_init(buf_base, MAX_VIM_INPUT);
}

static void read_stdin(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    static char line[MAX_VIM_INPUT] = {0};
    static int content_len = 0;
    static int start = 0;
    static char json_line[MAX_VIM_INPUT] = {0};
    static int json_counter = 0;
    static int line_counter = 0;
    for (ssize_t i = 0; i <= nread; i++) {
        line[line_counter] = buf->base[i];
        if (buf->base[i] == '\n' && i > 0 && buf->base[i - 1] == '\r') {
            const char *sz = strstr(line, LENGTH_HEADER);
            if (sz) {
                content_len = atoi(sz + LENGTH_HEADER_SZ);
            } else if (strlen(line) == 2) {
                start = 1;
            }
            memset(line, 0, MAX_VIM_INPUT);
            line_counter = 0;
        } else {
            line_counter++;
            if (start == 1) {
                json_line[json_counter] = buf->base[i];
                if (json_counter == content_len) {
                    json_line[json_counter + 1] = '\0';
                    handle_json_msg(stream->loop, json_line);
                    memset(json_line, 0, MAX_VIM_INPUT);
                    memset(line, 0, MAX_VIM_INPUT);
                    start = 0;
                    content_len = 0;
                    json_counter = 0;
                    line_counter = 0;
                    break;
                }
                ++json_counter;
            }
        }
    }
}

static void on_signal(uv_signal_t *handle, int signum) {
    if (signum == SIGINT) {
        uv_stop(handle->loop);
    }
}

int main(void) {
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
