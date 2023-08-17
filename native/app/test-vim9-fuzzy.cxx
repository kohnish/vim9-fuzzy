#include "fuzzy.h"
#include "mru.h"
#include "search_helper.h"
#include "vim9_fuzzy_env.h"
#include <gtest/gtest.h>
#include <string>


// const std::string g_test_str = std::string("aあiい");
const char *s = "aあiい";
// 2 bytes: 110xxxxx 10xxxxxx
// 192
// 3 bytes: 1110xxxx 10xxxxxx 10xxxxxx
// 4 bytes: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

TEST(fuzzy, start_fuzzy_response) {
    if ((s[1] & 192) >= 192) {
        printf("two bytes\n");
    }
    if ((s[1] & 224) >= 224) {
        printf("three bytes\n");
    }
    if ((s[1] & 240) >= 240) {
        printf("four bytes \n");
    }
    // printf("%02X\n", g_test_str.c_str()[0]);
    // printf("%02X\n", g_test_str.c_str()[1]);
    // printf("%02X\n", g_test_str.c_str()[2]);
    // printf("%02X\n", g_test_str.c_str()[3]);
    // printf("%02X\n", g_test_str.c_str()[4]);
    // printf("%02X\n", g_test_str.c_str()[5]);
}
