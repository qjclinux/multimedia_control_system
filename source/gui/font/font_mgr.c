/*
 * font_mgr.c
 *
 *  Created on: 2019年8月12日
 *      Author: jace
 */

#include "console.h"
#include "crc16.h"
#include "string.h"
#include "osal.h"
#include "font_mgr.h"

#define DEBUG_FONT_MGR 1

#if DEBUG_FONT_MGR
#define FONT_MGR_LOG(fmt,args...)    do{\
                                /*os_printk("%s,%s(),%d:" ,__FILE__, __FUNCTION__,__LINE__);*/\
                                os_printk("%s(),%d:" , __FUNCTION__,__LINE__);\
                                os_printk(fmt,##args);\
                            }while(0)
#define FONT_MGR_INFO(fmt,args...)   do{\
                                os_printk(fmt,##args);\
                            }while(0)
#else
#define FONT_MGR_LOG(fmt,args...)
#define FONT_MGR_INFO(fmt,args...)
#endif

//----------------------------------------------------------------------------------------------
// 字库列表管理
//----------------------------------------------------------------------------------------------

//#if 0
#ifndef OS_BAREMETAL

    static OS_MUTEX m_font_lock=NULL;

    static inline void font_lock()
    {
        if(m_font_lock==NULL)
        {
            OS_MUTEX_CREATE(m_font_lock);
        }
        OS_MUTEX_GET(m_font_lock, OS_MUTEX_FOREVER);
    }

    static inline void font_unlock()
    {
        OS_MUTEX_PUT(m_font_lock);
    }
#else
    #define font_lock()
    #define font_unlock()

#endif

static struct os_list_node m_os_font_list=LIST_INIT(m_os_font_list);


//添加应用到列表
//注：font_info 可以是自动变量
int os_font_add(font_info_t *font_info)
{
    if (!font_info)
        return -1;

    font_info_t *p_node;
    font_lock();

    p_node=(font_info_t *)OS_MALLOC(sizeof(font_info_t));
    if(!p_node)
    {
        font_unlock();
        return -1;
    }

    *p_node=*font_info;
    os_list_insert_after(&m_os_font_list, &p_node->list);

    font_unlock();
    return 0;

}

//获取字库安装信息
//获取OK返回0，其他获取失败
int os_font_get(const char *name,font_info_t *node)
{
    font_info_t *os_font_node;
    struct os_list_node *list_node;
    font_lock();

    for (list_node = m_os_font_list.next;
        list_node != &(m_os_font_list);
        list_node = list_node->next)
    {
        os_font_node = os_list_entry(list_node, font_info_t, list);
        if(strncmp(os_font_node->font_attr.name,name,FONT_NAME_LENGHT)==0)
        {
            if(node)
            {
                *node=*os_font_node;
            }
            font_unlock();
            return 0;
        }
    }

    font_unlock();
    return -1;
}

//获取字库安装信息
font_info_t* os_font_get_suitable_size(const char *name,uint8_t size)
{
#define __diff(x,y) ((x)>(y)?((x)-(y)):((y)-(x)))

    font_info_t *os_font_node,*found_font_node=0;
    struct os_list_node *list_node;

    font_lock();

    for (list_node = m_os_font_list.next;
        list_node != &(m_os_font_list);
        list_node = list_node->next)
    {
        os_font_node = os_list_entry(list_node, font_info_t, list);
        if(strncmp(os_font_node->font_attr.name,name,FONT_NAME_LENGHT)==0)
        {
            if(found_font_node==0)
                found_font_node=os_font_node;
            else
            {
                //找到和目标字号相近/相等的字库
                if(__diff(found_font_node->font_attr.size,size)
                    >__diff(os_font_node->font_attr.size,size))
                {
                    found_font_node=os_font_node;
                }
                //相等，取小号字体
                else if(__diff(found_font_node->font_attr.size,size)
                    ==__diff(os_font_node->font_attr.size,size) &&
                    found_font_node->font_attr.size>os_font_node->font_attr.size)
                {
                    found_font_node=os_font_node;
                }
            }
        }
    }

    font_unlock();
    return found_font_node;
}

//检查字库是否存在
//0 不存在，1 存在
int  os_font_exists(const char *name)
{
    if(os_font_get(name,NULL))
    {
        return 0;
    }
    return 1;
}

//删除字库
int  os_font_delete(const char *name)
{
    font_info_t *os_font_node;
    struct os_list_node *list_node;
    font_lock();

    for (list_node = m_os_font_list.next;
        list_node != &(m_os_font_list);
        list_node = list_node->next)
    {
        os_font_node = os_list_entry(list_node, font_info_t, list);
        if(strncmp(os_font_node->font_attr.name,name,FONT_NAME_LENGHT)==0)
        {
            os_list_remove(&os_font_node->list);
            OS_FREE(os_font_node);

            font_unlock();
            return 0;
        }
    }

    font_unlock();
    return -1;
}

//获取字库安装信息节点
static font_info_t *  os_font_get_one(uint16_t app_id)
{
    font_info_t *os_font_node;
    struct os_list_node *list_node;
    font_lock();

    for (list_node = m_os_font_list.next;
        list_node != &(m_os_font_list);
        list_node = list_node->next)
    {
        os_font_node = os_list_entry(list_node, font_info_t, list);
        if(os_font_node->app_id==app_id)
        {
            font_unlock();
            return os_font_node;
        }
    }

    font_unlock();
    return 0;
}

//删除同一APP安装包的所有字库
int  os_font_delete_all(uint16_t app_id)
{
    font_info_t *os_font_node;
    font_lock();

    while(1)
    {
        os_font_node=os_font_get_one(app_id);
        if(os_font_node)
        {
            os_list_remove(&os_font_node->list);
            OS_FREE(os_font_node);
        }
        else
        {
            break;
        }
    }

    font_unlock();
    return -1;
}

//调试
void os_font_print_all(void)
{
#if USE_OS_DEBUG
    font_info_t *os_font_node;
    struct os_list_node *list_node;
    int i;

    font_lock();

    for (list_node = m_os_font_list.next,i=0;
        list_node != &(m_os_font_list);
        list_node = list_node->next)
    {
        os_font_node = os_list_entry(list_node, font_info_t, list);

        OS_INFO(" font[%d] f_id=%d,app_id=%d,offset=%d,name=%s,size=%d,w=%d,h=%d \r\n",
                i++,
                os_font_node->file_id,os_font_node->app_id,os_font_node->offset,
                os_font_node->font_attr.name,os_font_node->font_attr.size,
                os_font_node->font_attr.width,os_font_node->font_attr.height);
    }

    font_unlock();
#endif
}


//调试
void os_font_self_test(void)
{
#if 0
    FONT_MGR_LOG("font_info_t=%d\r\n",sizeof(font_info_t));
    FONT_MGR_LOG("inst_pkg_res_font_t=%d\r\n",sizeof(inst_pkg_res_font_t));


    font_info_t node={
        .app_id=1,
        .file_id=2,
    };

    os_font_print_all();

    node.app_id=1;
    strcpy(node.font_attr.name,"font1");
    os_font_add(&node);
    os_font_print_all();

    node.app_id=1;
    strcpy(node.font_attr.name,"font2");
    os_font_add(&node);
    os_font_print_all();

    node.app_id=1;
    strcpy(node.font_attr.name,"font3");
    os_font_add(&node);
    os_font_print_all();

    /*
    os_font_delete("font3");
    os_font_print_all();

    os_font_delete("font1");
    os_font_print_all();

    os_font_delete("font2");
    os_font_print_all();
    */

    os_font_delete_all(1);
    os_font_print_all();

#endif
}


