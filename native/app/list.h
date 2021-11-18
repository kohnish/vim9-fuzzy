#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct list_t {
    void *entry;
    struct list_t *next;
} list_t;

list_t *list_create();
void list_append(list_t *dst_list, void *entry);
void list_del_entry(list_t *dst_list, void *entry);

#ifdef __cplusplus
}
#endif

#endif // LIST_H
