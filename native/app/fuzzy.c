/*
 * Fuzzy string matching
 * Ported from vim search.c which is also ported from
 * the lib_fts library authored by Forrest Smith.
 * https://github.com/forrestthewoods/lib_fts/tree/master/code
 *
 * The following blog describes the fuzzy matching algorithm:
 * https://www.forrestthewoods.com/blog/reverse_engineering_sublime_texts_fuzzy_match/
 */

#include "fuzzy.h"
#include "json_msg_handler.h"
#include "search_helper.h"
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_RES_NUM 500
#define MAX_FUZZY_MATCHES 256
#define FALSE 0
#define TRUE 1
#define ARRAY_LENGTH(a) (sizeof(a) / sizeof((a)[0]))

// bonus for adjacent matches; this is higher than SEPARATOR_BONUS so that
// matching a whole word is preferred.
#define SEQUENTIAL_BONUS 40
// bonus if match occurs after a path separator
#define PATH_SEPARATOR_BONUS 30
// bonus if match occurs after a word separator
#define WORD_SEPARATOR_BONUS 25
// bonus if match is uppercase and prev is lower
#define CAMEL_BONUS 30
// bonus if the first letter is matched
#define FIRST_LETTER_BONUS 15
// penalty applied for every letter in str before the first match
#define LEADING_LETTER_PENALTY -5
// maximum penalty for leading letters
#define MAX_LEADING_LETTER_PENALTY -15
// penalty for every letter that doesn't match
#define UNMATCHED_LETTER_PENALTY -1
// penalty for gap in matching positions (-2 * k)
#define GAP_PENALTY -2
// Score for a string that doesn't fuzzy match the pattern
#define SCORE_NONE -9999

#define FUZZY_MATCH_RECURSION_LIMIT 10

/*
 * Compute a score for a fuzzy matched string. The matching character locations
 * are in 'matches'.
 */
static size_t fuzzy_match_compute_score(const char *str, size_t strSz, const int *matches, size_t numMatches) {
    size_t score;
    int penalty;
    size_t unmatched;
    // const char *p = str;
    // unsigned int sidx = 0;

    // Initialize score
    score = 100;

    // Apply leading letter penalty
    penalty = LEADING_LETTER_PENALTY * matches[0];
    if (penalty < MAX_LEADING_LETTER_PENALTY) {
        penalty = MAX_LEADING_LETTER_PENALTY;
    }
    score += penalty;

    // Apply unmatched penalty
    unmatched = strSz - numMatches;
    score += UNMATCHED_LETTER_PENALTY * unmatched;

    // Apply ordering bonuses
    for (size_t i = 0; i < numMatches; ++i) {
        size_t currIdx = matches[i];

        if (i > 0) {
            size_t prevIdx = matches[i - 1];

            // Sequential
            if (currIdx == (prevIdx + 1)) {
                score += SEQUENTIAL_BONUS;
            } else {
                score += GAP_PENALTY * (currIdx - prevIdx);
            }
        }

        // Check for bonuses based on neighbor character value
        if (currIdx > 0) {
            // Camel case
            unsigned char neighbor = ' ';
            // int curr;

            neighbor = str[currIdx - 1];
            // curr = str[currIdx];

            // if (vim_islower(neighbor) && vim_isupper(curr))
            //     score += CAMEL_BONUS;

            // Bonus if the match follows a separator character
            if (neighbor == '/' || neighbor == '\\') {
                score += PATH_SEPARATOR_BONUS;
            } else if (neighbor == ' ' || neighbor == '_') {
                score += WORD_SEPARATOR_BONUS;
            }
        } else {
            // First letter
            score += FIRST_LETTER_BONUS;
        }
    }
    return score;
}

typedef struct search_result_t {
    size_t score;
    int matches[MAX_FUZZY_MATCHES];
} search_result_t;

typedef struct search_query_t {
    const char *line;
    const char *search_word;
    size_t line_len;
    size_t max_matches;
    search_result_t result;
} search_query_t;

typedef struct search_state_t {
    int next_match;
    const int max_matches;
    int line_begin;
    int *src_matches;
    int matches[MAX_FUZZY_MATCHES];
    int score;
    int recursion_count;
    int idx;
} search_state_t;


// clang-format off
static size_t fuzzy_match_recursive(
        const char *search_word,
        const char *line,
        int str_idx,
        size_t *out_score,
        const char *whole_word,
        size_t strLen,
        int *src_matches,
        int *matches,
        size_t max_matches,
        size_t next_match,
        int *recursion_count) {
// clang-format on
    int recursiveMatch = FALSE;
    int bestRecursiveMatches[MAX_FUZZY_MATCHES];
    size_t bestRecursiveScore = 0;
    int first_match;
    int matched;

    ++*recursion_count;
    if (*recursion_count >= FUZZY_MATCH_RECURSION_LIMIT) {
        return 0;
    }


    first_match = TRUE;
    size_t pat_counter = 0;
    size_t str_counter = 0;

    while (str_counter < strlen(line) && pat_counter < strlen(search_word)) {
        int c1;
        int c2;

        c1 = tolower((unsigned char)search_word[pat_counter]);
        c2 = tolower((unsigned char)line[str_counter]);

        // Found match
        if (c1 == c2) {
            int recursiveMatches[MAX_FUZZY_MATCHES];
            size_t recursiveScore = 0;

            // Supplied matches buffer was too short
            if (next_match >= max_matches) {
                return 0;
            }

            // "Copy-on-Write" srcMatches into matches
            if (first_match && src_matches) {
                memcpy(matches, src_matches, next_match * sizeof(src_matches[0]));
                first_match = FALSE;
            }

            const char *next_char = line + 1;

// clang-format off
            if (fuzzy_match_recursive(
                        search_word,
                        next_char,
                        str_idx + 1,
                        &recursiveScore,
                        whole_word,
                        strLen,
                        matches,
                        recursiveMatches,
                        ARRAY_LENGTH(recursiveMatches),
                        next_match,
                        recursion_count
                        )
            ) {
// clang-format on
                if (!recursiveMatch || recursiveScore > bestRecursiveScore) {
                    memcpy(bestRecursiveMatches, recursiveMatches, MAX_FUZZY_MATCHES * sizeof(recursiveMatches[0]));
                    bestRecursiveScore = recursiveScore;
                }
                recursiveMatch = TRUE;
            }

            // Advance
            matches[next_match] = str_idx;
            ++next_match;
            ++pat_counter;
        }
        ++str_idx;
        ++str_counter;
    }

    // Determine if full fuzpat was matched
    matched = search_word[pat_counter] == '\0' ? TRUE : FALSE;

    // Calculate score
    if (matched) {
        *out_score = fuzzy_match_compute_score(whole_word, strLen, matches, next_match);
    }

    // Return best result
    if (recursiveMatch && (!matched || bestRecursiveScore > *out_score)) {
        // Recursive score is better than "this"
        memcpy(matches, bestRecursiveMatches, max_matches * sizeof(matches[0]));
        *out_score = bestRecursiveScore;
        return next_match;
    } else if (matched) {
        return next_match; // "this" score is better than recursive
    }

    return 0; // no match
}

static void fuzzy_match(search_query_t *query) {
    int recursionCount = 0;
    int score = 0;
    size_t numMatches = 0;
    size_t matchCount = 0;

    char search_word_copy[PATH_MAX];
    memset(search_word_copy, '\0', PATH_MAX);
    strcpy(search_word_copy, query->search_word);
    char *search_word_ptr = search_word_copy;

    int counter = 0;
    int complete = FALSE;
    while (TRUE) {
        // Extract one word from the pattern (separated by space)
        if (search_word_ptr[counter] == ' ') {
            ++counter;
            continue;
        }
        if (search_word_ptr[counter] == '\0') {
            break;
        }

        // clang-format off
        matchCount = fuzzy_match_recursive(
                &search_word_ptr[counter],
                query->line,
                0,
                &query->result.score,
                query->search_word,
                query->line_len,
                NULL,
                query->result.matches + numMatches,
                query->max_matches - numMatches,
                0,
                &recursionCount);
        // clang-format on

        if (matchCount == 0) {
            numMatches = 0;
            break;
        }
        query->result.score += score;
        numMatches += matchCount;

        if (complete) {
            break;
        }
        ++counter;
    }
}

static int result_compare(const void *s1, const void *s2) {
    size_t v1 = ((file_info_t *)s1)->fuzzy_score;
    size_t v2 = ((file_info_t *)s2)->fuzzy_score;

    return v1 == v2 ? 0 : v1 > v2 ? -1 : 1;
}

static void sanitize_word(const char *word, char *buf) {
    size_t counter = 0;
    for (size_t i = 0; i < strlen(word); i++) {
        if (word[i] != ' ') {
            buf[counter] = word[i];
            counter++;
        }
    }
}

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

void start_fuzzy_response(const char *search_keyword, const char *cmd, file_info_t *files, size_t len) {
    size_t matched_len = 0;
    static char word[MAX_VIM_INPUT];
    memset(word, 0, MAX_VIM_INPUT);

    size_t current_sz = INITIAL_RES_NUM;
    file_info_t *file_res = malloc(sizeof(file_info_t) * INITIAL_RES_NUM);

    sanitize_word(search_keyword, word);

    for (size_t i = 0; i < len; i++) {
        if (cancel == 1) {
            toggle_cancel(0);
            free(file_res);
            return;
        }
        search_result_t search_result = {.matches = {0}, .score = 0};

        // clang-format off
        search_query_t query = {
            .line = files[i].file_name,
            .line_len = files[i].f_len,
            .search_word = word,
            .max_matches = MAX_FUZZY_MATCHES,
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
            file_res[matched_len].f_len = files[i].f_len;
            ++matched_len;
        }
    }
    qsort(file_res, matched_len, sizeof(file_info_t), result_compare);
    send_res_from_file_info(cmd, file_res, matched_len);

    free(file_res);
}
