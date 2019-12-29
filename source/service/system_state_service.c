/*
 * sys_sta_service.c
 *
 *  Created on: 2019Äê6ÔÂ17ÈÕ
 *      Author: Administrator
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <time.h>
#include "osal.h"
#include "console.h"
#include "system_state_service.h"
#include "app_mgr.h"

static struct os_list_node m_sys_sta_list=LIST_INIT(m_sys_sta_list);

static os_state_node_t *sys_sta_find_node(uint32_t app_id)
{
    os_state_node_t *found_node;
    struct os_list_node *list_node;

    for (list_node = m_sys_sta_list.next;
        list_node != &(m_sys_sta_list);
        list_node = list_node->next)
    {
        found_node = os_list_entry(list_node, os_state_node_t, list);
        if (found_node->app_id == app_id)
            return found_node;
    }

    return NULL;
}

int os_state_subscribe(uint32_t event,
    void (*callback)(uint32_t event,const void *pdata,uint16_t size) )
{
    uint32_t app_id;
    os_state_node_t *node;
    app_id = os_app_get_runing();

    // only one call back subscribe for an app
    node=sys_sta_find_node(app_id);
    if(!node)
    {
        node = (os_state_node_t*)OS_MALLOC(sizeof(os_state_node_t));
        if (!node)
        {
            return -1;
        }
    }

    node->app_id=app_id;
    node->event=event;
    node->callback=callback;

    os_list_insert_after(&m_sys_sta_list, &node->list);

    return 0;
}

void os_state_unsubscribe(uint32_t app_id)
{
    os_state_node_t *node;
    node=sys_sta_find_node(app_id);
    if(node)
    {
        os_list_remove(&node->list);
        OS_FREE(node);
    }
}

//os_state_node_t *os_state_node_get(uint32_t app_id)
//{
//    return sys_sta_find_node(app_id);
//}

void os_state_dispatch(uint32_t app_id,uint32_t event,const void *pdata,uint16_t size)
{
    os_state_node_t *handle_node;
    handle_node=sys_sta_find_node(app_id);
    if(!handle_node)
    {
        return;
    }

    if(event & handle_node->event)
    {
        handle_node->callback(event & handle_node->event,pdata,size);
    }
}






