#include "str_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

str_pool_t **init_str_pool(size_t sz) {
    str_pool_t *pool = malloc(sizeof(str_pool_t));
    str_pool_t **p_pool = malloc(sizeof(str_pool_t));
    p_pool[0] = pool;
    pool->strings = malloc(sz);
    pool->idx = 0;
    pool->pos = 0;
    pool->sz = sz;
    pool->initial_sz = sz;
    return p_pool;
}

char *pool_str_with_len(str_pool_t ***pool_addr, const char *s, size_t len) {
    str_pool_t **pool = *pool_addr;
    size_t s_len = len + 1;
    if (len == 0) {
        s_len = strlen(s) + 1;
    }
    size_t idx = pool[0]->idx;
    size_t new_idx = idx;
    size_t sz = pool[0]->sz;
    size_t pos = pool[0]->pos;
    if (s_len >= sz - pos) {
        new_idx++;
        pool = realloc(pool, sizeof(str_pool_t) * (new_idx + 1));
        *pool_addr = pool;
        pool[new_idx] = malloc(sizeof(str_pool_t));
        size_t initial_sz = sz + s_len + 1;
        pool[new_idx]->strings = malloc(initial_sz);
        pool[0]->pos = 0;
        pool[0]->sz = initial_sz;
        pool[0]->idx = new_idx;
        sz = initial_sz;
        pos = 0;
    }
    strcpy(&pool[new_idx]->strings[pos], s);
    size_t new_pos = pos + s_len;
    pool[new_idx]->strings[new_pos] = '\0';
    pool[0]->pos = new_pos + 1;
    return &pool[new_idx]->strings[pos];
}

void deinit_str_pool(str_pool_t **pool) {
    size_t p_sz = pool[0]->idx;
    for (size_t i = 0; i <= p_sz; i++) {
        free(pool[i]->strings);
        free(pool[i]);
    }
    free(pool);
    pool = NULL;
}
