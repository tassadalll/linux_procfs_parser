#ifndef __PP_LIST__
#define __PP_LIST__

#include <stdbool.h>

typedef void* pp_list_t;

pp_list_t pp_list_create();

void pp_list_destroy(pp_list_t list_h);

void pp_list_destroy_with_nodes(pp_list_t list_h, void (*destroy)(void *data));

int pp_list_size(pp_list_t list_h);

bool pp_list_get(pp_list_t list_h, int index, void **data);

bool pp_list_pop(pp_list_t list_h, int index, void **data);

bool pp_list_lpush(pp_list_t list_h, void *data);

bool pp_list_lpop(pp_list_t list_h, void **data);

bool pp_list_rpush(pp_list_t list_h, void *data);

bool pp_list_rpop(pp_list_t list_h, void **data);

#endif /* __PP_LIST__ */