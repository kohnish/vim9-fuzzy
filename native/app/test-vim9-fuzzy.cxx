#include "vim9_fuzzy_env.h"
#include "fuzzy.h"
#include "mru.h"
#include "search_helper.h"
#include <gtest/gtest.h>

TEST(fuzzy, start_fuzzy_response) {
    init_cancel_mutex();

    file_info_t files_list[] = {
        { "path_name", "file_name", 1, 0, 0 },
    };
    size_t match_num = start_fuzzy_response("1", "file", files_list, 1);
    ASSERT_EQ(match_num, 0);
    match_num = start_fuzzy_response("f", "file", files_list, 1);
    ASSERT_GT(match_num, 0);
    match_num = start_fuzzy_response("file", "file", files_list, 1);
    ASSERT_GT(match_num, 0);
    match_num = start_fuzzy_response("file name", "file", files_list, 1);
    ASSERT_GT(match_num, 0);

    deinit_cancel_mutex();
}
