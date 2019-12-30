/*
 * os_list.c
 *
 *  Created on: 2019Äê6ÔÂ17ÈÕ
 *      Author: jace
 */

#include <stdio.h>
#include <stdint.h>
#include "os_list.h"

#if !USE_STATIC_INLINE_LIST
/**
 * @brief initialize a list
 */

void os_list_init(os_list_t *l)
{
    l->next = l->prev = l;
}

/**
 * @brief insert a node after a list
 *
 * @param l list to insert it
 * @param n new node to be inserted
 */
void os_list_insert_after(os_list_t *l, os_list_t *n)
{
    l->next->prev = n;
    n->next = l->next;

    l->next = n;
    n->prev = l;
}

/**
 * @brief insert a node before a list
 *
 * @param n new node to be inserted
 * @param l list to insert it
 */
void os_list_insert_before(os_list_t *l, os_list_t *n)
{
    l->prev->next = n;
    n->prev = l->prev;

    l->prev = n;
    n->next = l;
}

/**
 * @brief remove node from list.
 * @param n the node to remove from the list.
 */
void os_list_remove(os_list_t *n)
{
    n->next->prev = n->prev;
    n->prev->next = n->next;

    n->next = n->prev = n;
}

/**
 * @brief tests whether a list is empty
 * @param l the list to test.
 */
int os_list_isempty(const os_list_t *l)
{
    return l->next == l;
}
#endif

