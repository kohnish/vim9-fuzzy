#define _GNU_SOURCE /* Required for strcasestr. */
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "fuzzy.h"
#include "json_msg_handler.h"
#include "search_helper.h"

#undef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define INITIAL_RES_NUM 500

typedef struct search_result_t {
    int32_t score;
    char *match_pos;
} search_result_t;

typedef struct search_query_t {
    const char *line;
    size_t line_len;
    const char *search_word;
    search_result_t result;
} search_query_t;

static uv_mutex_t cancel_mutex;
static int cancel = 0;

void init_cancel_mutex() {
    uv_mutex_init(&cancel_mutex);
}

void deinit_cancel_mutex() {
    uv_mutex_destroy(&cancel_mutex);
}

void toggle_cancel(int val) {
    uv_mutex_lock(&cancel_mutex);
    cancel = val;
    uv_mutex_unlock(&cancel_mutex);
}

static int32_t compute_score(int32_t jump, bool first_char, const char *restrict match) {
    const int adjacency_bonus = 15;
    const int separator_bonus = 30;
    const int camel_bonus = 30;
    const int first_letter_bonus = 15;

    const int leading_letter_penalty = -5;
    const int max_leading_letter_penalty = -15;

    int32_t score = 0;

    /* Apply bonuses. */
    if (!first_char && jump == 0) {
        score += adjacency_bonus;
    }
    if (!first_char || jump > 0) {
        if (isupper(*match) && islower(*(match - 1))) {
            score += camel_bonus;
        }
        if (isalnum(*match) && !isalnum(*(match - 1))) {
            score += separator_bonus;
        }
    }
    if (first_char && jump == 0) {
        /* Match at start of string gets separator bonus. */
        score += first_letter_bonus;
    }

    /* Apply penalties. */
    if (first_char) {
        score += MAX(leading_letter_penalty * jump, max_leading_letter_penalty);
    }

    return score;
}

static int32_t fuzzy_match_recurse(const char *restrict pattern, const char *restrict str, search_result_t *result, bool first_char) {
    if (*pattern == '\0') {
        /* We've matched the full pattern. */
        return result->score;
    }

    const char *match = str;
    const char search[2] = {*pattern, '\0'};

    int32_t best_score = INT32_MIN;

    /*
     * Find all occurrences of the next pattern character in str, and
     * recurse on them.
     */
    size_t counter = 0;
    while ((match = strcasestr(match, search)) != NULL) {
        result->match_pos[counter] = '1';
        result->score = compute_score(match - str, first_char, match);
        int32_t subscore = fuzzy_match_recurse(pattern + 1, match + 1, result, false);
        best_score = MAX(best_score, subscore);
        match++;
        ++counter;
    }

    if (best_score == INT32_MIN) {
        /* We couldn't match the rest of the pattern. */
        return INT32_MIN;
    } else {
        return result->score + best_score;
    }
}


/*
 * Returns score if each character in pattern is found sequentially within str.
 * Returns INT32_MIN otherwise.
 */
static int32_t fuzzy_match(search_query_t *query) {
    const int unmatched_letter_penalty = -1;
    const size_t slen = strlen(query->search_word);
    query->result.score = 100;

    if (*query->line == '\0') {
        return query->result.score;
    }
    if (slen < query->line_len) {
        return INT32_MIN;
    }

    /* We can already penalise any unused letters. */
    query->result.score += unmatched_letter_penalty * (int32_t)(slen - query->line_len);

    /* Perform the match. */
    query->result.score = fuzzy_match_recurse(query->line, query->search_word, &query->result, true);

    return query->result.score;
}

static unsigned long binstr_to_int(const char *s) {
    return strtoul(s, NULL, 2);
}

static int result_compare(const void *s1, const void *s2) {
    size_t v1 = ((file_info_t *)s1)->fuzzy_score;
    size_t v2 = ((file_info_t *)s2)->fuzzy_score;
    return v1 == v2 ? 0 : v1 > v2 ? -1 : 1;
}

size_t start_fuzzy_response(const char *search_keyword, const char *cmd, file_info_t *files, size_t len) {
    size_t matched_len = 0;

    size_t current_sz = INITIAL_RES_NUM;
    file_info_t *file_res AUTO_FREE_FILE_INFO = malloc(sizeof(file_info_t) * INITIAL_RES_NUM);

    for (size_t i = 0; i < len; i++) {
        if (cancel == 1) {
            toggle_cancel(0);
            return matched_len;
        }
        files[i].f_len = strlen(files[i].file_name);
        char *match_pos_str AUTO_FREE_STR = malloc(files[i].f_len + 1);
        memset(match_pos_str, '0', files[i].f_len);
        match_pos_str[files[i].f_len] = '\0';
        search_result_t search_result = {.score = 0, .match_pos = match_pos_str};

        // clang-format off
        search_query_t query = {
            .line = files[i].file_name,
            .line_len = files[i].f_len,
            .search_word = search_keyword,
            .result = search_result
        };
        // clang-format on

        fuzzy_match(&query);
        if (query.result.score > 0) {
            if (matched_len >= current_sz) {
                current_sz = current_sz * 2;
                file_res = realloc(file_res, sizeof(file_info_t) * current_sz);
            }
            file_res[matched_len].file_path = files[i].file_path;
            file_res[matched_len].fuzzy_score = query.result.score;
            file_res[matched_len].match_pos_flag = binstr_to_int(query.result.match_pos);
            // printf("WIP: %s\n", query.result.match_pos);
            // fflush(stdout);
            ++matched_len;
        }
    }
    qsort(file_res, matched_len, sizeof(file_info_t), result_compare);
    send_res_from_file_info(cmd, file_res, matched_len);

    return matched_len;
}

