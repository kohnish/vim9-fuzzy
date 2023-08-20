#include "fuzzy.h"
#include "grep.h"
#include "json_msg_handler.h"
#include "mru.h"
#include "search_helper.h"
#include "vim9_fuzzy_env.h"
#include <gtest/gtest.h>
#include <string>

TEST(fuzzy, grep) {
    static uv_loop_t loop;
    uv_loop_init(&loop);
    const char *cmd = "rg --max-depth=2 --max-filesize 1M --color=never -Hn --no-heading j ~/linux";
    int ret = queue_grep(&loop, "grep", cmd, 1);
    uv_run(&loop, UV_RUN_DEFAULT);
    ASSERT_EQ(ret, 0);
}
