#include "vim9_fuzzy_env.h"
#include "fuzzy.h"
#include "mru.h"
#include "search_helper.h"
#include <gtest/gtest.h>
#include <memory>

// ToDo: Fix this
TEST(fuzzy, start_fuzzy_response) {
    init_cancel_mutex();

    file_info_t files_list[] = {
        { "path_name", "file_name", 1, 0, 0, 0 },
        { "native/app/json_msg_handler.c", "json_msg_handler.c", 1, 0, 0, 0 },
    };
    size_t match_num = start_fuzzy_response("1", "file", files_list, 1);
    ASSERT_EQ(match_num, 0);
    match_num = start_fuzzy_response("f", "file", files_list, 1);
    ASSERT_GT(match_num, 0);
    match_num = start_fuzzy_response("file", "file", files_list, 1);
    ASSERT_GT(match_num, 0);
    match_num = start_fuzzy_response("file name", "file", files_list, 1);
    ASSERT_GT(match_num, 0);
    match_num = start_fuzzy_response("i", "file", files_list, 1);
    ASSERT_GT(match_num, 0);
    match_num = start_fuzzy_response("l", "file", files_list, 1);
    ASSERT_GT(match_num, 0);
    //match_num = start_fuzzy_response("fe", "file", files_list, 1);
    match_num = start_fuzzy_response("name", "file", files_list, 1);
    match_num = start_fuzzy_response("file_name", "file", files_list, 1);
    match_num = start_fuzzy_response("jsonmsghandler", "file", files_list, 2);

    deinit_cancel_mutex();
}
