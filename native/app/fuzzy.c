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
static int fuzzy_match_compute_score(const char *str, int strSz, const int *matches, int numMatches) {
    int score;
    int penalty;
    int unmatched;
    int i;
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
    for (i = 0; i < numMatches; ++i) {
        unsigned int currIdx = matches[i];

        if (i > 0) {
            unsigned int prevIdx = matches[i - 1];

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
            int neighbor = ' ';
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
    int score;
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

static int fuzzy_match_recursive(const char *fuzpat, const char *str, int strIdx, int *outScore, const char *strBegin, int strLen, int *srcMatches, int *matches, int maxMatches, int nextMatch, int *recursionCount) {
    int recursiveMatch = FALSE;
    int bestRecursiveMatches[MAX_FUZZY_MATCHES];
    int bestRecursiveScore = 0;
    int first_match;
    int matched;

    ++*recursionCount;
    if (*recursionCount >= FUZZY_MATCH_RECURSION_LIMIT) {
        return 0;
    }

    first_match = TRUE;
    size_t pat_counter = 0;
    size_t str_counter = 0;

    while (str_counter < strlen(str) && pat_counter < strlen(fuzpat)) {
        int c1;
        int c2;

        c1 = tolower(fuzpat[pat_counter]);
        c2 = tolower(str[str_counter]);

        // Found match
        if (c1 == c2) {
            int recursiveMatches[MAX_FUZZY_MATCHES];
            int recursiveScore = 0;

            // Supplied matches buffer was too short
            if (nextMatch >= maxMatches) {
                return 0;
            }

            // "Copy-on-Write" srcMatches into matches
            if (first_match && srcMatches) {
                memcpy(matches, srcMatches, nextMatch * sizeof(srcMatches[0]));
                first_match = FALSE;
            }

            const char *next_char = str + 1;
            if (fuzzy_match_recursive(fuzpat, next_char, strIdx + 1, &recursiveScore, strBegin, strLen, matches, recursiveMatches, ARRAY_LENGTH(recursiveMatches), nextMatch, recursionCount)) {
                if (!recursiveMatch || recursiveScore > bestRecursiveScore) {
                    memcpy(bestRecursiveMatches, recursiveMatches, MAX_FUZZY_MATCHES * sizeof(recursiveMatches[0]));
                    bestRecursiveScore = recursiveScore;
                }
                recursiveMatch = TRUE;
            }

            // Advance
            matches[nextMatch] = strIdx;
            ++nextMatch;
            ++pat_counter;
        }
        ++strIdx;
        ++str_counter;
    }

    // Determine if full fuzpat was matched
    matched = fuzpat[pat_counter] == '\0' ? TRUE : FALSE;

    // Calculate score
    if (matched) {
        *outScore = fuzzy_match_compute_score(strBegin, strLen, matches, nextMatch);
    }

    // Return best result
    if (recursiveMatch && (!matched || bestRecursiveScore > *outScore)) {
        // Recursive score is better than "this"
        memcpy(matches, bestRecursiveMatches, maxMatches * sizeof(matches[0]));
        *outScore = bestRecursiveScore;
        return nextMatch;
    } else if (matched) {
        return nextMatch; // "this" score is better than recursive
    }

    return 0; // no match
}

static void fuzzy_match(search_query_t *query) {
    int recursionCount = 0;
    int score = 0;
    int numMatches = 0;
    int matchCount = 0;

    char pat[PATH_MAX];
    memset(pat, '\0', PATH_MAX);
    strcpy(pat, query->search_word);
    char *p = pat;

    int counter = 0;
    int complete = FALSE;
    while (TRUE) {
        // Extract one word from the pattern (separated by space)
        if (p[counter] == ' ') {
            ++counter;
            continue;
        }
        if (p[counter] == '\0') {
            complete = TRUE;
        }

        matchCount = fuzzy_match_recursive(&p[counter], query->line, 0, &query->result.score, query->search_word, query->line_len, NULL, query->result.matches + numMatches, query->max_matches - numMatches, 0, &recursionCount);
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
    int v1 = ((file_info_t *)s1)->fuzzy_score;
    int v2 = ((file_info_t *)s2)->fuzzy_score;

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

size_t start_fuzzy_response(const char *search_keyword, const char *cmd, file_info_t *files, size_t len) {
    size_t matched_len = 0;
    static char word[MAX_VIM_INPUT];
    static file_info_t file_res[MAX_RESPONSE_LINES + 1];
    memset(file_res, 0, sizeof(file_info_t) * MAX_RESPONSE_LINES);
    memset(word, 0, MAX_VIM_INPUT);
    int last_score = 0;

    sanitize_word(search_keyword, word);

    for (size_t i = 0; i < len; i++) {
        if (cancel == 1) {
            toggle_cancel(0);
            return 0;
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
            if (matched_len < MAX_RESPONSE_LINES) {
                file_res[matched_len].file_path = files[i].file_path;
                file_res[matched_len].fuzzy_score = query.result.score;
                file_res[matched_len].f_len = files[i].f_len;
                if (matched_len == MAX_RESPONSE_LINES - 1) {
                    qsort(file_res, matched_len, sizeof(file_info_t), result_compare);
                    last_score = file_res[matched_len].fuzzy_score;
                }
                ++matched_len;
            } else {
                if (last_score < query.result.score) {
                    last_score = query.result.score;
                    file_res[MAX_RESPONSE_LINES].file_path = files[i].file_path;
                    file_res[MAX_RESPONSE_LINES].fuzzy_score = query.result.score;
                    file_res[MAX_RESPONSE_LINES].f_len = files[i].f_len;
                }
            }
        }
    }
    qsort(file_res, matched_len, sizeof(file_info_t), result_compare);
    send_res_from_file_info(cmd, file_res, matched_len);

    return matched_len;
}
