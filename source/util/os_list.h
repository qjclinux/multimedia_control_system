#ifndef _OS_LIST_H_
#define _OS_LIST_H_

struct os_list_node
{
    struct os_list_node *next;                          /**< point to next node. */
    struct os_list_node *prev;                          /**< point to prev node. */
};
typedef struct os_list_node os_list_t;

#define LIST_INIT(node) {&node,&node}

#define USE_STATIC_INLINE_LIST (1)

#if USE_STATIC_INLINE_LIST
/**
 * @brief initialize a list 
 */

static inline void os_list_init(os_list_t *l)
{
    l->next = l->prev = l;
}

/**
 * @brief insert a node after a list
 *
 * @param l list to insert it
 * @param n new node to be inserted
 */
static inline void os_list_insert_after(os_list_t *l, os_list_t *n)
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
static inline void os_list_insert_before(os_list_t *l, os_list_t *n)
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
static inline void os_list_remove(os_list_t *n)
{
    n->next->prev = n->prev;
    n->prev->next = n->next;

    n->next = n->prev = n;
}

/**
 * @brief tests whether a list is empty
 * @param l the list to test.
 */
static inline int os_list_isempty(const os_list_t *l)
{
    return l->next == l;
}
#else

void os_list_init(os_list_t *l);
void os_list_insert_after(os_list_t *l, os_list_t *n);
void os_list_insert_before(os_list_t *l, os_list_t *n);
void os_list_remove(os_list_t *n);
int os_list_isempty(const os_list_t *l);

#endif


/**
 * @brief get the struct for this entry
 * @param node the entry point(the node's structure must the same as member)
 * @param type the type of structure
 * @param member the name of list in structure
 */
#define os_list_entry(node, type, member) \
    ((type *)((char *)(node) - (unsigned long)(&((type *)0)->member)))

#endif
