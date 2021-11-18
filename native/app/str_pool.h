#ifndef STR_POOL_H
#define STR_POOL_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct str_pool_t {
    char *strings;
    size_t idx;
    size_t pos;
    size_t sz;
    size_t initial_sz;
} str_pool_t;

str_pool_t **init_str_pool(size_t sz);
void deinit_str_pool(str_pool_t **pool);
char *str(str_pool_t ***pool_addr, const char *s);

#ifdef __cplusplus
}
#endif

#endif // STR_POOL_H
