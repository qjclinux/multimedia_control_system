/*
 * app_mgr.c
 *
 *  Created on: 2019年6月17日
 *      Author: jace
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include "app_mgr.h"
#include "console.h"
#include "console.h"
#include "os_list.h"
#include "osal.h"
#include "app_lock.h"
#include "gui_window.h"

//----------------------------------------------------------------------------------------------
// 应用列表
//----------------------------------------------------------------------------------------------
static struct os_list_node m_app_list[]={
    LIST_INIT(m_app_list[0]),/* 表盘 */
    LIST_INIT(m_app_list[1]),/* 应用 */
    LIST_INIT(m_app_list[2]),/* 其他类型， 如下拉、上拉、运动、应用列表 等*/
    LIST_INIT(m_app_list[3]),/*二级以上菜单*/
};

typedef enum{
    APP_NODE_IDX_DIAL,
    APP_NODE_IDX_TOOL,
    APP_NODE_IDX_MENU,
    APP_NODE_IDX_SUB,
    APP_NODE_IDX_MAX,
}app_node_idx_t;


//添加应用到列表
//注意：node 为非自动变量
int os_app_add(os_app_node_t *node)
{
    if (!node)
        return -1;
    app_lock();

//#define add_method os_list_insert_after
#define add_method os_list_insert_before

    if(IS_DIAL_TYPE(node->info->type))
    {
        add_method(&m_app_list[APP_NODE_IDX_DIAL], &node->list);
    }
    else if(IS_TOOL_TYPE(node->info->type))
    {
        add_method(&m_app_list[APP_NODE_IDX_TOOL], &node->list);
    }
    else if(IS_MISC_TYPE(node->info->type))
    {
        add_method(&m_app_list[APP_NODE_IDX_MENU], &node->list);
    }
    else if(IS_SUB_TYPE(node->info->type))
    {
        add_method(&m_app_list[APP_NODE_IDX_SUB], &node->list);
    }
    app_unlock();
    return 0;
}

//申请空间，复制inst_info，之后添加应用到列表
int os_app_copy_and_add(const app_inst_info_t *inst_info)
{
    if (!inst_info)
        return -1;

    os_app_node_t *p_app_node;
    app_lock();
    p_app_node=(os_app_node_t *)OS_MALLOC(sizeof(os_app_node_t));
    if(!p_app_node)
    {
        app_unlock();
        return -1;
    }
    app_inst_info_t *p_inst_info;
    p_inst_info=(app_inst_info_t *)OS_MALLOC(sizeof(app_inst_info_t));
    if(!p_inst_info)
    {
        OS_FREE(p_app_node);
        app_unlock();
        return -1;
    }
    *p_inst_info=*inst_info;
    p_app_node->info=p_inst_info;
    os_app_add(p_app_node);
    app_unlock();
    return 0;

}

//从列表移除应用
void os_app_remove(os_app_node_t *node)
{
    if(!node)
        return;
    app_lock();
    os_list_remove(&node->list);
    app_unlock();
    //Not need to free memory here, it should be free outside?
}

//从列表移除应用,并释放节点空间
void os_app_free(os_app_node_t *node)
{
    if(!node)
        return;
    app_lock();
    os_list_remove(&node->list);

    //如果是系统应用，不能释放内存
//    if(IS_SYSRAM_ADDRESS(node->info))
//        OS_FREE((void*)node->info);
//    if(IS_SYSRAM_ADDRESS(node))
//        OS_FREE(node);
    app_unlock();
}

//获取应用信息
os_app_node_t *os_app_get_node(uint16_t app_id)
{
    os_app_node_t *os_app_node;
    struct os_list_node *list_node;
    app_lock();
    for(int i=0;i<APP_NODE_IDX_MAX;i++)
    {
        for (list_node = m_app_list[i].next;
            list_node != &(m_app_list[i]);
            list_node = list_node->next)
        {
            os_app_node = os_list_entry(list_node, os_app_node_t, list);
            if (os_app_node->info->app_id == app_id)
            {
                app_unlock();
                return os_app_node;
            }
        }
    }
    app_unlock();
    return NULL;
}

//获取APP的下一个节点
os_app_node_t *os_app_get_node_next(uint16_t app_id)
{
    os_app_node_t *os_app_node=NULL;
    struct os_list_node *list_node;
    uint8_t node_list_idx;

    app_lock();
    os_app_node=os_app_get_node(app_id);
    if(os_app_node)
    {
        if(IS_DIAL_TYPE(os_app_node->info->type))
        {
            node_list_idx=APP_NODE_IDX_DIAL;
        }
        else if(IS_TOOL_TYPE(os_app_node->info->type))
        {
            node_list_idx=APP_NODE_IDX_TOOL;
        }
        else if(IS_MISC_TYPE(os_app_node->info->type))
        {
            node_list_idx=APP_NODE_IDX_MENU;
        }
        else //if(IS_SUB_TYPE(os_app_node->info->type))
        {
            node_list_idx=APP_NODE_IDX_SUB;
        }

        list_node=(struct os_list_node *)(os_app_node->list.next);
        if(list_node==&(m_app_list[node_list_idx]))
        {
            list_node=list_node->next;
        }
        os_app_node = os_list_entry(list_node, os_app_node_t, list);
        app_unlock();
        return os_app_node;
    }
    app_unlock();
    return NULL;
}

//获取APP的前一个节点
os_app_node_t *os_app_get_node_before(uint16_t app_id)
{
    os_app_node_t *os_app_node=NULL;
    struct os_list_node *list_node;
    uint8_t node_list_idx;

    app_lock();
    os_app_node=os_app_get_node(app_id);
    if(os_app_node)
    {
        if(IS_DIAL_TYPE(os_app_node->info->type))
        {
            node_list_idx=APP_NODE_IDX_DIAL;
        }
        else if(IS_TOOL_TYPE(os_app_node->info->type))
        {
            node_list_idx=APP_NODE_IDX_TOOL;
        }
        else if(IS_MISC_TYPE(os_app_node->info->type))
        {
            node_list_idx=APP_NODE_IDX_MENU;
        }
        else //if(IS_SUB_TYPE(os_app_node->info->type))
        {
            node_list_idx=APP_NODE_IDX_SUB;
        }

        list_node=(struct os_list_node *)(os_app_node->list.prev);
        if(list_node==&(m_app_list[node_list_idx]))
        {
            list_node=list_node->prev;
        }
        os_app_node = os_list_entry(list_node, os_app_node_t, list);
        app_unlock();
        return os_app_node;
    }
    app_unlock();
    return NULL;
}

//for debug use
void os_app_print_all(void)
{
#if USE_OS_DEBUG
    os_app_node_t *os_app_node;
    struct os_list_node *list_node;

    app_lock();
    for(int i=0;i<APP_NODE_IDX_MAX;i++)
    {
        for (list_node = m_app_list[i].next;
            list_node != &(m_app_list[i]);
            list_node = list_node->next)
        {
            os_app_node = os_list_entry(list_node, os_app_node_t, list);
            OS_LOG("list[%d] app_id=%x,main=%08x,mem=%08x\r\n",
                i,os_app_node->info->app_id,
                (int )os_app_node->info->main,
                (int )os_app_node->info->elf_inram_addr
                );
        }
    }
    app_unlock();
#endif

}
//----------------------------------------------------------------------------------------------
// 应用属性
//----------------------------------------------------------------------------------------------
//应用类型
uint16_t os_app_get_type(uint16_t app_id)
{
    os_app_node_t *os_app_node=NULL;

    app_lock();
    os_app_node=os_app_get_node(app_id);
    if(os_app_node)
    {
        app_unlock();
        return os_app_node->info->type;
    }
    app_unlock();
    return APP_TYPE_UNKNOW;
}
//应用数
uint16_t os_app_get_num(uint16_t type)
{
    uint16_t app_num;
    uint8_t node_list_idx;
    struct os_list_node *list_node;

    app_lock();
    app_num=0;
//    node_list_idx=IS_DIAL_TYPE(type)?APP_NODE_IDX_DIAL:APP_NODE_IDX_TOOL;
    if(IS_DIAL_TYPE(type))
    {
        node_list_idx=APP_NODE_IDX_DIAL;
    }
    else if(IS_TOOL_TYPE(type))
    {
        node_list_idx=APP_NODE_IDX_TOOL;
    }
    else if(IS_MISC_TYPE(type))
    {
        node_list_idx=APP_NODE_IDX_MENU;
    }
    else //if(IS_SUB_TYPE(type))
    {
        node_list_idx=APP_NODE_IDX_SUB;
    }

    for (list_node = m_app_list[node_list_idx].next;
        list_node != &(m_app_list[node_list_idx]);
        list_node = list_node->next)
    {
        app_num++;
    }
    app_unlock();
    return app_num;
}

//列表中的第一个应用
uint16_t os_app_get_first_id(uint16_t type)
{
    uint16_t app_id;
    uint8_t node_list_idx;
    struct os_list_node *list_node;

    app_lock();
//    node_list_idx=IS_DIAL_TYPE(type)?APP_NODE_IDX_DIAL:APP_NODE_IDX_TOOL;
    if(IS_DIAL_TYPE(type))
    {
        node_list_idx=APP_NODE_IDX_DIAL;
    }
    else if(IS_TOOL_TYPE(type))
    {
        node_list_idx=APP_NODE_IDX_TOOL;
    }
    else if(IS_MISC_TYPE(type))
    {
        node_list_idx=APP_NODE_IDX_MENU;
    }
    else //if(IS_SUB_TYPE(type))
    {
        node_list_idx=APP_NODE_IDX_SUB;
    }

    list_node = m_app_list[node_list_idx].next;
    app_id=os_list_entry(list_node, os_app_node_t, list)->info->app_id;

    app_unlock();
    return app_id;
}
//----------------------------------------------------------------------------------------------
// currunt runing app id
//----------------------------------------------------------------------------------------------
static uint16_t m_runing_app_id = APP_ID_NONE;

//当前运行的APP id
void os_app_set_runing(uint16_t new_app_id)
{
    app_lock();
    m_runing_app_id = new_app_id;
    app_unlock();
}

uint16_t os_app_get_runing(void)
{
    uint16_t app_id;
    app_lock();
    app_id = m_runing_app_id;
    app_unlock();
    return app_id;
}

//根据APP ID获取其下一个
uint16_t os_app_get_id_next(uint16_t app_id)
{
    os_app_node_t *os_app_node;
    app_lock();
    os_app_node=os_app_get_node_next(app_id);
    if(os_app_node==NULL)
    {
        app_unlock();
        return APP_ID_NONE;
    }
    app_unlock();
    return os_app_node->info->app_id;
}

//根据APP ID获取其前一个
uint16_t os_app_get_id_before(uint16_t app_id)
{
    os_app_node_t *os_app_node;
    app_lock();
    os_app_node=os_app_get_node_before(app_id);
    if(os_app_node==NULL)
    {
        app_unlock();
        return APP_ID_NONE;
    }
    app_unlock();
    return os_app_node->info->app_id;
}

//----------------------------------------------------------------------------------------------
// APP 运行/退出
//----------------------------------------------------------------------------------------------

#define USE_MEASURE_START_EXIT_TIME (0)

//退出APP
int app_exit(uint16_t app_id,bool kill_window)
{
#if USE_MEASURE_START_EXIT_TIME
    uint32_t t1=0,t2=0;
    t1=OS_GET_TICK_COUNT();
#endif

//    os_timer_delete_all(app_id);
//    os_com_unsubscribe(app_id);
    os_state_unsubscribe(app_id);

    //退出GUI窗口 --由于切换效果要使用到上一个应用的窗口，所以窗口释放要分场景
    if(kill_window)
    {
        gui_window_destroy_current();
    }

    os_app_node_t *os_app_node;

    app_lock();
    os_app_node=os_app_get_node(app_id);
    if(os_app_node==NULL)
    {
        app_unlock();
        return -1;
    }

    //动态安装的要清空内存
    //释放内存后，不需要更新flash应用列表中的信息，因为下一次启动时需要使用到该地址
    if (IS_USER_APP(os_app_node->info->type))
    {
//        if(IS_SYSRAM_ADDRESS(os_app_node->info->elf_inram_addr))
//        {
//            OS_FREE((void*)os_app_node->info->elf_inram_addr);
//        }
    }

    os_app_set_runing(APP_ID_NONE);
    app_unlock();

#if USE_MEASURE_START_EXIT_TIME
    t2=OS_GET_TICK_COUNT();
    OS_LOG("exit t=%d (%d ms) \r\n",t2-t1,OS_TICKS_2_MS(t2-t1));
#endif

    return 0;
}

static int app_start_k(uint16_t app_id,bool need_rel,int argc, char *argv[])
{
    os_app_node_t *os_app_node;

    /*
     * TODO：
     * 经测量，重位花费大量时间(300ms+)，所以表盘切换时不要一直进行重定位，否则会卡。表盘没有切换就让这个表盘一直保持，不退出
     *
     */
#if USE_MEASURE_START_EXIT_TIME
    uint32_t t1=0,t2=0;
    t1=OS_GET_TICK_COUNT();
#endif

    app_lock();
    os_app_node=os_app_get_node(app_id);

    if(os_app_node==NULL)
    {
        app_unlock();
        return -1;
    }

//    if(is_cached_addr((uint32_t)os_app_node->info->main)==false)
//    {
//        OS_LOG("app=%d mian addr=%x err!\r\n",os_app_node->info->main);
//        app_unlock();
//        return -1;
//    }

    if (need_rel && IS_USER_APP(os_app_node->info->type))
    {
        //重定位.data .bss后再启动
//        uint32_t mem_addr=
//            os_app_node->info->elf_inram_addr==INVALID_ELF_INRAM_ADDR?0:os_app_node->info->elf_inram_addr;

//        if(app_relocate_rw_zi(os_app_node->info->elf_inrom_addr,
//            mem_addr,(Elf32_Addr *)&mem_addr))
//        {
//            OS_LOG("relocate rw_zi err!\r\n");
//            app_unlock();
//            return -1;
//        }

        //更新列表中的RAM信息
//        if(os_app_node->info->elf_inram_addr!=mem_addr)
//        {
////            OS_LOG("relocate rw_zi update!\r\n");
//            *(uint32_t*)(&os_app_node->info->elf_inram_addr)=mem_addr; //把常量变为可读写变量，然后赋值(动态安装的信息是内存，可修改；如果是系统的则是常量，不可修改)
//            app_info_set(os_app_node->info);
//        }
    }

    os_app_set_runing(os_app_node->info->app_id);
    app_unlock();

#if USE_MEASURE_START_EXIT_TIME
    t2=OS_GET_TICK_COUNT();
    OS_LOG("start t=%d (%d ms) \r\n",t2-t1,OS_TICKS_2_MS(t2-t1));
#endif

    return os_app_node->info->main(argc,argv);
}

//启动app
int app_start(uint16_t app_id)
{
    return app_start_k(app_id,true,0,0);
}

//带参数启动app
int app_start_with_argument(uint16_t app_id,int argc, char *argv[])
{
    return app_start_k(app_id,true,argc,argv);
}

//启动app（不进行rw,zi重定位，该函数可在调用完app_setup之后使用）
int app_start_without_rel(uint16_t app_id)
{
    return app_start_k(app_id,false,0,0);
}

//检查APP是否存在
//返回：1存在 ，0不存在
int app_exists(uint16_t app_id)
{
    os_app_node_t *os_app_node;
    app_lock();
    os_app_node=os_app_get_node(app_id);
    if(os_app_node==NULL)
    {
        app_unlock();
        return 0;
    }
    app_unlock();
    return 1;
}
