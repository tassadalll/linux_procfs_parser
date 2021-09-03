#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"

struct pp_node
{
    struct pp_node *prev;
    struct pp_node *next;
    void*  data;
};

struct pp_list
{
    struct pp_node *head;
    struct pp_node *tail;
    int size;
};

static struct pp_node *create_node(void *data);
static void destroy_node(struct pp_node *node);

struct pp_list *pp_list_create()
{
    struct pp_list* list = malloc(sizeof(struct pp_list));
    if (!list) {
        return NULL;
    }
    memset(list, 0x00, sizeof(struct pp_list));

    return list;
}

void pp_list_destroy(struct pp_list* list)
{
    if (list) {
        free(list);
    }
}

void pp_list_destroy_with_nodes(struct pp_list *list, void (*fn_destroy)(void *data))
{
    struct pp_node* node = NULL;

    node = list->head;
    while(node) {
        struct pp_node* next = node->next;
        
        if(node->data) {
            if(fn_destroy) {
                fn_destroy(node->data);
            } else {
                free(node->data);
            }
        }

        destroy_node(node);
        node = next;
    }
    
    pp_list_destroy(list);
}

static struct pp_node *create_node(void* data)
{
    struct pp_node* node = NULL;
    node = malloc(sizeof(struct pp_node));
    if(node == NULL) {
        return NULL;
    }

    node->prev = NULL;
    node->next = NULL;
    node->data = data;

    return node;
}

static void destroy_node(struct pp_node *node)
{
    free(node);
}

inline int pp_list_size(struct pp_list *list)
{
    return list->size;
}

bool pp_list_get(struct pp_list *list, int index, void **data)
{
    struct pp_node *node = NULL;
    int idx = 0;

    if((unsigned int)index >= list->size) {
        return false;
    }
    
    node = list->head;
    while (idx++ < index) {
        node = node->next;
    }

    *data = node->data;
    
    return true;
}

bool pp_list_pop(struct pp_list *list, int index, void **data)
{
    struct pp_node *node = NULL;
    int idx = 0;

    if ((unsigned int)index >= list->size) {
        return false;
    }

    while (idx++ < index) {
        node = node->next;
    }

    if (node->prev == NULL && node->next == NULL) {
        list->head = NULL;
        list->tail = NULL;
    }

    if (node->prev) {
        node->prev->next = node->next;
    }

    if (node->next) {
        node->next->prev = node->prev;
    }

    *data = node->data;
    destroy_node(node);
    list->size--;

    return true;
}

bool pp_list_lpush(struct pp_list *list, void *data)
{
    struct pp_node* node = NULL;

    node = create_node(data);
    if (node == NULL) {
        return false;
    }

    if (list->size) {
        node->next = list->head;
        list->head->prev = node;
        list->head = node;
    } else {
        list->head = node;
        list->tail = node;
    }

    list->size++;

    return true;
}

bool pp_list_lpop(struct pp_list *list, void **data)
{
    struct pp_node *node = NULL;

    if (list->size <= 0) {
        return false;
    }

    node = list->head;
    if (node->next) {
        list->head = node->next;
        list->head->prev = NULL;
    } else {
        list->head = NULL;
        list->tail = NULL;
    }

    *data = node->data;
    destroy_node(node);
    list->size--;

    return true;
}

bool pp_list_rpush(struct pp_list *list, void *data)
{
    struct pp_node* node = NULL;

    node = create_node(data);
    if (node == NULL) {
        return false;
    }

    if (list->size) {
        node->prev = list->tail;
        list->tail->next = node;
        list->tail = node;
    } else {
        list->head = node;
        list->tail = node;
    }

    list->size++;

    return true;
}

bool pp_list_rpop(struct pp_list *list, void **data)
{
    struct pp_node* node = NULL;

    if (list->size <= 0) {
        return false;
    }

    node = list->tail;
    if (node = NULL) {
        return false;
    }

    if (node->prev) {
        list->tail = node->prev;
        list->tail->next = NULL;
    } else {
        list->head = NULL;
        list->tail = NULL;
    }

    *data = node->data;
    destroy_node(node);
    list->size--;

    return true;
}
