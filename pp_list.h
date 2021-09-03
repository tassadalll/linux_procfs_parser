#ifndef __PP_LIST__
#define __PP_LIST__

#include <stdbool.h>

typedef struct pp_list* pp_list;

pp_list pp_list_create();

void pp_list_destroy(pp_list list);

void pp_list_destroy_with_nodes(pp_list list, void (*destroy)(void *data));

int pp_list_size(struct pp_list *list);

bool pp_list_get(pp_list list, int index, void **data);

bool pp_list_pop(pp_list list, int index, void **data);

bool pp_list_lpush(pp_list list, void *data);

bool pp_list_lpop(pp_list list, void **data);

bool pp_list_rpush(pp_list list, void *data);

bool pp_list_rpop(pp_list list, void **data);

#endif /* __PP_LIST__ */