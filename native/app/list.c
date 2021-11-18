#include "list.h"
#include "stdio.h"

list_t *list_create() {
    list_t *list = malloc(sizeof(list_t));
    list->entry = NULL;
    list->next = NULL;
    return list;
}

void list_append(list_t *dst_list, void *entry) {
    if (dst_list->entry == NULL) {
        dst_list->entry = entry;
        return;
    }
    list_t *local_list = dst_list;
    while (local_list && local_list->next != NULL) {
        local_list = local_list->next;
    };
    local_list->next = malloc(sizeof(list_t));
    local_list->next->entry = entry;
    local_list->next->next = NULL;
}

void list_del_entry(list_t *dst_list, void *entry) {
    if (dst_list->entry == entry) {
        if (dst_list->next == NULL) {
            dst_list->entry = NULL;
        } else {
            void *next_entry = dst_list->next->entry;
            list_t *next_list = dst_list->next->next;
            free(dst_list->next);
            dst_list->entry = next_entry;
            dst_list->next = next_list;
        }
        return;
    }

    list_t *local_list = dst_list;
    list_t *prev_list = dst_list;
    while (local_list != NULL) {
        if (entry == local_list->entry) {
            prev_list->next = local_list->next;
            free(local_list);
            break;
        }
        prev_list = local_list;
        local_list = local_list->next;
    };
}
