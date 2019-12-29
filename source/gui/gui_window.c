/*
 * gui_window.c
 *
 *  Created on: 2019年7月11日
 *      Author: jace
 */
#include <string.h>
#include <stdbool.h>
#include "osal.h"
#include "gui.h"
#include "console.h"
#include "gui_window.h"
#include "gui_res.h"
#include "gui_lock.h"
#include "font_data.h"
#include "app_mgr.h"
#include "jacefs.h"
#include "math.h"
#include "gui_window_cfg.h"

#define DEBUG_GUI_WID 0
#if DEBUG_GUI_WID
    #define GUI_WID_LOG(fmt,args...) do{\
            /*printf("%s,%s(),%d:" ,__FILE__, __FUNCTION__,__LINE__);*/\
            os_printk("%s(),%d:" , __FUNCTION__,__LINE__);\
            os_printk(fmt,##args);\
        }while(0)
    #define GUI_WID_INFO(fmt,args...)        do{\
            os_printk(fmt,##args);\
        }while(0)
#else
    #define GUI_WID_LOG(fmt,args...)
    #define GUI_WID_INFO(fmt,args...)
#endif

static gui_window_hdl_t *m_gui_windows_stack;
static uint16_t m_gui_window_cnt=0;

#ifndef M_PI
    #define M_PI        3.14159265358979323846
#endif

//窗口入栈
//返回：0 成功，-1 失败
static int gui_window_push(gui_window_hdl_t hdl)
{
    gui_window_hdl_t *gui_windows;

    gui_lock();
    gui_windows=(gui_window_hdl_t *)OS_MALLOC(sizeof(gui_window_hdl_t)*(m_gui_window_cnt+1));

    if(!gui_windows)
    {
        gui_unlock();
        return -1;
    }

    if(m_gui_window_cnt>0)
    {
        memcpy(gui_windows,m_gui_windows_stack,sizeof(gui_window_hdl_t)*(m_gui_window_cnt));
        OS_FREE(m_gui_windows_stack);
    }
    gui_windows[m_gui_window_cnt]=hdl;
    m_gui_window_cnt++;
    m_gui_windows_stack=gui_windows;
    gui_unlock();

    return 0;
}

//窗口出栈
//返回：hdl 成功，0 失败
static gui_window_hdl_t gui_window_pop()
{
    gui_window_hdl_t hdl;
    gui_window_hdl_t *gui_windows;

    gui_lock();

    if(m_gui_window_cnt==0)
    {
        gui_unlock();
        return 0;
    }

    if(m_gui_window_cnt==1)
    {
        hdl=m_gui_windows_stack[0];
        OS_FREE(m_gui_windows_stack);
        m_gui_window_cnt=0;
        gui_unlock();
        return hdl;
    }

    gui_windows=(gui_window_hdl_t *)OS_MALLOC(sizeof(gui_window_hdl_t)*(m_gui_window_cnt-1));
    if(!gui_windows)
    {
        gui_unlock();
        return 0;
    }

    m_gui_window_cnt--;
    memcpy(gui_windows,m_gui_windows_stack,sizeof(gui_window_hdl_t)*(m_gui_window_cnt));
    hdl=m_gui_windows_stack[m_gui_window_cnt];
    OS_FREE(m_gui_windows_stack);
    m_gui_windows_stack=gui_windows;

    gui_unlock();

    return hdl;
}

//取出栈顶第二位窗口
//返回：hdl 成功，0 失败
static gui_window_hdl_t gui_window_pop2()
{
    gui_window_hdl_t hdl;
    gui_window_hdl_t *gui_windows;

    gui_lock();

    if(m_gui_window_cnt<2)
    {
        gui_unlock();
        return 0;
    }

    gui_windows=(gui_window_hdl_t *)OS_MALLOC(sizeof(gui_window_hdl_t)*(m_gui_window_cnt-1));
    if(!gui_windows)
    {
        gui_unlock();
        return 0;
    }

    m_gui_window_cnt--;
    memcpy(gui_windows,m_gui_windows_stack,sizeof(gui_window_hdl_t)*(m_gui_window_cnt));
    gui_windows[m_gui_window_cnt-1]=m_gui_windows_stack[m_gui_window_cnt];


    hdl=m_gui_windows_stack[m_gui_window_cnt-1];
    OS_FREE(m_gui_windows_stack);
    m_gui_windows_stack=gui_windows;

    gui_unlock();

    return hdl;
}

//获取栈顶窗口
//返回：hdl 成功，0 失败
static gui_window_hdl_t gui_window_get_top()
{
    gui_window_hdl_t hdl;

    gui_lock();
    if(m_gui_window_cnt==0)
    {
        gui_unlock();
        return 0;
    }

    hdl=m_gui_windows_stack[m_gui_window_cnt-1];
    gui_unlock();

    return hdl;
}

//获取栈顶窗口
//返回：hdl 成功，0 失败
static gui_window_hdl_t gui_window_get_top2()
{
    gui_window_hdl_t hdl;

    gui_lock();
    if(m_gui_window_cnt<2)
    {
        gui_unlock();
        return 0;
    }

    hdl=m_gui_windows_stack[m_gui_window_cnt-2];
    gui_unlock();

    return hdl;
}

//调试打印所有窗口
void gui_window_print_all(void)
{
#if USE_OS_DEBUG
    gui_lock();
    for(int i=0;i<m_gui_window_cnt;i++)
    {
        gui_window_t *gwin=m_gui_windows_stack[i];
        OS_LOG("%08x gwin[%d]={x=%d,y=%d,w=%d,h=%d,bg_bmp=%x,layer_cnt=%d,event_cnt=%d,}\r\n",
            (uint32_t)gwin,
            i,
            gwin->x,
            gwin->y,
            gwin->w,
            gwin->h,
            (uint32_t)gwin->bg_bmp,
            gwin->layer_cnt,
            gwin->event_cnt
        );
    }
    gui_unlock();
#endif
}


/**
@brief : 创建窗口

@param :
-x   窗口内x显示偏移
-y   窗口内y显示偏移
-w   窗口宽
-h   窗口高

@retval:
- hdl   成功
- 0     失败
*/
gui_window_hdl_t gui_window_create(uint16_t x,uint16_t y,uint16_t w,uint16_t h)
{
    gui_window_t *gwin;

    gwin=(gui_window_t *)OS_MALLOC(sizeof(gui_window_t));

    if(!gwin || w==0 ||h==0)
        return 0;

    gwin->app_id=os_app_get_runing();

    gwin->x=x;
    gwin->y=y;
    gwin->w=w;
    gwin->h=h;

    //窗口是 GUI_RES_X 整数倍
//    gwin->w=(gwin->w+GUI_RES_X-1)/GUI_RES_X*GUI_RES_X;
//    gwin->h=(gwin->h+GUI_RES_Y-1)/GUI_RES_Y*GUI_RES_Y;

    //窗口不能比LCD小
    if(gwin->w<GUI_RES_X)
        gwin->w=GUI_RES_X;
    if(gwin->h<GUI_RES_Y)
        gwin->h=GUI_RES_Y;

    //显示内容都在窗口内
//    if(gwin->x + GUI_RES_X > gwin->w)
//        gwin->x=gwin->w-GUI_RES_X;
//    if(gwin->y + GUI_RES_Y > gwin->h)
//        gwin->y=gwin->h-GUI_RES_Y;

    if(gwin->x >= gwin->w)
        gwin->x=gwin->w-1;
    if(gwin->y >= gwin->h)
        gwin->y=gwin->h-1;

    gwin->bg_color=INVALID_COLOR;/* 默认不填充背景颜色，以便加快窗口刷新时间 */
    gwin->bg_bmp=NULL;
    gwin->scroll=GUI_DISABLE_SCROLL;
    gwin->effect=GUI_EFFECT_NONE;
    gwin->layer_cnt=0;
    gwin->event_cnt=0;
    gwin->layers=NULL;
    gwin->events=NULL;

    gwin->loaded=false;

    //放到GUI窗口栈
    if(gui_window_push((gui_window_hdl_t*)gwin))
    {
        OS_FREE(gwin);
        return 0;
    }

    GUI_WID_LOG("create success,addr=%08x  ,window num=%d\r\n",(uint32_t)gwin,m_gui_window_cnt);

    //调试，观察窗口内存是否释放
    if(m_gui_window_cnt>2)
    {
        OS_LOG("create win addr=%08x ,window num=%d, last window not free !!!\r\n",(uint32_t)gwin,m_gui_window_cnt);
    }


    return (gui_window_hdl_t*)gwin;
}

/**
@brief : 创建窗口：大小根据屏幕自动赋值宽高

@param :
-x   窗口内x显示偏移
-y   窗口内y显示偏移

@retval:
- hdl   成功
- 0     失败
*/
gui_window_hdl_t gui_window_create_maximum(uint16_t x,uint16_t y)
{
    return gui_window_create(x,y,GUI_RES_X,GUI_RES_Y);
}

/**
@brief : 设置窗口显示起始位置（偏移）

@param :
-hdl 窗口句柄
-x   窗口内x显示偏移
-y   窗口内y显示偏移

@retval:
- 0     成功
- -1    失败
*/
int gui_window_set_position(gui_window_hdl_t hdl,uint16_t x,uint16_t y)
{
    if(!hdl)
        return -1;
    gui_window_t *gwin=(gui_window_t *)hdl;

    gui_lock();
    gwin->x = x;
    gwin->y = y;

    //窗口不能比LCD小
//    if(gwin->w<GUI_RES_X)
//        gwin->w=GUI_RES_X;
//    if(gwin->h<GUI_RES_Y)
//        gwin->h=GUI_RES_Y;

    //显示内容都在窗口内
//    if(gwin->x + GUI_RES_X > gwin->w)
//        gwin->x=gwin->w-GUI_RES_X;
//    if(gwin->y + GUI_RES_Y > gwin->h)
//        gwin->y=gwin->h-GUI_RES_Y;

    if(gwin->x >= gwin->w)
        gwin->x=gwin->w-1;
    if(gwin->y >= gwin->h)
        gwin->y=gwin->h-1;

//    GUI_WID_LOG("x=%d,y=%d,gwin->x=%d,gwin->y=%d \r\n",x,y,gwin->x,gwin->y);

    gui_unlock();

    return 0;
}

//获取显示的y偏移
uint16_t gui_window_get_y(gui_window_hdl_t hdl)
{
    gui_window_t *gwin;
    uint16_t y;
    gui_lock();

    if(!hdl )
    {
        gwin=(gui_window_t *)gui_window_get_top();
        if(!gwin)
        {
            gui_unlock();
            return 0;
        }
    }
    else
    {
        gwin=(gui_window_t *)hdl;
    }
    y=gwin->y;
    gui_unlock();

    return y;
}

/**
@brief : 设置窗口大小

@param :
-hdl 窗口句柄
-w   窗口宽
-h   窗口高

@retval:
- 0 成功
- -1 失败
*/
int gui_window_set_size(gui_window_hdl_t hdl,uint16_t w,uint16_t h)
{
    if(!hdl || w==0 || h==0)
        return -1;
    gui_window_t *gwin=(gui_window_t *)hdl;

    gui_lock();
    gwin->w = w;
    gwin->h = h;

    //窗口是 GUI_RES_X 整数倍
//    gwin->w=(gwin->w+GUI_RES_X-1)/GUI_RES_X*GUI_RES_X;
//    gwin->h=(gwin->h+GUI_RES_Y-1)/GUI_RES_Y*GUI_RES_Y;

    //窗口不能比LCD小
    if(gwin->w<GUI_RES_X)
        gwin->w=GUI_RES_X;
    if(gwin->h<GUI_RES_Y)
        gwin->h=GUI_RES_Y;

    //显示内容都在窗口内
//    if(gwin->x + GUI_RES_X > gwin->w)
//        gwin->x=gwin->w-GUI_RES_X;
//    if(gwin->y + GUI_RES_Y > gwin->h)
//        gwin->y=gwin->h-GUI_RES_Y;

    if(gwin->x >= gwin->w)
        gwin->x=gwin->w-1;
    if(gwin->y >= gwin->h)
        gwin->y=gwin->h-1;

    gui_unlock();
    return 0;
}

/**
@brief : 设置滚动属性

@param :
-hdl      窗口句柄
-scroll   窗口滚动方向，ref@gui_window_scroll_t

@retval:
- 0 成功
- -1 失败
*/
int gui_window_set_scroll(gui_window_hdl_t hdl,gui_window_scroll_t scroll )
{
    if(!hdl)
        return -1;
    gui_window_t *gwin=(gui_window_t *)hdl;

    gui_lock();
    gwin->scroll = scroll;
    gui_unlock();
    return 0;
}

/**
@brief : 设置背景图片（系统内部使用）

@param :
-hdl      窗口句柄
-res_id   图片资源的ID
-app_id   图片资源所属APP

@retval:
- 0 成功
- -1 失败
*/
int gui_window_set_bg_bmp_inner(gui_window_hdl_t hdl,uint16_t  res_id,uint16_t app_id)
{
    if(!hdl)
        return -1;
    gui_window_t *gwin=(gui_window_t *)hdl;

    gui_lock();

    gui_bmp_layer_attr_t *bg_bmp;

    bg_bmp=gui_layer_get_bmp_attr(res_id,app_id,APP_INST_PKG_FILE_ID);
    if(!bg_bmp)
    {
        gui_unlock();
        return -1;
    }

    if(gwin->bg_bmp)
    {
        OS_FREE(gwin->bg_bmp);
    }
    gwin->bg_bmp=bg_bmp;

    if(gwin->bg_color==INVALID_COLOR)
    {
        gwin->bg_color=BLACK;
    }

    gui_unlock();
    return 0;

}

/**
@brief : 设置背景图片
                        注意：该背景图像只能是当前窗口所属的APP的资源（暂时不可设置为其他APP的资源）

@param :
-hdl      窗口句柄
-res_id   图片资源的ID

@retval:
- 0 成功
- -1 失败
*/
int gui_window_set_bg_bmp(gui_window_hdl_t hdl,uint16_t  res_id)
{
    uint16_t app_id;
    app_id=os_app_get_runing();

    return gui_window_set_bg_bmp_inner(hdl,res_id,app_id);
}

/**
@brief : 设置背景颜色

@param :
-hdl      窗口句柄
-bg_color 背景颜色值，只支持RGB565颜色

@retval:
- 0 成功
- -1 失败
*/
int gui_window_set_bg_color(gui_window_hdl_t hdl,uint16_t  bg_color)
{
    if(!hdl)
        return -1;
    gui_window_t *gwin=(gui_window_t *)hdl;

    gui_lock();
    if(INVALID_COLOR==bg_color)
        bg_color=INVALID_COLOR-1;
    gwin->bg_color = bg_color;
    gui_unlock();

    return 0;
}

/**
@brief : 设置动画效果（暂未使用--动画未实现）

@param :
-hdl      窗口句柄
-effect   窗口加载时的效果

@retval:
- 0 成功
- -1 失败
*/
int gui_window_set_effect(gui_window_hdl_t hdl,gui_window_effect_t  effect)
{
    if(!hdl)
        return -1;
    gui_window_t *gwin=(gui_window_t *)hdl;

    gui_lock();
    gwin->effect = effect;
    gui_unlock();
    return 0;
}

/**
@brief : 设置窗口加载完成

@param :
-hdl    窗口句柄，  注意：如果hdl==NULL，那么将设置栈顶的窗口

@retval:
- 0 成功
- -1 失败
*/
int gui_window_set_loaded(gui_window_hdl_t hdl)
{
    gui_window_t *gwin;

    gui_lock();

    if(!hdl )
    {
        gwin=(gui_window_t *)gui_window_get_top();
        if(!gwin)
        {
            gui_unlock();
            return -1;
        }
    }
    else
    {
        gwin=(gui_window_t *)hdl;
    }

    gwin->loaded = true;
    gui_unlock();
    return 0;
}

/**
@brief : 获取窗口是否加载完成

@param :
-hdl    窗口句柄，  注意：如果hdl==NULL，那么将获取栈顶的窗口

@retval:
- 0 加载未完成
- -1 加载完成
*/
int gui_window_get_loaded(gui_window_hdl_t hdl)
{
    gui_window_t *gwin;
    bool loaded=false;

    gui_lock();
    if(!hdl )
    {
        gwin=(gui_window_t *)gui_window_get_top();
        if(!gwin)
        {
            gui_unlock();
            return 0;
        }
    }
    else
    {
        gwin=(gui_window_t *)hdl;
    }

    loaded=gwin->loaded;
    gui_unlock();

    return loaded;
}

/**
@brief : 删除窗口，释放内存

@param :
-hdl    窗口句柄

@retval:
- 0 成功
- -1 失败
*/
static int __gui_window_destroy(gui_window_hdl_t hdl)
{
    if(!hdl)
        return -1;
    gui_window_t *gwin=(gui_window_t *)hdl;
    int i;

//    gui_lock();

    //窗口事件
    if(gwin->event_cnt>0)
    {
        for(i=0;i<gwin->event_cnt;i++)
        {
            OS_FREE(gwin->events[i]);
        }
        OS_FREE(gwin->events);
    }

    //窗口图层:图层里面有事件，有图层属性
    if(gwin->layer_cnt>0)
    {
        for(i=0;i<gwin->layer_cnt;i++)
        {
            gui_layer_destroy(gwin->layers[i]);
        }
        OS_FREE(gwin->layers);
    }

    //窗口背景
    if(gwin->bg_bmp)
    {
        OS_FREE(gwin->bg_bmp);
    }

    OS_FREE(gwin);

//    gui_unlock();

    GUI_WID_LOG("delete success,addr=%08x  ,window num=%d\r\n",(uint32_t)gwin,m_gui_window_cnt);
    return 0;
}

/**
@brief : 删除栈顶窗口，释放内存

@param :
-

@retval:
- 0 成功
- -1 失败
*/
int gui_window_destroy_current()
{
    gui_window_hdl_t hdl=gui_window_pop();
    return __gui_window_destroy(hdl);
}

/**
@brief : 删除栈顶第2个窗口，释放内存

@param :
-

@retval:
- 0 成功
- -1 失败
*/
int gui_window_destroy_last()
{
    //删除队栈顶的所有窗口
    gui_window_hdl_t hdl;

    do{
        hdl=gui_window_pop2();
        if(hdl)
        {
            __gui_window_destroy(hdl);
            continue;
        }
    }while(0);

    return 0;
}

/**
@brief : 添加图层

@param :
-hdl    窗口句柄
-layer  图层句柄

@retval:
- 0 成功
- -1 失败
*/
int gui_window_add_layer(gui_window_hdl_t hdl,gui_layer_hdl_t layer)
{
    if(!hdl || !layer)
        return -1;

    gui_window_t *gwin=(gui_window_t *)hdl;
    gui_layer_hdl_t *new_layer;

    gui_lock();
    new_layer = OS_MALLOC(sizeof(gui_layer_hdl_t) * (gwin->layer_cnt + 1));
    if (NULL == new_layer)
    {
        OS_FREE(layer);//TODO:此处可能不应该释放空间
        gui_unlock();
        return -1;
    }

    if (gwin->layer_cnt>0)
    {
        memcpy(new_layer, gwin->layers, sizeof(gui_layer_hdl_t)*(gwin->layer_cnt));
        OS_FREE(gwin->layers);
    }

    //顺序添加
    new_layer[gwin->layer_cnt] = layer;

    gwin->layer_cnt++;
    gwin->layers=new_layer;

    gui_unlock();
    return 0;
}

/**
@brief : 添加窗口事件

@param :
-hdl    窗口句柄
-event  窗口事件数据

@retval:
- 0 成功
- -1 失败
*/
static int __gui_window_add_event(gui_window_hdl_t hdl,gui_event_hdl_t event)
{
    if(!hdl || !event)
        return -1;

    gui_window_t *gwin=(gui_window_t *)hdl;
    gui_event_hdl_t *new_events;

    gui_lock();
    new_events = OS_MALLOC(sizeof(gui_event_hdl_t) * (gwin->event_cnt + 1));
    if (NULL == new_events)
    {
        gui_unlock();
        return -1;
    }

    if (gwin->event_cnt>0)
    {
        memcpy(new_events, gwin->events, sizeof(gui_event_hdl_t)*(gwin->event_cnt));
        OS_FREE(gwin->events);
    }
    new_events[gwin->event_cnt] = event;

    gwin->event_cnt++;
    gwin->events=new_events;

    gui_unlock();
    return 0;
}

/**
@brief : 订阅窗口事件

@param :
-hdl        窗口句柄
-evt_type   窗口事件类型，ref@gui_event_type_t
-cb         事件回调函数，格式ref@gui_event_call_back_t

@retval:
- 0 成功
- -1 失败
*/
int gui_window_subscribe_event(gui_window_hdl_t hdl,
    gui_event_type_t evt_type,
    gui_event_call_back_t cb)
{
    if(!hdl || evt_type==GUI_EVT_NONE || !cb)
        return -1;

    gui_event_t *new_event;

    gui_lock();
    new_event = OS_MALLOC(sizeof(gui_event_t));
    if (NULL == new_event)
    {
        gui_unlock();
        return -1;
    }

    new_event->cb=cb;
    new_event->type=evt_type;

    if(__gui_window_add_event(hdl,(gui_event_hdl_t)new_event))
    {
        OS_FREE(new_event);
        gui_unlock();
        return -1;
    }

    gui_unlock();
    return 0;
}

/**
@brief : 窗口事件回调处理

@param :
-hdl        窗口句柄
-evt_type   窗口事件类型，ref@gui_event_type_t
-pdata      事件的数据，数据的格式为ref@ gui_evt_tp_data_t
-size       事件的数据大小（字节）

@retval:
- 0 成功
- -1 失败
*/
int gui_window_dispatch_event(gui_window_hdl_t hdl,
    gui_event_type_t evt_type,uint8_t *pdata, uint16_t size)
{
    if(evt_type==GUI_EVT_NONE || !pdata )
        return -1;

    gui_window_t *gwin;
    gui_event_t *handle_event;
    gui_layer_t *layer;
    int i,j;

    gui_lock();

    if(!hdl )
    {
        gwin=(gui_window_t *)gui_window_get_top();
        if(!gwin)
        {
            gui_unlock();
            return -1;
        }
    }
    else
    {
        gwin=(gui_window_t *)hdl;
    }


    //窗口事件
    for(i=0;i<gwin->event_cnt;i++)
    {
        handle_event= (gui_event_t *)gwin->events[i];
        if(handle_event->type==evt_type && handle_event->cb)
        {
            handle_event->cb((void*)gwin,pdata, size);
        }
    }

    gui_evt_tp_data_t *tp_data=(gui_evt_tp_data_t *)pdata;
    uint16_t win_x=gwin->x+tp_data->x;
    uint16_t win_y=gwin->y+tp_data->y;
    uint16_t lyr_x,lyr_y;

    //图层事件
    for(i=0;i<gwin->layer_cnt;i++)
    {
        layer= (gui_layer_t *)gwin->layers[i];

        for(j=0;j<layer->event_cnt;j++)
        {
            handle_event= (gui_event_t *)layer->events[j];

//            GUI_WID_LOG("layer[%d]=(%d,%d)~(%d,%d),xy=(%d,%d)\r\n",
//                j,
//                layer->x,layer->y,
//                (layer->x+layer->w),(layer->y+layer->h),
//                win_x,win_y
//                );

            if(layer->visible==LAYER_INVISIBLE)
                continue;

            //固定图层，坐标为当前LCD坐标，不是窗口偏移后的坐标
            if(layer->fixed==LAYER_ENABLE_FIXED)
            {
                lyr_x=layer->x%GUI_RES_X+gwin->x;
                lyr_y=layer->y%GUI_RES_Y+gwin->y;
            }
            else
            {
                lyr_x=layer->x;
                lyr_y=layer->y;
            }

            //如果当前操作的tp坐标在图层坐标范围内，上报事件
            if(lyr_x<=win_x
                && (lyr_x+layer->w)>win_x
                && lyr_y<=win_y
                && (lyr_y+layer->h)>win_y)
            {
                if(handle_event->type==evt_type && handle_event->cb)
                {
                    handle_event->cb(layer,pdata, size);
                }
            }
        }
    }

    gui_unlock();
    return 0;
}
/*------------------------------------------------------------------------------------------------*/

//一行数据的缓存
static uint16_t one_line_buf[GUI_RES_X];

typedef struct{
    uint16_t disp_w,disp_h;         //窗口读取的实际大小（绝对值）
    uint16_t start_x,start_y;       //窗口的读取开始xy坐标（绝对值，表示的是在窗口内的坐标）
    uint16_t offset_x,offset_y;     //图层在fb内的偏移（相对值，表示的是图层显示开始坐标--如layer_start_x/y 相对start_x/y的差值）
}draw_win_range_t;

typedef struct{
    uint16_t layer_start_x,layer_start_y,   //实际显示的图层范围（绝对值，表示的是在窗口内的坐标）
             layer_end_x,layer_end_y;
    uint16_t layer_w,layer_h;               //图层宽高（绝对值）
}draw_layer_range_t;

typedef struct{
    uint16_t read_w,read_h,             //实际读取的图像的宽高（绝对值）
        read_offset_x,read_offset_y;    //实际读取的图层内的xy偏移（相对值，相对于layer_start_x/y的差值）
}draw_pix_range_t;

static inline int __fill_layer(gui_window_t *gwin,uint16_t *fill_fb,draw_win_range_t *win_rg);

/**
@brief : 加载窗口内容到fb

@param :
-hdl       窗口句柄
-fill_fb   窗口数据填充的fb
-win_off_x  加载的窗口附加偏移，最终窗口偏移为 gwin->x+offset_x             ,注意：0<= win_off_x <GUI_RES_X
-win_load_w 加载的窗口实际宽，最终窗口为 w=load_w>gwin->w?gwin->w:load_w  ,注意：0<= win_load_w <GUI_RES_X
-win_off_y  加载的窗口附加偏移，最终窗口偏移为 gwin->y+offset_y             ,注意：0<= win_off_y <GUI_RES_Y
-win_load_h 加载的窗口实际高，最终窗口为 h=load_h>gwin->h?gwin->h:load_h  ,注意：0<= win_load_h <GUI_RES_Y

@retval:
- 0 成功
- -1 失败
*/

static int __gui_window_fb_fill(gui_window_hdl_t hdl,uint16_t *fill_fb,
    uint16_t win_off_x,uint16_t win_load_w,
    uint16_t win_off_y,uint16_t win_load_h
    )
{
    if(!hdl || !fill_fb)
    {
        return -1;
    }

//    return 0;

    gui_window_t *gwin=(gui_window_t *)hdl;
    draw_win_range_t win_rg;

#if MEASURE_FILL_TIME
    uint32_t t1=0,t2=0,t3=0;

    t1=OS_GET_TICK_COUNT();
#endif

    gui_lock();

    //如果窗口什么都没有，那么直接返回
    if(gwin->layer_cnt==0 && gwin->bg_bmp==NULL)
    {
        gui_unlock();
        return -1;
    }

    win_rg.start_x=win_rg.start_y=0;

    win_rg.disp_w=win_load_w;
    if(win_rg.disp_w>gwin->w)
        win_rg.disp_w=gwin->w;
    if(win_rg.disp_w > GUI_RES_X)
        win_rg.disp_w=GUI_RES_X;

    win_rg.disp_h=win_load_h;
    if(win_rg.disp_h>gwin->h)
        win_rg.disp_h=gwin->h;
    if(win_rg.disp_h > GUI_RES_Y)
        win_rg.disp_h=GUI_RES_Y;

    /*
     * 根据滚动属性，确定显示的窗口范围
     */
    switch(gwin->scroll)
    {
        /*
         * 如果没有使能滚动，那么从0坐标开始显示
         * 如果 win_rg.start_x>= GUI_RES_X，那么不需要加载窗口内容
         * 如果 win_rg.start_x+win_rg.disp_w >= GUI_RES_X，那么窗口加载宽为 win_rg.disp_w=GUI_RES_X-win_rg.start_x
         */
        case GUI_DISABLE_SCROLL:
        {
            win_rg.start_x=win_off_x;
            win_rg.start_y=win_off_y;

            if(win_rg.start_x>=GUI_RES_X)
            {
//                OS_LOG("start_x=%d\r\n",win_rg.start_x);
                gui_unlock();
                return -1;
            }

            if(win_rg.start_x+win_rg.disp_w>GUI_RES_X)
            {
                win_rg.disp_w=GUI_RES_X-win_rg.start_x;
            }

            if(win_rg.start_y>=GUI_RES_Y)
            {
//                OS_LOG("start_y=%d win_off_y=%d win_load_h=%d\r\n",win_rg.start_y,win_off_y,win_load_h);
                gui_unlock();
                return -1;
            }

            if(win_rg.start_y+win_rg.disp_h>GUI_RES_Y)
            {
                win_rg.disp_h=GUI_RES_Y-win_rg.start_y;
            }

            break;
        }

//        case GUI_ENABLE_HOR_SCROLL:
//        case GUI_ENABLE_VER_SCROLL:
        case GUI_ENABLE_SCROLL:
        default:
        {
            win_rg.start_x=gwin->x+win_off_x;
            win_rg.start_y=gwin->y+win_off_y;

            if(win_rg.start_x>=gwin->w)
            {
//                OS_LOG("start_x=%d\r\n",win_rg.start_x);
                gui_unlock();
                return -1;
            }

            if(win_rg.start_x+win_rg.disp_w>gwin->w)
            {
                win_rg.disp_w=gwin->w-win_rg.start_x;
            }

            if(win_rg.start_y>=gwin->h)
            {
//                OS_LOG("start_y=%d\r\n",win_rg.start_y);
                gui_unlock();
                return -1;
            }

            if(win_rg.start_y+win_rg.disp_h>gwin->h)
            {
                win_rg.disp_h=gwin->h-win_rg.start_y;
            }

            break;
        }
    }

    GUI_WID_LOG("win=%08x disp_w=%d,disp_h=%d,start_x=%d,start_y=%d \r\n",
        (uint32_t)gwin,win_rg.disp_w,win_rg.disp_h,win_rg.start_x,win_rg.start_y);


    /*
     * 获取背景
     */
#if 1
    uint16_t i,j;
    if(gwin->bg_bmp)
    {
        /*
         * 背景图片从左上角的(0,0)开始显示
         * 如果其不在显示窗口内，则不需要填充
         *
         * TODO:目前不支持背景滚动，需要完善
         */
        uint16_t bg_bmp_w,bg_bmp_h;
        uint16_t bg_bmp_x,bg_bmp_y;

        bg_bmp_x=win_off_x;
        bg_bmp_y=win_off_y;

        bg_bmp_w=gwin->bg_bmp->w-win_off_x;
        bg_bmp_h=gwin->bg_bmp->h-win_off_y;

        if(bg_bmp_w>win_rg.disp_w)
            bg_bmp_w=win_rg.disp_w;

        if(bg_bmp_h>win_rg.disp_h)
           bg_bmp_h=win_rg.disp_h;

        if(bg_bmp_x>=gwin->bg_bmp->w || bg_bmp_x>=GUI_RES_X)
        {
           goto _fill_bg_color;
        }

        if(bg_bmp_x+bg_bmp_w>GUI_RES_X)
        {
           bg_bmp_w=GUI_RES_X-bg_bmp_x;
        }

        if(bg_bmp_y>=gwin->bg_bmp->h || bg_bmp_y>=GUI_RES_Y)
        {
           goto _fill_bg_color;
        }

        if(bg_bmp_y+bg_bmp_h>GUI_RES_Y)
        {
           bg_bmp_h=GUI_RES_Y-bg_bmp_y;
        }

//           GUI_WID_LOG("win=%08x disp_w=%d,disp_h=%d,start_x=%d,start_y=%d bmp={%d,%d,%d,%d}\r\n",
//                   (uint32_t)gwin,win_rg.disp_w,win_rg.disp_h,win_rg.start_x,win_rg.start_y,
//                   bg_bmp_x,bg_bmp_y,bg_bmp_w,bg_bmp_h);

        /*
        *  实测240*240的背景图片，读取完整需要20ms！
        *
        *TODO:
        *  这里数据一行一行获取，效率低，是导致刷屏慢的原因，可优化！
        *
        */

        for(i=0;i<bg_bmp_h;i++)
        {
           gui_res_get_data(gwin->bg_bmp,
                   (gwin->bg_bmp->w*(i+bg_bmp_y)+bg_bmp_x)*GUI_BYTES_PER_PIX,
                   bg_bmp_w*GUI_BYTES_PER_PIX,
                   (uint8_t*)&fill_fb[i*GUI_RES_X] //填充总是从(0,0)位置开始
               );
        }

        /*
        * 把背景图像未覆盖到的区域用背景颜色填充
        * 主要是填充右边和底边
        */
        if(bg_bmp_w<win_rg.disp_w)
        {
           for(i=0;i<win_rg.disp_h;i++)
           {
               for (j = bg_bmp_w; j < win_rg.disp_w; j++)
               {
                   fill_fb[i * GUI_RES_X + j] = gwin->bg_color;
               }
           }
        }

        for(i=bg_bmp_h;i<win_rg.disp_h;i++)
        {
          for (j = 0; j < win_rg.disp_w; j++)
          {
              fill_fb[i * GUI_RES_X + j] = gwin->bg_color;
          }
        }
    }
    else if(gwin->bg_color!=INVALID_COLOR)
    {
_fill_bg_color:

         /*
          * 实测240*240的窗口，填充颜色需要9ms！  改用DMA传输2字节宽 ,耗时为 1 ms,4字节宽0 ms
          *
          */
#if USE_DMA_SPEED_UP_FILL
        if(win_rg.disp_w>=100)
        {
            if(win_rg.disp_w==GUI_RES_X && win_rg.disp_h==GUI_RES_Y)
            {
                static uint32_t dma_color;
                dma_color=gwin->bg_color<<16|gwin->bg_color;
                dma_start_k((uint32_t)(&dma_color),(uint32_t)fill_fb,
                    GUI_RES_X*GUI_RES_Y*GUI_BYTES_PER_PIX,false,true,4);
            }
            else
            {
                for(i=0;i<win_rg.disp_h;i++)
                {
                    dma_start_k((uint32_t)(&gwin->bg_color),(uint32_t)&fill_fb[i * GUI_RES_X],
                        win_rg.disp_w*GUI_BYTES_PER_PIX,false,true,2);
                }
            }
        }
        else
#endif
        {
            for(i=0;i<win_rg.disp_h;i++)
            {
                for (j = 0; j < win_rg.disp_w; j++)
                {
                    fill_fb[i * GUI_RES_X + j] = gwin->bg_color;
                }
            }
        }
    }
#endif

    /*
     *  加载图层内容
     */

#if MEASURE_FILL_TIME
    t2=OS_GET_TICK_COUNT();
#endif

    __fill_layer(gwin,fill_fb,&win_rg);

#if MEASURE_FILL_TIME
    t3=OS_GET_TICK_COUNT();
#endif


#if MEASURE_FILL_TIME
    OS_LOG("bg=%d (%d ms) lyr=%d (%d ms)\r\n",
                t2-t1,OS_TICKS_2_MS(t2-t1),
                t3-t1,OS_TICKS_2_MS(t3-t1));
#endif

    gui_unlock();

    return 0;
}

//计算图层有效范围，计算图层对应窗口的偏移
static inline int __get_range(
    draw_win_range_t *win_rg,
    draw_layer_range_t *lyr_rg,
    draw_pix_range_t *pix_rg
    )
{
    // 图层开始位置在 显示窗口内（4种情况：整个图层在窗口内，图层左半部分在窗口内，图层上半部分在窗口内，图层左上部分在窗口内）
    if(lyr_rg->layer_start_x >= win_rg->start_x
        && lyr_rg->layer_start_y >= win_rg->start_y
        && lyr_rg->layer_start_x < (win_rg->start_x+win_rg->disp_w)
        && lyr_rg->layer_start_y < (win_rg->start_y+win_rg->disp_h) )
    {
        win_rg->offset_x=(lyr_rg->layer_start_x-win_rg->start_x);
        win_rg->offset_y=(lyr_rg->layer_start_y-win_rg->start_y);

        if((lyr_rg->layer_w+lyr_rg->layer_start_x) > (win_rg->start_x+win_rg->disp_w))
            pix_rg->read_w=win_rg->start_x+win_rg->disp_w-lyr_rg->layer_start_x;
        else
            pix_rg->read_w=lyr_rg->layer_w;

        if((lyr_rg->layer_h+lyr_rg->layer_start_y) > (win_rg->start_y+win_rg->disp_h))
            pix_rg->read_h=win_rg->start_y+win_rg->disp_h-lyr_rg->layer_start_y;
        else
            pix_rg->read_h=lyr_rg->layer_h;
    }
    // 图层结束位置在 显示窗口内
    //（3种情况：图层右下在窗口内，图层右半部分在窗口内，图层下半部分在窗口内）
    else if( lyr_rg->layer_end_x >= win_rg->start_x && lyr_rg->layer_end_y >= win_rg->start_y
            && lyr_rg->layer_end_x < (win_rg->start_x+win_rg->disp_w) && lyr_rg->layer_end_y < (win_rg->start_y+win_rg->disp_h)
        )
    {
        if(lyr_rg->layer_start_x > win_rg->start_x)
        {
            win_rg->offset_x=lyr_rg->layer_start_x - win_rg->start_x;
            pix_rg->read_w=lyr_rg->layer_w;
            //pix_rg->read_offset_x=0;
        }
        else
        {
            win_rg->offset_x=0;
            pix_rg->read_offset_x=win_rg->start_x-lyr_rg->layer_start_x;
            pix_rg->read_w=lyr_rg->layer_w-pix_rg->read_offset_x;
        }

        if(lyr_rg->layer_start_y > win_rg->start_y)
        {
            win_rg->offset_y=lyr_rg->layer_start_y - win_rg->start_y;
            pix_rg->read_h=lyr_rg->layer_h;
            //pix_rg->read_offset_y=0;
        }
        else
        {
            win_rg->offset_y=0;
            pix_rg->read_offset_y=win_rg->start_y-lyr_rg->layer_start_y;
            pix_rg->read_h=lyr_rg->layer_h-pix_rg->read_offset_y;
        }
    }
    // 图层结束位置在 显示窗口外，开始位置也在窗口外
    //（2种情况：图层右上部分在窗口内，图层左下部分在窗口内）
    else if( lyr_rg->layer_start_x >=win_rg->start_x && lyr_rg->layer_start_x < (win_rg->start_x+win_rg->disp_w) &&
        lyr_rg->layer_end_y >=win_rg->start_y && lyr_rg->layer_end_y < (win_rg->start_y+win_rg->disp_h)
        )
    {
        win_rg->offset_x=lyr_rg->layer_start_x - win_rg->start_x;
        //pix_rg->read_offset_x=0;
        pix_rg->read_w=win_rg->start_x+win_rg->disp_w-lyr_rg->layer_start_x;

        win_rg->offset_y= 0;
        pix_rg->read_offset_y=win_rg->start_y-lyr_rg->layer_start_y;
        pix_rg->read_h=lyr_rg->layer_h-pix_rg->read_offset_y;
    }
    else if( lyr_rg->layer_start_y >=win_rg->start_y && lyr_rg->layer_start_y < (win_rg->start_y+win_rg->disp_h) &&
        lyr_rg->layer_end_x >=win_rg->start_x && lyr_rg->layer_end_x < (win_rg->start_x+win_rg->disp_w)
        )
    {
        win_rg->offset_y=lyr_rg->layer_start_y - win_rg->start_y;
        //pix_rg->read_offset_y=0;
        pix_rg->read_h=win_rg->start_y+win_rg->disp_h-lyr_rg->layer_start_y;

        win_rg->offset_x= 0;
        pix_rg->read_offset_x=win_rg->start_x-lyr_rg->layer_start_x;
        pix_rg->read_w=lyr_rg->layer_w-pix_rg->read_offset_x;
    }
    /*
     * 图层w、h大于窗口，跨越窗口的情况：
     * （7种情况：）
     *
     */
    //1 图层x开始位置和结束位置都在窗口内，图层y开始位置和结束位置都在窗口外
    else if( lyr_rg->layer_start_x >=win_rg->start_x /*&& lyr_rg->layer_start_x < (win_rg->start_x+win_rg->disp_w)*/
        /*&& lyr_rg->layer_end_x >=win_rg->start_x */&& lyr_rg->layer_end_x < (win_rg->start_x+win_rg->disp_w)
        && lyr_rg->layer_start_y <win_rg->start_y
        && lyr_rg->layer_end_y >= (win_rg->start_y+win_rg->disp_h)
        )
    {
        win_rg->offset_x=lyr_rg->layer_start_x - win_rg->start_x;
        //pix_rg->read_offset_x=0;
        pix_rg->read_w=lyr_rg->layer_w;

        win_rg->offset_y= 0;
        pix_rg->read_offset_y=win_rg->start_y-lyr_rg->layer_start_y;
        pix_rg->read_h=win_rg->disp_h;
    }
    //2 图层x结束位置在窗口内，图层y开始位置和结束位置都在窗口外
    else if( /*lyr_rg->layer_start_x >=win_rg->start_x && lyr_rg->layer_start_x < (win_rg->start_x+win_rg->disp_w)
        &&*/ lyr_rg->layer_end_x >=win_rg->start_x && lyr_rg->layer_end_x < (win_rg->start_x+win_rg->disp_w)
        && lyr_rg->layer_start_y <win_rg->start_y
        && lyr_rg->layer_end_y >= (win_rg->start_y+win_rg->disp_h)
        )
    {
        win_rg->offset_x=0;
        pix_rg->read_offset_x=win_rg->start_x-lyr_rg->layer_start_x;
        pix_rg->read_w=lyr_rg->layer_w-pix_rg->read_offset_x;

        win_rg->offset_y= 0;
        pix_rg->read_offset_y=win_rg->start_y-lyr_rg->layer_start_y;
        pix_rg->read_h=win_rg->disp_h;
    }
    //3 图层x开始位置在窗口内，图层y开始位置和结束位置都在窗口外
    else if( lyr_rg->layer_start_x >=win_rg->start_x && lyr_rg->layer_start_x < (win_rg->start_x+win_rg->disp_w)
        /*&& lyr_rg->layer_end_x >=win_rg->start_x && lyr_rg->layer_end_x < (win_rg->start_x+win_rg->disp_w)*/
        && lyr_rg->layer_start_y <win_rg->start_y
        && lyr_rg->layer_end_y >= (win_rg->start_y+win_rg->disp_h)
        )
    {
        win_rg->offset_x=lyr_rg->layer_start_x-win_rg->start_x;
        pix_rg->read_offset_x=0;
        pix_rg->read_w=win_rg->disp_w-win_rg->offset_x;

        win_rg->offset_y= 0;
        pix_rg->read_offset_y=win_rg->start_y-lyr_rg->layer_start_y;
        pix_rg->read_h=win_rg->disp_h;
    }
    //4 图层y开始位置和结束位置都在窗口内，图层x开始位置和结束位置都在窗口外
    else if( lyr_rg->layer_start_y >=win_rg->start_y /*&& lyr_rg->layer_start_y < (win_rg->start_y+win_rg->disp_h)*/
        /*&& lyr_rg->layer_end_y >=win_rg->start_y */&& lyr_rg->layer_end_y < (win_rg->start_y+win_rg->disp_h)
        && lyr_rg->layer_start_x <win_rg->start_x
        && lyr_rg->layer_end_x >= (win_rg->start_x+win_rg->disp_w)
        )
    {
        win_rg->offset_x= 0;
        pix_rg->read_offset_x=win_rg->start_x-lyr_rg->layer_start_x;
        pix_rg->read_w=win_rg->disp_w;

        win_rg->offset_y=lyr_rg->layer_start_y - win_rg->start_y;
        pix_rg->read_offset_y=0;
        pix_rg->read_h=lyr_rg->layer_h;
    }
    //5 图层y结束位置在窗口内，图层x开始位置和结束位置都在窗口外
    else if( /*lyr_rg->layer_start_y >=win_rg->start_y && lyr_rg->layer_start_y < (win_rg->start_y+win_rg->disp_h)
        &&*/ lyr_rg->layer_end_y >=win_rg->start_y && lyr_rg->layer_end_y < (win_rg->start_y+win_rg->disp_h)
        && lyr_rg->layer_start_x <win_rg->start_x
        && lyr_rg->layer_end_x >= (win_rg->start_x+win_rg->disp_w)
        )
    {
        win_rg->offset_x= 0;
        pix_rg->read_offset_x=win_rg->start_x-lyr_rg->layer_start_x;
        pix_rg->read_w=win_rg->disp_w;

        win_rg->offset_y=0;
        pix_rg->read_offset_y=win_rg->start_y-lyr_rg->layer_start_y;
        pix_rg->read_h=lyr_rg->layer_h-pix_rg->read_offset_y;
    }
    //6 图层x结束位置在窗口内，图层x开始位置和结束位置都在窗口外
    else if( lyr_rg->layer_start_y >=win_rg->start_y && lyr_rg->layer_start_y < (win_rg->start_y+win_rg->disp_h)
        /*&& lyr_rg->layer_end_y >=win_rg->start_y && lyr_rg->layer_end_y < (win_rg->start_y+win_rg->disp_h)*/
        && lyr_rg->layer_start_x <win_rg->start_x
        && lyr_rg->layer_end_x >= (win_rg->start_x+win_rg->disp_w)
        )
    {
        win_rg->offset_x= 0;
        pix_rg->read_offset_x=win_rg->start_x-lyr_rg->layer_start_x;
        pix_rg->read_w=win_rg->disp_w;

        win_rg->offset_y=lyr_rg->layer_start_y-win_rg->start_y;
        pix_rg->read_offset_y=0;
        pix_rg->read_h=win_rg->disp_h-win_rg->offset_y;
    }
    //7 图层x，y开始位置和结束位置都在窗口外--图层大于显示窗口
    else if( lyr_rg->layer_start_y < win_rg->start_y
        && lyr_rg->layer_end_y >= (win_rg->start_y+win_rg->disp_h)
        && lyr_rg->layer_start_x <win_rg->start_x
        && lyr_rg->layer_end_x >= (win_rg->start_x+win_rg->disp_w)
        )
    {
        win_rg->offset_x= 0;
        pix_rg->read_offset_x=win_rg->start_x-lyr_rg->layer_start_x;
        pix_rg->read_w=win_rg->disp_w;

        win_rg->offset_y=0;
        pix_rg->read_offset_y=win_rg->start_y-lyr_rg->layer_start_y;
        pix_rg->read_h=win_rg->disp_h;
    }
    //TODO:修复其他情况
    else
    {
        pix_rg->read_w=pix_rg->read_h=0;
        win_rg->offset_x=win_rg->offset_y=0;

        return -1;
    }
    return 0;
}

//把layer 的内容加载到fb
static inline int __fill_layer(gui_window_t *gwin,uint16_t *fill_fb,draw_win_range_t *win_rg)
{
    uint16_t i,j,k;

    draw_layer_range_t lyr_rg;
    draw_pix_range_t pix_rg;


    for(i=0;i<gwin->layer_cnt;i++)
    {
        gui_layer_t *layer=gwin->layers[i];

        if(layer->visible==LAYER_INVISIBLE)
            continue;

        if(layer->fixed==LAYER_ENABLE_FIXED)
        {
            lyr_rg.layer_start_x=layer->x%GUI_RES_X+gwin->x;//取离窗口顶部的偏移
            lyr_rg.layer_start_y=layer->y%GUI_RES_Y+gwin->y;
        }
        else
        {
            lyr_rg.layer_start_x=layer->x;
            lyr_rg.layer_start_y=layer->y;
        }

        lyr_rg.layer_w=layer->w;
        lyr_rg.layer_h=layer->h;

        lyr_rg.layer_end_x=lyr_rg.layer_start_x+lyr_rg.layer_w-1;
        lyr_rg.layer_end_y=lyr_rg.layer_start_y+lyr_rg.layer_h-1;


        pix_rg.read_offset_x=0;
        pix_rg.read_offset_y=0;

        switch(layer->type)
        {
            /* 图像图层 */
            case LAYER_TYPE_BITMAP:
            {
                gui_bmp_layer_attr_t *bmp_attr=(gui_bmp_layer_attr_t *)layer->attr;

                //图片比图层大，只显示图层大小的内容；否则以图片大小为准
                if(lyr_rg.layer_w>bmp_attr->w)
                    lyr_rg.layer_w=bmp_attr->w;
                if(lyr_rg.layer_h>bmp_attr->h)
                    lyr_rg.layer_h=bmp_attr->h;

                lyr_rg.layer_end_x=lyr_rg.layer_start_x+lyr_rg.layer_w-1;
                lyr_rg.layer_end_y=lyr_rg.layer_start_y+lyr_rg.layer_h-1;

                //图像不旋转
                if(layer->angle==0
                    //|| gwin->loaded==false //在切换界面时，不旋转图像，加快处理速度
                )
                {
                    if(__get_range(win_rg,&lyr_rg,&pix_rg))
                    {
                        break;
                    }

                    //填充FB
                    switch(bmp_attr->type)
                    {
                        case GUI_PIX_TYPE_RGB565:
                        {
                            if(layer->transparent==LAYER_ENABLE_TRANSP)
                            {
                                for(j=0;j<pix_rg.read_h;j++)
                                {
                                   gui_res_get_data(bmp_attr ,
                                           (bmp_attr->w*(j+pix_rg.read_offset_y)+pix_rg.read_offset_x)*GUI_BYTES_PER_PIX,
                                           pix_rg.read_w*GUI_BYTES_PER_PIX,
                                           (uint8_t*)one_line_buf
                                       );

                                  for(k=0;k<pix_rg.read_w;k++)
                                  {
                                       //透明的颜色不显示
                                       if(one_line_buf[k]!=layer->trans_color)
                                           fill_fb[(j+win_rg->offset_y)*GUI_RES_X+win_rg->offset_x+k]=one_line_buf[k];
                                  }
                                }
                            }
                            else
                            {
                                for(j=0;j<pix_rg.read_h;j++)
                                {
                                   gui_res_get_data(bmp_attr,
                                        (bmp_attr->w*(j+pix_rg.read_offset_y)+pix_rg.read_offset_x)*GUI_BYTES_PER_PIX,
                                        pix_rg.read_w*GUI_BYTES_PER_PIX,
                                        (uint8_t*)&fill_fb[(j+win_rg->offset_y)*GUI_RES_X+win_rg->offset_x]
                                    );
                                }
                            }
                            break;
                        }

                        case GUI_PIX_TYPE_RGB888:
                        case GUI_PIX_TYPE_RGB:
                        case GUI_PIX_TYPE_RGB3:
                        case GUI_PIX_TYPE_RGB1:
                        default:
                        {
                            break;
                        }
                    }


                    GUI_WID_LOG("layer[%d] read_w=%d,read_h=%d,win_rg->offset_x=%d,offset_y=%d\r\n",
                        i,pix_rg.read_w,pix_rg.read_h,win_rg->offset_x,win_rg->offset_y);

                    //TODO:有的图层重叠，不要每个图层内容都读取，只读取最前面的图层显示，被覆盖的图层不要读取以节省时间

                    break;
                }

#if MEASURE_FILL_TIME
                uint32_t t1,t2;
                t1=OS_GET_TICK_COUNT();
#endif

                pix_rg.read_w=lyr_rg.layer_w;
                pix_rg.read_h=lyr_rg.layer_h;

                //TODO：这里限制了图像大小小于GUI_RES_X*GUI_RES_Y的情况，如需要则修改
                if(pix_rg.read_w>GUI_RES_X)
                    pix_rg.read_w=GUI_RES_X;
                if(pix_rg.read_h>GUI_RES_Y)
                    pix_rg.read_h=GUI_RES_Y;

                /*
                 * 图像旋转: 使用三角函数将图像坐标一个个转换
                 * TODO：
                 *      1 由于每个像素点都要计算坐标，比较耗时，需要优化
                 *      2 由于图像旋转会带来变形（出现锯齿），所以图像旋转只建议旋转90度的倍数 或 45度对称
                 */

                int16_t x0,y0;
                int16_t x,y,x1,y1;
                uint16_t *pix_data;
                float rad;
                float cos_val,sin_val;
                int16_t fix_x,fix_y;

                rad=layer->angle*M_PI/180;
                x0=lyr_rg.layer_start_x+layer->rotate_center_x;
                y0=lyr_rg.layer_start_y+layer->rotate_center_y;
                cos_val= cos(rad);
                sin_val= sin(rad);

                pix_data=one_line_buf;

                /*
                                                             旋转图像修正：
                                                                屏幕如果是偶数的宽高，那么旋转中心必须取4个点，旋转之后的图像才能和原来重合。
                                                                如果只取一个点，具体偏移情况为（取旋转中心为4个点中的右下角的点）：
                                                                    第1象限，图像不旋转，图像不偏移
                                                                    第2象限，图像旋转90，图像偏移x+1，所以旋转中心x-1
                                                                    第3象限，图像旋转180，图像偏移 x+1,y+1，旋转中心x-1,y-1
                                                                    第4象限，图像旋转270，图像偏移 y+1，旋转中心y-1
                  */
                if(layer->angle<90)
                {
                    fix_x=0;
                    fix_y=0;
                }
                else if(layer->angle<180)
                {
                    fix_x=-1;
                    fix_y=0;
                }
                else if(layer->angle<270)
                {
                    fix_x=-1;
                    fix_y=-1;
                }
                else //if(layer->angle<360)
                {
                    fix_x=0;
                    fix_y=-1;
                }

                switch(bmp_attr->type)
                {
                    case GUI_PIX_TYPE_RGB565:
                    {
                        for(j=0;j<pix_rg.read_h;j++)
                        {

                            gui_res_get_data(bmp_attr,
                                       (bmp_attr->w*(j))*GUI_BYTES_PER_PIX,
                                       pix_rg.read_w*GUI_BYTES_PER_PIX,
                                       (uint8_t*)one_line_buf
                                   );

                              for(k=0;k<pix_rg.read_w;k++)
                              {
                                  //透明的颜色不显示
                                   if(pix_data[k]!=layer->trans_color)
                                   {
                                       x=lyr_rg.layer_start_x+k;
                                       y=lyr_rg.layer_start_y+j;

                                       /*
                                        * 设围绕旋转的原点为(X0,Y0)，需要旋转的点为(X,Y),旋转后的点为(Xt,Yt)，旋转角度为a
                                        * 旋转公式：
                                        *   Xt= (X - X0)*cos(a) - (Y - Y0)*sin(a) + X0
                                        *   Yt= (X - X0)*sin(a) + (Y - Y0)*cos(a) + Y0
                                        */

                                       x1=(x - x0)*cos_val- (y - y0)*sin_val + x0 +0.5f;
                                       y1=(x - x0)*sin_val + (y - y0)*cos_val + y0 +0.5f;

                                       //图像修正
                                       x1+=fix_x;
                                       y1+=fix_y;

                                       GUI_WID_LOG("xy={%d,%d} ==> xy={%d,%d}\r\n",(int16_t)x,(int16_t)y,(int16_t)x1,(int16_t)y1);

                                       if(x1>=win_rg->start_x && x1<win_rg->start_x+win_rg->disp_w
                                           && y1>=win_rg->start_y && y1<win_rg->start_y+win_rg->disp_h)
                                       {
                                           //实际上，不管窗口如何偏移，数据填充到fb的都是从(0,0)开始，所以这里要减去窗口的起始值，得到从(0,0)的偏移
                                           fill_fb[((int16_t)y1-win_rg->start_y)*GUI_RES_X+(int16_t)x1-win_rg->start_x]
                                                   =pix_data[k];
                                       }
                                       else
                                       {
                                           GUI_WID_LOG("rotation err!\r\n");
                                       }

                                   }
                              }
                        }
                        break;
                    }

                    case GUI_PIX_TYPE_RGB1:
                    default:
                    {
                        break;
                    }
                }


#if MEASURE_FILL_TIME
                t2=OS_GET_TICK_COUNT();

                OS_LOG("bmp rot=%d (%d ms) \r\n",
                    t2-t1,OS_TICKS_2_MS(t2-t1));
#endif

                break;
            }

            /*
             * 像素图层
             *
             * TODO:像素图层增加像素 w h 属性，而不是用layer的 w h 属性
             */
            case LAYER_TYPE_PIXEL:
            {
                //由于旋转时图像加载速度过慢，所以在切换窗口时不要加载
//                if(gwin->loaded==false)
//                {
//                    break;
//                }

                gui_pixel_layer_attr_t *pix_attr=(gui_pixel_layer_attr_t *)layer->attr;

//                GUI_WID_LOG("layer[%d] read_w=%d,pix_rg.read_h=%d,x=%d,y=%d,angle=%d,pix_type=%x,xy={%d,%d}\r\n",
//                    i,layer->w,layer->h,layer->x,layer->y,pix_attr->angle,pix_attr->type,
//                    pix_attr->rotate_center_x,pix_attr->rotate_center_y);

                //以下 目前只支持RGB565 rgb1(黑白)
                if(pix_attr->type!=GUI_PIX_TYPE_RGB565 && pix_attr->type!=GUI_PIX_TYPE_RGB1 )
                    continue;

                //图像不旋转
                if(layer->angle==0
                    //||gwin->loaded==false //在切换界面时，不旋转图像，加快处理速度
                )
                {
                    if(__get_range(win_rg,&lyr_rg,&pix_rg))
                    {
                        break;
                    }

                    //填充FB
                    switch(pix_attr->type)
                    {
                        case GUI_PIX_TYPE_RGB565:
                        {
                            uint16_t *pix_data=(uint16_t *)pix_attr->data;
                            if(layer->transparent==LAYER_ENABLE_TRANSP)
                            {
                                for (j = 0; j < pix_rg.read_h; j++)
                                {
                                    for (k = 0; k < pix_rg.read_w; k++)
                                    {
                                        //透明的颜色不显示
                                        if (pix_data[layer->w * (j + pix_rg.read_offset_y) + pix_rg.read_offset_x + k]
                                            != layer->trans_color)
                                            {
                                                fill_fb[(j + win_rg->offset_y) * GUI_RES_X + win_rg->offset_x + k] =
                                                    pix_data[layer->w * (j + pix_rg.read_offset_y) + pix_rg.read_offset_x + k];
                                            }
                                    }
                                }
                            }
                            else
                            {
                                for (j = 0; j < pix_rg.read_h; j++)
                                {
                                    memcpy(&fill_fb[(j + win_rg->offset_y) * GUI_RES_X + win_rg->offset_x],
                                        &pix_data[layer->w * (j + pix_rg.read_offset_y) + pix_rg.read_offset_x],
                                        pix_rg.read_w * GUI_BYTES_PER_PIX);
                                }
                            }
                        }
                        break;

                        case GUI_PIX_TYPE_RGB1:
                        {
                            uint8_t *pix_data = (uint8_t *)pix_attr->data;
                            uint16_t pix_idx, byte_idx, bit_idx, color;
                            for (j = 0; j < pix_rg.read_h; j++)
                            {
                                for (k = 0; k < pix_rg.read_w; k++)
                                {
                                    pix_idx = layer->w * (j + pix_rg.read_offset_y) + pix_rg.read_offset_x + k;

                                    byte_idx = pix_idx >> 3;
                                    bit_idx = pix_idx & 0x7;
                                    color = pix_data[byte_idx] & (0x1 << bit_idx);

                                    if (color)
                                    {
                                        fill_fb[(j + win_rg->offset_y) * GUI_RES_X + win_rg->offset_x + k] =
                                            layer->front_color;
                                    }
                                    else if(layer->transparent!=LAYER_ENABLE_TRANSP)
                                    {
                                        fill_fb[(j + win_rg->offset_y) * GUI_RES_X + win_rg->offset_x + k] =
                                            layer->back_color;
                                    }
                                }
                            }
                        }
                        break;
                    }

                    break;
                }

                //以下 目前只支持RGB565
                if(pix_attr->type!=GUI_PIX_TYPE_RGB565)
                    continue;

#if MEASURE_FILL_TIME
                uint32_t t1,t2;
                t1=OS_GET_TICK_COUNT();
#endif

                /*
                 * 图像旋转: 使用三角函数将图像坐标一个个转换
                 * TODO：
                 *      1 由于每个像素点都要计算坐标，比较耗时，需要优化
                 *      2 由于图像旋转会带来变形（出现锯齿），所以图像旋转只建议旋转90度的倍数 或 45度对称
                 *
                 * TODO：这里的代码和上面的重复了，考虑合并！
                 */

                int16_t x0,y0;
                int16_t x,y,x1,y1;
                uint16_t *pix_data;
                float rad;
                float cos_val,sin_val;
                int16_t fix_x,fix_y;

                rad=layer->angle*M_PI/180;
                x0=lyr_rg.layer_start_x+layer->rotate_center_x;
                y0=lyr_rg.layer_start_y+layer->rotate_center_y;
                cos_val= cos(rad);
                sin_val= sin(rad);

                pix_data=(uint16_t *)pix_attr->data;

                /*
                                                             旋转图像修正：
                                                                屏幕如果是偶数的宽高，那么旋转中心必须取4个点，旋转之后的图像才能和原来重合。
                                                                如果只取一个点，具体偏移情况为（取旋转中心为4个点中的右下角的点）：
                                                                    第1象限，图像不旋转，图像不偏移
                                                                    第2象限，图像旋转90，图像偏移x+1，所以旋转中心x-1
                                                                    第3象限，图像旋转180，图像偏移 x+1,y+1，旋转中心x-1,y-1
                                                                    第4象限，图像旋转270，图像偏移 y+1，旋转中心y-1
                  */
                if(layer->angle<90)
                {
                    fix_x=0;
                    fix_y=0;
                }
                else if(layer->angle<180)
                {
                    fix_x=-1;
                    fix_y=0;
                }
                else if(layer->angle<270)
                {
                    fix_x=-1;
                    fix_y=-1;
                }
                else //if(layer->angle<360)
                {
                    fix_x=0;
                    fix_y=-1;
                }

                switch(pix_attr->type)
                {
                    case GUI_PIX_TYPE_RGB565:
                    {
                        for(j=0;j<lyr_rg.layer_h;j++)
                        {
                              for(k=0;k<lyr_rg.layer_w;k++)
                              {
                                  //透明的颜色不显示
                                   if(pix_data[layer->w*(j)+k]!=layer->trans_color)
                                   {
                                       x=lyr_rg.layer_start_x+k;
                                       y=lyr_rg.layer_start_y+j;

                                       /*
                                        * 设围绕旋转的原点为(X0,Y0)，需要旋转的点为(X,Y),旋转后的点为(Xt,Yt)，旋转角度为a
                                        * 旋转公式：
                                        *   Xt= (X - X0)*cos(a) - (Y - Y0)*sin(a) + X0
                                        *   Yt= (X - X0)*sin(a) + (Y - Y0)*cos(a) + Y0
                                        */

                                       x1=(x - x0)*cos_val- (y - y0)*sin_val + x0 +0.5f;
                                       y1=(x - x0)*sin_val + (y - y0)*cos_val + y0 +0.5f;

                                       //图像修正
                                       x1+=fix_x;
                                       y1+=fix_y;

                                       GUI_WID_LOG("xy={%d,%d} ==> xy={%d,%d}\r\n",(int16_t)x,(int16_t)y,(int16_t)x1,(int16_t)y1);

                                       /*if(x1>=0 && x1<GUI_RES_X && y1>=0 && y1<GUI_RES_Y)//这个是显示在窗口0偏移的，测试用
                                       {
                                           fill_fb[(int16_t)y1*GUI_RES_X+(int16_t)x1]=pix_data[layer->w*(j)+k];
                                       }*/
                                       if(x1>=win_rg->start_x && x1<win_rg->start_x+win_rg->disp_w
                                           && y1>=win_rg->start_y && y1<win_rg->start_y+win_rg->disp_h)
                                       {
                                           //实际上，不管窗口如何偏移，数据填充到fb的都是从(0,0)开始，所以这里要减去窗口的起始值，得到从(0,0)的偏移
                                           fill_fb[((int16_t)y1-win_rg->start_y)*GUI_RES_X+(int16_t)x1-win_rg->start_x]
                                                   =pix_data[layer->w*(j)+k];
                                       }
                                       else
                                       {
                                           GUI_WID_LOG("rotation err!\r\n");
                                       }

                                   }
                              }
                        }
                        break;
                    }

                    case GUI_PIX_TYPE_RGB1:
                    default:
                    {
                        break;
                    }
                }

#if MEASURE_FILL_TIME
                t2=OS_GET_TICK_COUNT();

                OS_LOG("pix rot=%d (%d ms) \r\n",
                    t2-t1,OS_TICKS_2_MS(t2-t1));
#endif


                break;
            }

            /* 文本图层 */
            case LAYER_TYPE_TEXT:
            {
                gui_text_layer_attr_t *text_attr=(gui_text_layer_attr_t *)layer->attr;
                uint16_t font_h=0,bytes=0;

#if MEASURE_FILL_TIME
                uint32_t t1,t2;
                t1=OS_GET_TICK_COUNT();
#endif
                //取最大的一个字体的高度，确保行高对齐; 取最大文字所占字节，分配空间
                for(j=0;j<CHARACTER_SET_MAX;j++)
                {
                    if(text_attr->fonts[j])
                    {
                        if(font_h<text_attr->fonts[j]->font_attr.height)
                        {
                            font_h=text_attr->fonts[j]->font_attr.height;
                        }

                        if(bytes<text_attr->fonts[j]->font_attr.bytes_per_word)
                        {
                            bytes=text_attr->fonts[j]->font_attr.bytes_per_word;
                        }
                    }
                }
                if(font_h==0 || bytes==0)
                {
                    break;
                }

                if(__get_range(win_rg,&lyr_rg,&pix_rg))
                {
                    break;
                }

                //uint8_t pix_data[text_attr->fonts[]->font_attr.bytes_per_word];
                uint8_t *pix_data=(uint8_t *)OS_MALLOC(bytes);
                if(pix_data==NULL)
                    break;

                switch(text_attr->text_encoding)
                {
                     case TEXT_ENCODING_ASCII:
                         //break;

                     case TEXT_ENCODING_GB2312:
                     case TEXT_ENCODING_GB2312_2BYTES:
                     {
                         const font_info_t *font;
                         uint16_t text_x=0,text_y=0;

                         uint16_t t_w, t_h,                  //实际显示的文字宽高
                                  t_offset_x,t_offset_y,     //实际读取的文字xy偏移 --相当于图层偏移
                                  lyr_offset_x,lyr_offset_y; //文字在图层内的偏移  --相当于窗口偏移

                         uint16_t text_code;

                         font_data_read_t font_data_read;

                        for (j = 0; j < text_attr->text_len; j++)
                        {
                            text_code=text_attr->text_code[j];
                            if (IS_ASCII(text_code))
                            {
                                //非正常字符转为空格
                                if (text_code < FONT_ASCII_MIN)
                                {
                                    text_code = FONT_ASCII_MIN;
                                }
                                font=text_attr->fonts[CHARACTER_SET_ASCII];
                                font_data_read=font_ascii_data;
                            }

                            //中文
                            else
                            {
                                font=text_attr->fonts[CHARACTER_SET_GB2312];
                                font_data_read=font_gb2312_data;
                            }

                            //如果字体的宽大于图层宽，那么不显示（横向不显示半个文字，只有纵向显示不完整的文字）
                            if ( font== NULL || font->font_attr.width > lyr_rg.layer_w)
                            {
                                continue;            //TODO:结束循环
                            }

                            if (text_x + font->font_attr.width > lyr_rg.layer_w)
                            {
                                text_x = 0;
                                text_y += font_h;
                            }

                            //计算文字在图层内的位置
                            if ( (text_x + font->font_attr.width) <= pix_rg.read_offset_x)
                            {
                                goto next_word;
                            }
                            else if (text_x >= (pix_rg.read_offset_x+pix_rg.read_w) )
                            {
                                goto next_word;
                            }

                            if ( (text_y + font->font_attr.height) <= pix_rg.read_offset_y)
                            {
                                goto next_word;
                            }

                            if (text_y >= (pix_rg.read_offset_y + pix_rg.read_h))
                            {
                                break;//结束循环
                            }

                            //计算文字偏移
                            if(text_x<pix_rg.read_offset_x)
                            {
                                t_offset_x=pix_rg.read_offset_x-text_x;
                                t_w=font->font_attr.width-t_offset_x;
                                lyr_offset_x=0;
                            }
                            else
                            {
                                t_offset_x=0;
                                t_w=font->font_attr.width;
                                lyr_offset_x=text_x-pix_rg.read_offset_x;
                            }
                            if(text_x+t_w > (pix_rg.read_offset_x+pix_rg.read_w))
                            {
                                t_w= pix_rg.read_offset_x+pix_rg.read_w-text_x;
                            }

                            if(text_y<pix_rg.read_offset_y)
                            {
                                t_offset_y=pix_rg.read_offset_y-text_y;
                                t_h=font->font_attr.height-t_offset_y;
                                lyr_offset_y=0;
                            }
                            else
                            {
                                t_offset_y=0;
                                t_h=font->font_attr.height;
                                lyr_offset_y=text_y-pix_rg.read_offset_y;
                            }
                            if(text_y+t_h > (pix_rg.read_offset_y+pix_rg.read_h))
                            {
                                t_h= pix_rg.read_offset_y+pix_rg.read_h-text_y;
                            }

                            GUI_WID_LOG("t_w=%d,t_h=%d,t_x=%d,t_y=%d,lyr_x=%d,lyr_y=%d,text_x=%d,text_y=%d, \r\n",
                                    t_w, t_h,
                                    t_offset_x,t_offset_y,
                                    lyr_offset_x,lyr_offset_y,text_x,text_y);

                            if(t_w>0 && t_h>0)
                            {

                                if(!font_data_read(text_code, font, pix_data))
                                {
                                    uint16_t pix_idx, byte_idx, bit_idx, color;
                                    for (k = 0; k < t_h; k++)
                                    {
                                        for (uint16_t m = 0; m < t_w; m++)
                                        {
                                            pix_idx = font->font_attr.width * (k + t_offset_y) + t_offset_x + m;

                                            byte_idx = pix_idx >> 3;
                                            bit_idx = pix_idx & 0x7;
                                            color = pix_data[byte_idx] & (0x1 << bit_idx);

                                            if (color)
                                            {
                                                fill_fb[(k + win_rg->offset_y + lyr_offset_y) * GUI_RES_X + win_rg->offset_x + lyr_offset_x + m] =
                                                    layer->front_color;
                                            }
                                            else if(layer->transparent!=LAYER_ENABLE_TRANSP)
                                            {
                                                fill_fb[(k + win_rg->offset_y + lyr_offset_y) * GUI_RES_X + win_rg->offset_x + lyr_offset_x + m] =
                                                    layer->back_color;
                                            }
                                        }
                                    }

                                }
                            }

next_word:
                            text_x += font->font_attr.width;

                        }//end of for(j=0;j<text_attr->text_len;j++)

                        break;
                    }

                     case TEXT_ENCODING_UNICODE:
                         break;

                     case TEXT_ENCODING_UTF8:
                         break;

                     case TEXT_ENCODING_NONE:
                     default:
                     {

                     }
                }

                if(pix_data)
                {
                    OS_FREE(pix_data);
                }

#if MEASURE_FILL_TIME
                t2=OS_GET_TICK_COUNT();

                OS_LOG("txt=%d (%d ms) \r\n",
                    t2-t1,OS_TICKS_2_MS(t2-t1));
#endif
                break;
            }

            /* 几何图层 */
            case LAYER_TYPE_GEMETRY:
            {
                gui_geometry_layer_attr_t *geom_attr=(gui_geometry_layer_attr_t *)layer->attr;

                if(__get_range(win_rg,&lyr_rg,&pix_rg))
                {
                    break;
                }

                //TODO:以下坐标判断用了遍历的方式，太耗时了，需要改进
                switch(geom_attr->type)
                {
                    case GUI_GEOMETRY_POINT:
                    {
                        point_t *p=(point_t *)geom_attr->element;
                        if(p->x>=win_rg->start_x && p->x<win_rg->start_x+win_rg->disp_w
                           && p->y>=win_rg->start_y && p->y<win_rg->start_y+win_rg->disp_h)
                       {
                           fill_fb[(p->y-win_rg->start_y)*GUI_RES_X+p->x-win_rg->start_x]
                                   =layer->front_color;
                       }
                        break;
                    }
                    case GUI_GEOMETRY_LINE:
                    {

                        gui_geometry_line_t *line=(gui_geometry_line_t *)geom_attr->element;

                        //垂直 直线
                        if(line->points[0].x==(float)line->points[1].x)
                        {
                            for (j = 0; j < pix_rg.read_h; j++)
                            {
                                fill_fb[(j + win_rg->offset_y) * GUI_RES_X + win_rg->offset_x ] = layer->front_color;
                            }
                            break;
                        }

                        //水平 直线
                        else if(line->points[0].y==(float)line->points[1].y)
                        {
                            for (k = 0; k < pix_rg.read_w; k++)
                            {
                                fill_fb[ win_rg->offset_y * GUI_RES_X + win_rg->offset_x +k] = layer->front_color;
                            }
                            break;
                        }

                        //斜线
                        else
                        {
                            /*
                             * y=kx+b,k=(y1-y0)/(x1-x0)
                             * ==>b=y-kx
                             * ==>x=(y-b)/k
                             */

                            float dx=(float)line->points[0].x-(float)line->points[1].x;
                            float dy=(float)line->points[0].y-(float)line->points[1].y;
                            float line_k=dy/dx;//TODO:修复当斜率无限小或者无限大的情况
                            float line_b=(float)line->points[0].y-line_k*(float)line->points[0].x;

                            //取差值最大的轴为计算增量，因为差值小的那个轴可能会有相同的值
                            if(fabs(dx)>=fabs(dy))
                            {
                                for (k = 0; k < pix_rg.read_w; k++)
                                {
                                    //这里j表示 y, k表示 x
                                    //y=kx+b
                                    j=line_k*(k+pix_rg.read_offset_x+lyr_rg.layer_start_x)+line_b+0.5f;//0.5 为4舍5入

                                   if(j>=win_rg->start_y && j<win_rg->start_y+win_rg->disp_h)
                                   {
                                       fill_fb[(j-win_rg->start_y)*GUI_RES_X+k+win_rg->offset_x]
                                               =layer->front_color;
                                   }

                                   //和上面等效
                                   /*fill_fb[(j-lyr_rg.layer_start_y-pix_rg.read_offset_y + win_rg->offset_y)
                                           * GUI_RES_X + win_rg->offset_x + k] =layer->front_color;*/
                                }
                            }
                            else
                            {
                                for (j = 0; j < pix_rg.read_h; j++)
                                {
                                    //这里j表示 y, k表示 x
                                    //(y-b)/k
                                    k=(j+pix_rg.read_offset_y+lyr_rg.layer_start_y-line_b)/line_k+0.5f;//0.5 为4舍5入

                                   if(k>=win_rg->start_x && k<win_rg->start_x+win_rg->disp_w)
                                   {
                                       fill_fb[(j+win_rg->offset_y)*GUI_RES_X+k-win_rg->start_x]
                                               =layer->front_color;
                                   }
                                }
                            }
                        }

                        break;
                    }
                    case GUI_GEOMETRY_POLYGON:
                    {
//                        gui_geometry_polygon_t *polygon=(gui_geometry_polygon_t *)geom_attr->element;
                        //TODO:待实现
                        break;
                    }
                    case GUI_GEOMETRY_CIRCLE:
                    {
//                        gui_geometry_circle_t *circle=(gui_geometry_circle_t *)geom_attr->element;
                        //TODO:待实现
                        break;
                    }
                    case GUI_GEOMETRY_ARC:
                    {
//                        gui_geometry_circle_t *circle=(gui_geometry_circle_t *)geom_attr->element;
                        //TODO:待实现
                        break;
                    }

                    default:/* 不支持的类型 */
                    {
                        break;
                    }
                }

                break;
            }

            /*
             * 柱状图
             */
            case LAYER_TYPE_HISTOGRAM:
            {
                gui_histogram_layer_attr_t *histogram_attr=(gui_histogram_layer_attr_t *)layer->attr;
                if(__get_range(win_rg,&lyr_rg,&pix_rg))
                {
                    break;
                }

                uint16_t _w, _h,                   //实际显示的柱子宽高
                          _offset_x,_offset_y,     //实际读取的柱子xy偏移 --相当于图层偏移
                          lyr_offset_x,lyr_offset_y; //柱子在图层内的偏移  --相当于窗口偏移

                for(j=0;j<histogram_attr->num;j++)
                {
                    gui_pillar_t *pillar=&histogram_attr->pillars[j];

                    //计算柱子在图层内的位置
                    if ( (pillar->x + pillar->w) <= pix_rg.read_offset_x)
                    {
                        continue;
                    }
                    else if (pillar->x >= (pix_rg.read_offset_x+pix_rg.read_w) )
                    {
                        continue;
                    }

                    if ( (pillar->y + pillar->h) <= pix_rg.read_offset_y)
                    {
                        continue;
                    }

                    if (pillar->y >= (pix_rg.read_offset_y + pix_rg.read_h))
                    {
                        continue;
                    }

                    //计算每个柱子的偏移
                    if(pillar->x<pix_rg.read_offset_x)
                    {
                        _offset_x=pix_rg.read_offset_x-pillar->x;
                        _w=pillar->w-_offset_x;
                        lyr_offset_x=0;
                    }
                    else
                    {
                        _offset_x=0;
                        _w=pillar->w;
                        lyr_offset_x=pillar->x-pix_rg.read_offset_x;
                    }
                    if(pillar->x+_w > (pix_rg.read_offset_x+pix_rg.read_w))
                    {
                        _w= pix_rg.read_offset_x+pix_rg.read_w-pillar->x;
                    }

                    if(pillar->y<pix_rg.read_offset_y)
                    {
                        _offset_y=pix_rg.read_offset_y-pillar->y;
                        _h=pillar->h-_offset_y;
                        lyr_offset_y=0;
                    }
                    else
                    {
                        _offset_y=0;
                        _h=pillar->h;
                        lyr_offset_y=pillar->y-pix_rg.read_offset_y;
                    }
                    if(pillar->y+_h > (pix_rg.read_offset_y+pix_rg.read_h))
                    {
                        _h= pix_rg.read_offset_y+pix_rg.read_h-pillar->y;
                    }

                    GUI_WID_LOG("_w=%d,_h=%d,o_x=%d,o_y=%d,lyr_x=%d,lyr_y=%d,p_x=%d,p_y=%d, \r\n",
                            _w, _h,
                            _offset_x,_offset_y,
                            lyr_offset_x,lyr_offset_y,pillar->x,pillar->y);

                    if(_w>0 && _h>0)
                    {
                        for (k = 0; k < _h; k++)
                        {
                            for (uint16_t m = 0; m < _w; m++)
                            {
                                fill_fb[(k + win_rg->offset_y + lyr_offset_y) * GUI_RES_X + win_rg->offset_x + lyr_offset_x + m] =
                                    pillar->color;
                            }
                        }
                    }
                }

                break;
            }
        }

    }


    return 0;
}
/*------------------------------------------------------------------------------------------------*/
/*
 * 开/关闭显示刷新
 * 在关闭屏幕时，设置窗口状态为 GUI_WIN_CLOSE ，亮屏时设置为 GUI_WIN_OPEN，以避免一直在刷新屏幕
 *
 */

typedef enum{
    GUI_WIN_CLOSE,
    GUI_WIN_OPEN,
    GUI_WIN_UNKNOW,
}gui_display_state_t;
static uint8_t gui_display_state=GUI_WIN_UNKNOW;

void gui_display_close()
{
    gui_display_state=GUI_WIN_CLOSE;
}

void gui_display_open()
{
    gui_display_state=GUI_WIN_OPEN;
}

#define gui_display_is_close() (gui_display_state==GUI_WIN_CLOSE)

/*------------------------------------------------------------------------------------------------*/

/*
 * 记录当前刷新的窗口是否改变，用于加速显示
 */
static gui_window_t *last_disp_gwin=NULL;

/**
@brief : 显示窗口：实时加载窗口内容显示，
                            此方法适合用于窗口内容有更新的显示，对于窗口内容没有更新只是窗口坐标变化的，请使用 gui_window_show_pre 方法

                    注：如果窗口未加载完成，则不显示

@param :
-hdl       窗口句柄，如果为hdl=NULL则显示栈顶窗口

@retval:
- 无
*/
static void gui_window_render(gui_window_hdl_t hdl)
{
    gui_window_t *gwin;
    uint16_t *disp_fb;

#if !USE_SINGLE_BUF_RENDER
    uint16_t *fill_fb;
#endif
    gui_lock();

#if MEASURE_RENDER_TIME
    uint32_t t1=0,t2=0;
    t1=OS_GET_TICK_COUNT();
#endif

    if(!hdl )
    {
        gwin=(gui_window_t *)gui_window_get_top();
        if(!gwin)
        {
            GUI_WID_LOG("no window to show!\r\n");
            gui_unlock();
            return ;
        }
    }
    else
    {
        gwin=(gui_window_t *)hdl;
    }


    if(!gwin->loaded)
    {
        GUI_WID_LOG("dial window not loaded!\r\n");
        gui_unlock();
        return ;
    }
#if !USE_SINGLE_BUF_RENDER
    fill_fb=gui_get_back_fb();
#endif
    disp_fb=gui_get_front_fb();

#if !USE_SINGLE_BUF_RENDER

    #if GUI_BUF_NEED_DIV
    {
        for(int i=0;i<GUI_BUF_DIV;i++)
        {
            if(!__gui_window_fb_fill((gui_window_hdl_t)gwin,fill_fb,0,GUI_RES_X,i*GUI_RES_Y_DIV,GUI_RES_Y_DIV))
            {

                #if USE_DMA_SPEED_UP_RENDER
                {
                    //用DMA加速传输 ,耗时为 0 ms
                    dma_start((uint32_t)fill_fb,(uint32_t)disp_fb,
                        GUI_RES_Y_DIV*GUI_RES_X*GUI_BYTES_PER_PIX);
                }
                #else
                {
                    //这里耗时 7ms
                    for(int i=0;i<GUI_RES_Y_DIV;i++)
                    {
                        for (int j = 0; j < GUI_RES_X; j++)
                        {
                            disp_fb[i * GUI_RES_X + j] = fill_fb[i * GUI_RES_X + j];
                        }
                    }
                }
                #endif

            }
            gui_flush(disp_fb,0,i*GUI_RES_Y_DIV,GUI_RES_X,(i+1)*GUI_RES_Y_DIV,false);
        }
    }
    #else
    {
        if(!__gui_window_fb_fill((gui_window_hdl_t)gwin,fill_fb,0,GUI_RES_X,0,GUI_RES_Y))
        {
            #if USE_DMA_SPEED_UP_RENDER
            {
                //用DMA加速传输 ,耗时为 0 ms
                dma_start((uint32_t)fill_fb,(uint32_t)disp_fb,GUI_RES_Y*GUI_RES_X*GUI_BYTES_PER_PIX);
            }
            #else
            {
                //这里耗时 7ms
                for(int i=0;i<GUI_RES_Y;i++)
                {
                    for (int j = 0; j < GUI_RES_X; j++)
                    {
                        disp_fb[i * GUI_RES_X + j] = fill_fb[i * GUI_RES_X + j];
                    }
                }
            }
            #endif

        }
        gui_flush(disp_fb,0,0,GUI_RES_X,GUI_RES_Y,false);
    }
    #endif

#else

    #if GUI_BUF_NEED_DIV
    {
        for(int i=0;i<GUI_BUF_DIV;i++)
        {
            __gui_window_fb_fill((gui_window_hdl_t)gwin,disp_fb,0,GUI_RES_X,i*GUI_RES_Y_DIV,GUI_RES_Y_DIV);
            gui_flush(disp_fb,0,i*GUI_RES_Y_DIV,GUI_RES_X,(i+1)*GUI_RES_Y_DIV,true);
        }
    }
    #else
    {
        __gui_window_fb_fill((gui_window_hdl_t)gwin,disp_fb,0,GUI_RES_X,0,GUI_RES_Y);
        gui_flush(disp_fb,0,0,GUI_RES_X,GUI_RES_Y,true);
    }
    #endif
#endif

    last_disp_gwin=NULL;

    gui_unlock();

#if MEASURE_RENDER_TIME
    t2=OS_GET_TICK_COUNT();
    OS_LOG("render time=%d (%d ms) \r\n",t2-t1,OS_TICKS_2_MS(t2-t1));
#endif

}


void gui_window_show(gui_window_hdl_t hdl)
{
    if(gui_display_is_close())
        return ;

    gui_window_render(hdl);
}

/*------------------------------------------------------------------------------------------------*/

/**
@brief : 显示窗口:使用预加载的窗口内容填充显示，预加载的内容保存在 back_fb ，显示的内容使用 front_fb

         注意：   此方法适合用于窗口内容没有更新只是窗口坐标变化的（如滚动效果、滑动效果），对于窗口内容有更新的显示请使用 gui_window_show 方法

        同一窗口显示加速设计：
              记录上一次显示的坐标 （last_disp_x ，last_disp_y），与本次坐标比较 （gwin->x，gwin->y）
              得到差值（ diff_x,diff_y），将上一次的显示内容 back_fb 移动（ diff_x,diff_y），
              然后从flash中读取新的偏移内容


@param :
-hdl       窗口句柄，如果为hdl=NULL则显示栈顶窗口

@retval:
- 无
*/
static void gui_window_render_pre(gui_window_hdl_t hdl)
{
    gui_window_t *gwin;
    uint16_t *disp_fb;

#if !USE_SINGLE_BUF_RENDER_PRE
    uint16_t *fill_fb;
#endif

    static uint16_t last_x=0,last_y=0;
    int16_t _x,_y;
    int i
#if !USE_DMA_SPEED_UP_RENDER_PRE
    ,j
#endif
    ;

    gui_lock();

    if(!hdl )
    {
        gwin=(gui_window_t *)gui_window_get_top();
        if(!gwin)
        {
            GUI_WID_LOG("no window to show!\r\n");
            gui_unlock();
            return ;
        }
    }
    else
    {
        gwin=(gui_window_t *)hdl;
    }


    if(!gwin->loaded)
    {
        GUI_WID_LOG("window not loaded!\r\n");
        gui_unlock();
        return ;
    }

#if !USE_SINGLE_BUF_RENDER_PRE
    fill_fb=gui_get_back_fb();
#endif
    disp_fb=gui_get_front_fb();


#if MEASURE_RENDER_PRE_TIME
    uint32_t t1=0,t2=0;
    t1=OS_GET_TICK_COUNT();
#endif

#define DMA_START_PIX (100)    //使用DMA时复制像素数据的数据最小像素点，小于该值不使用DMA

    //窗口变化需要重新加载
    if(last_disp_gwin!=gwin)
    {
        __gui_window_fb_fill((gui_window_hdl_t)gwin,disp_fb,0,GUI_RES_X,0,GUI_RES_Y);
    }
    else
    {
        _x=gwin->x-last_x;
        _y=gwin->y-last_y;

#if !USE_SINGLE_BUF_RENDER_PRE
        /*
         * 窗口左右移动
         */
        if(_x!=0)
        {
            if(_x>0)//窗口向左移动
            {
                #if USE_DMA_SPEED_UP_RENDER_PRE
                {
                    if((GUI_RES_X-_x)>=DMA_START_PIX)
                    {
                        for(i=0;i<GUI_RES_Y;i++)
                        {
                            dma_start_k((uint32_t)&disp_fb[i*GUI_RES_X+_x],
                                (uint32_t)&disp_fb[i*GUI_RES_X],
                                (GUI_RES_X-_x)*GUI_BYTES_PER_PIX,true,true,2);
                        }
                    }
                    else
                    {
                        for(i=0;i<GUI_RES_Y;i++)
                        {
                            for (j = 0; j < GUI_RES_X-_x; j++)
                            {
                                disp_fb[i*GUI_RES_X+j] = disp_fb[i*GUI_RES_X+j+_x];
                            }
                        }
                    }

                    __gui_window_fb_fill(gwin,fill_fb,GUI_RES_X-_x,_x,0,GUI_RES_Y);

                    if(_x>=DMA_START_PIX)
                    {
                        for(i=0;i<GUI_RES_Y;i++)
                        {
                            dma_start_k((uint32_t)&fill_fb[i*GUI_RES_X],
                                (uint32_t)&disp_fb[i*GUI_RES_X+(GUI_RES_X-_x)],
                                (_x)*GUI_BYTES_PER_PIX,true,true,2);
                        }
                    }
                    else
                    {
                        for(i=0;i<GUI_RES_Y;i++)
                        {
                            for (j = 0; j < _x; j++)
                            {
                                disp_fb[i*GUI_RES_X+(GUI_RES_X-_x)+j] = fill_fb[i*GUI_RES_X+j];
                            }
                        }
                    }
                }
                #else
                {
                    for(i=0;i<GUI_RES_Y;i++)
                    {
                        for (j = 0; j < GUI_RES_X-_x; j++)
                        {
                            disp_fb[i*GUI_RES_X+j] = disp_fb[i*GUI_RES_X+j+_x];
                        }
                    }

                    __gui_window_fb_fill(gwin,fill_fb,GUI_RES_X-_x,_x,0,GUI_RES_Y);

                    for(i=0;i<GUI_RES_Y;i++)
                    {
                        for (j = 0; j < _x; j++)
                        {
                            disp_fb[i*GUI_RES_X+(GUI_RES_X-_x)+j] = fill_fb[i*GUI_RES_X+j];
                        }
                    }
                }
                #endif
            }
            else
            {
                _x=-_x;
                #if USE_DMA_SPEED_UP_RENDER_PRE
                {
                    for(i=0;i<GUI_RES_Y;i++)//这里地址是倒着的，不方便用DMA
                    {
                        for (j = 0; j < GUI_RES_X-_x; j++)
                        {
                            disp_fb[i*GUI_RES_X+GUI_RES_X-1-j] = disp_fb[i*GUI_RES_X+GUI_RES_X-1-j-_x];
                        }
                    }

                    __gui_window_fb_fill(gwin,fill_fb,0,_x,0,GUI_RES_Y);

                    if(_x>=DMA_START_PIX)
                    {
                        for(i=0;i<GUI_RES_Y;i++)
                        {
                            dma_start_k((uint32_t)&fill_fb[i*GUI_RES_X],
                                (uint32_t)&disp_fb[i*GUI_RES_X],
                                (_x)*GUI_BYTES_PER_PIX,true,true,2);
                        }
                    }
                    else
                    {
                        for(i=0;i<GUI_RES_Y;i++)
                        {
                            for (j = 0; j < _x; j++)
                            {
                                disp_fb[i*GUI_RES_X+j] = fill_fb[i*GUI_RES_X+j];
                            }
                        }
                    }
                }
                #else
                {
                    for(i=0;i<GUI_RES_Y;i++)
                    {
                        for (j = 0; j < GUI_RES_X-_x; j++)
                        {
                            disp_fb[i*GUI_RES_X+GUI_RES_X-1-j] = disp_fb[i*GUI_RES_X+GUI_RES_X-1-j-_x];
                        }
                    }

                    __gui_window_fb_fill(gwin,fill_fb,0,_x,0,GUI_RES_Y);

                    for(i=0;i<GUI_RES_Y;i++)
                    {
                        for (j = 0; j < _x; j++)
                        {
                            disp_fb[i*GUI_RES_X+j] = fill_fb[i*GUI_RES_X+j];
                        }
                    }
                }
                #endif
            }

        }

        /*
         * 窗口上下移动
         */
        if(_y!=0)
        {
           if(_y>0)//窗口向下移动
           {
               for(i=0;i<GUI_RES_Y-_y;i++)
               {
                   #if USE_DMA_SPEED_UP_RENDER_PRE
                   {
                        //用DMA加速传输 ,
                        dma_start((uint32_t)&disp_fb[(i+_y)*GUI_RES_X],
                            (uint32_t)&disp_fb[i*GUI_RES_X],
                            GUI_RES_X*GUI_BYTES_PER_PIX);
                   }
                   #else
                   {
                       for (j = 0; j < GUI_RES_X; j++)
                       {
                           disp_fb[i*GUI_RES_X+j] = disp_fb[(i+_y)*GUI_RES_X+j];
                       }
                   }
                   #endif
               }

               __gui_window_fb_fill(gwin,fill_fb,0,GUI_RES_X,GUI_RES_Y-_y,_y);

               for(i=0;i<_y;i++)
               {
                    #if USE_DMA_SPEED_UP_RENDER_PRE
                   {
                        //用DMA加速传输 ,
                        dma_start((uint32_t)&fill_fb[i*GUI_RES_X],
                            (uint32_t)&disp_fb[((GUI_RES_Y-_y)+i)*GUI_RES_X],
                            GUI_RES_X*GUI_BYTES_PER_PIX);
                   }
                   #else
                   {
                       for (j = 0; j < GUI_RES_X ; j++)
                       {
                           disp_fb[((GUI_RES_Y-_y)+i)*GUI_RES_X+j] = fill_fb[i*GUI_RES_X+j];
                       }
                   }
                    #endif
               }
           }
           else
           {
               _y=-_y;
               for(i=0;i<GUI_RES_Y-_y;i++)
               {
                    #if USE_DMA_SPEED_UP_RENDER_PRE
                    {
                        //用DMA加速传输 ,
                        dma_start((uint32_t)&disp_fb[(GUI_RES_Y-1-_y-i)*GUI_RES_X],
                            (uint32_t)&disp_fb[(GUI_RES_Y-1-i)*GUI_RES_X],
                            GUI_RES_X*GUI_BYTES_PER_PIX);
                    }
                    #else
                    {
                       for (j = 0; j < GUI_RES_X; j++)
                       {
                           disp_fb[(GUI_RES_Y-1-i)*GUI_RES_X+j] = disp_fb[(GUI_RES_Y-1-_y-i)*GUI_RES_X+j];
                       }
                    }
                    #endif
               }

               __gui_window_fb_fill(gwin,fill_fb,0,GUI_RES_X,0,_y);

               for(i=0;i<_y;i++)
               {
                   #if USE_DMA_SPEED_UP_RENDER_PRE
                   {
                        //用DMA加速传输 ,
                        dma_start((uint32_t)&fill_fb[i*GUI_RES_X],(uint32_t)&disp_fb[i*GUI_RES_X],
                            GUI_RES_X*GUI_BYTES_PER_PIX);
                   }
                   #else
                   {
                       for (j = 0; j < GUI_RES_X; j++)
                       {
                           disp_fb[i*GUI_RES_X+j] = fill_fb[i*GUI_RES_X+j];
                       }
                   }
                   #endif
               }
           }

        }
#else
        /*
         * 窗口左右移动
         */
        if(_x!=0)
        {
            if(_x>0)//窗口向左移动
            {
                #if USE_DMA_SPEED_UP_RENDER_PRE
                {
                    /*
                     * DMA传输有长度限制，为不能一次传输大于 0xffff 的
                     * 所以这里分三次传输
                     *
                     * 注意：
                     *  这里分2次传输只适合360*360、240*240分辨率的情况，其他分辨率要调试下看会不会有什么bug
                     *  （主要是DMA不对齐会传输失败）
                     *
                     * 耗时1-3ms
                     */
                    dma_start_k((uint32_t)&disp_fb[_x+GUI_RES_X*GUI_RES_Y/2*0],
                        (uint32_t)&disp_fb[0+GUI_RES_X*GUI_RES_Y/2*0],
                        (GUI_RES_X*GUI_RES_Y/2-_x)*GUI_BYTES_PER_PIX,true,true,2);

                    dma_start_k((uint32_t)&disp_fb[_x+GUI_RES_X*GUI_RES_Y/2*1],
                        (uint32_t)&disp_fb[0+GUI_RES_X*GUI_RES_Y/2*1],
                        (GUI_RES_X*GUI_RES_Y/2-_x)*GUI_BYTES_PER_PIX,true,true,2);

                }
                #else
                {
                    for(i=0;i<GUI_RES_Y;i++)
                    {
                        for (j = 0; j < GUI_RES_X-_x; j++)
                        {
                            disp_fb[i*GUI_RES_X+j] = disp_fb[i*GUI_RES_X+j+_x];
                        }
                    }
                }
                #endif

                __gui_window_fb_fill(gwin,&disp_fb[GUI_RES_X-_x],GUI_RES_X-_x,_x,0,GUI_RES_Y);
            }
            else
            {
                _x=-_x;
                #if USE_DMA_SPEED_UP_RENDER_PRE
                {
                    //这里地址是倒着的，不能直接用DMA。所以先用DMA让 disp_fb 往前移（ disp_fb 在定义buf时前面必须要额外预留多一行），再整体向下移动
                    //耗时11ms

                    uint16_t *new_disp_fb=(uint16_t *)(disp_fb-GUI_RES_X);

                    dma_start_k((uint32_t)&disp_fb[0],(uint32_t)&new_disp_fb[_x],
                        (GUI_RES_X*GUI_RES_Y/2-_x)*GUI_BYTES_PER_PIX,true,true,2);

                    dma_start_k((uint32_t)&disp_fb[GUI_RES_X*GUI_RES_Y/2*1],
                        (uint32_t)&new_disp_fb[GUI_RES_X*GUI_RES_Y/2*1+_x],
                        (GUI_RES_X*GUI_RES_Y/2-_x)*GUI_BYTES_PER_PIX,true,true,2);

                   for(i=0;i<GUI_RES_Y;i++)
                   {
                        //用DMA加速传输 ,
                        dma_start((uint32_t)&new_disp_fb[(GUI_RES_Y-1-i)*GUI_RES_X],
                            (uint32_t)&disp_fb[(GUI_RES_Y-1-i)*GUI_RES_X],
                            GUI_RES_X*GUI_BYTES_PER_PIX);
                   }
                }
                #else
                {
                    for(i=0;i<GUI_RES_Y;i++)
                    {
                        for (j = 0; j < GUI_RES_X-_x; j++)
                        {
                            disp_fb[i*GUI_RES_X+GUI_RES_X-1-j] = disp_fb[i*GUI_RES_X+GUI_RES_X-1-j-_x];
                        }
                    }
                }
                #endif

                __gui_window_fb_fill(gwin,disp_fb,0,_x,0,GUI_RES_Y);
            }

        }

        /*
         * 窗口上下移动
         */
        if(_y!=0)
        {
           if(_y>0)//窗口向上移动
           {
               for(i=0;i<GUI_RES_Y-_y;i++)
               {
                   #if USE_DMA_SPEED_UP_RENDER_PRE
                   {
                        //用DMA加速传输 ,
                        dma_start((uint32_t)&disp_fb[(i+_y)*GUI_RES_X],(uint32_t)&disp_fb[i*GUI_RES_X],
                            GUI_RES_X*GUI_BYTES_PER_PIX);
                   }
                   #else
                   {
                       for (j = 0; j < GUI_RES_X; j++)
                       {
                           disp_fb[i*GUI_RES_X+j] = disp_fb[(i+_y)*GUI_RES_X+j];
                       }
                   }
                   #endif
               }

               __gui_window_fb_fill(gwin,&disp_fb[(GUI_RES_Y-_y)*GUI_RES_X],0,GUI_RES_X,GUI_RES_Y-_y,_y);

           }
           else
           {
               _y=-_y;
               for(i=0;i<GUI_RES_Y-_y;i++)
               {
                   #define __END_Y (GUI_RES_Y-1)
                   #if USE_DMA_SPEED_UP_RENDER_PRE
                   {
                        //用DMA加速传输 ,
                        dma_start((uint32_t)&disp_fb[(__END_Y-_y-i)*GUI_RES_X],
                            (uint32_t)&disp_fb[(__END_Y-i)*GUI_RES_X],
                            GUI_RES_X*GUI_BYTES_PER_PIX);
                   }
                   #else
                   {
                       for (j = 0; j < GUI_RES_X; j++)
                       {
                           disp_fb[(__END_Y-i)*GUI_RES_X+j] = disp_fb[(__END_Y-_y-i)*GUI_RES_X+j];
                       }
                   }
                   #endif
               }

               __gui_window_fb_fill(gwin,disp_fb,0,GUI_RES_X,0,_y);

           }

        }
#endif

    }

    last_disp_gwin=gwin;
    last_x=gwin->x;
    last_y=gwin->y;

    gui_flush(disp_fb,0,0,GUI_RES_X,GUI_RES_Y,false);

#if MEASURE_RENDER_PRE_TIME
    t2=OS_GET_TICK_COUNT();
    OS_LOG("rd pre=%d (%d ms) \r\n",t2-t1,OS_TICKS_2_MS(t2-t1));
#endif

    gui_unlock();
}


void gui_window_show_pre(gui_window_hdl_t hdl)
{
    if(gui_display_is_close())
        return ;

    /*
     * 分屏情况下暂时不能支持这种方式刷屏，速度太慢
     */
#if GUI_BUF_NEED_DIV
    return ;
#endif

    gui_window_render_pre(hdl);
}

/*------------------------------------------------------------------------------------------------*/

static uint16_t last_top_pix=0;
static bool window_relaod=true;

/**
@brief : 显示栈顶2个窗口组合成的窗口，如果没有2个窗口，那么不做处理

@param :
-top_pix    栈顶窗口显示的像素（栈顶第二窗口像素为 GUI_RES_X-top_pix）
-effect     两个窗口组合的效果

@retval:
- 无

 * 注：
 *
 * 切换两个窗口显示加速设计：
 *
 * 把旧窗口（上一次运行的APP窗口，last_gwin）的内容加载到 front_fb  ，新窗口（本次运行的APP窗口，gwin）的内容加载到 back_fb
 * 1 如果TP移动方向不改变，那么显示到LCD的内容（front_fb）往同一方向移动同时用back_fb填充，过程中不需要重新读取flash
 * 2 如果TP方向改变，那么last_gwin和gwin要调换，同时需要加载gwin到back_fb，接着使用步骤1操作
 *
 * TODO & BUG：
 *      1） 该方法在TP快速左右滑动时，依然明显感觉到显示有卡顿，因为首次读取窗口的时间并没有优化！
 *      2）这里的方法不支持 单buf处理，必须要开两个buf
 *
 *      3)目前这里的刷新方法仅支持屏 GUI_RES_X 和 GUI_RES_Y 相同的情况，如两者不相等需要修改实现方法
 *
 */
static void gui_window_render_mix(uint16_t top_pix,gui_window_effect_t effect)
{
    gui_window_t *gwin,*last_gwin;

    uint16_t *disp_fb;

#if !USE_SINGLE_BUF_RENDER_MIX
    uint16_t *fill_fb;
#endif


#if MEASURE_RENDER_MIX_TIME
    uint32_t t1=0,t2=0,t3=0,t4=0;
#endif

    gui_lock();

    gwin=(gui_window_t *)gui_window_get_top();
    last_gwin=(gui_window_t *)gui_window_get_top2();

    if(!gwin || !last_gwin)
    {
        GUI_WID_LOG("no 2 window to show!\r\n");
        gui_unlock();
        return ;
    }


#if USE_SINGLE_BUF_RENDER_MIX
    disp_fb=gui_get_front_fb();
#else
    disp_fb=gui_get_front_fb();
    fill_fb=gui_get_back_fb();
#endif

    //首次加载时要重新置标志，重新读取数据
    if(gwin->loaded || last_gwin->loaded)
    {
        window_relaod=true;
    }

    //如果进入切换窗口，则要把窗口的属性设置为加载中
    gwin->loaded=false;
    last_gwin->loaded=false;

    /*
     * 计算加载的窗口方向和像素
     */
    uint16_t diff_pix;                        /* 本次显示窗口和上一次相比移动的像素 */
    gui_window_t *load_gwin;
    static gui_window_t *last_load_gwin=NULL;
    int i,j;

    if(window_relaod )
    {
        last_top_pix=0;
    }

    if(top_pix>last_top_pix)//正向加载
    {
        load_gwin=gwin;
    }
    else if(top_pix<last_top_pix)
    {
        load_gwin=last_gwin;//反向加载
    }
    else
    {
        load_gwin=last_load_gwin;
    }

    diff_pix=top_pix>last_top_pix?top_pix-last_top_pix:last_top_pix-top_pix;

    //TODO:
    //    top_pix=GUI_RES_X，last_top_pix=0会导致无法显示新窗口
    //    --但目前不会出现这种情况，因为top_pix不会从0直接跳到 GUI_RES_X(GUI_RES_Y)
    //
    //TODO:如果GUI_RES_X和GUI_RES_Y不相等，这里需要分条件判断
    //
    if(diff_pix>=GUI_RES_X)
    {
        diff_pix=0;
    }


#if MEASURE_RENDER_MIX_TIME
    t1=OS_GET_TICK_COUNT();
#endif

    if(window_relaod || last_load_gwin!=load_gwin)
    {
        window_relaod=false;

#if !USE_SINGLE_BUF_RENDER_MIX
        __gui_window_fb_fill((gui_window_hdl_t)load_gwin,fill_fb,0,GUI_RES_X,0,GUI_RES_Y);
#endif

//        OS_INFO("reload=%x\r\n",(uint32_t)load_gwin);
    }
    last_load_gwin=load_gwin;


#if MEASURE_RENDER_MIX_TIME
    t2=OS_GET_TICK_COUNT();
#endif

#if !USE_SINGLE_BUF_RENDER_MIX

    if(diff_pix>0)
    {
        //两个窗口组合
        switch((int)effect)
        {
            case GUI_EFFECT_NONE://直接显示 disp_fb
            {
                break;
            }

            /* 从左->右 滚动 */
            case GUI_EFFECT_ROLL_L2R:
            {
                if(load_gwin==gwin)//正向
                {
                    //旧窗口向右移动
                    if(diff_pix<GUI_RES_X)
                    {
                        for(i=0;i<GUI_RES_Y;i++)
                        {
                            for (j = 0; j < GUI_RES_X-top_pix; j++)
                            {
                                disp_fb[i*GUI_RES_X+GUI_RES_X-1-j] = disp_fb[i*GUI_RES_X+GUI_RES_X-1-j-diff_pix];
                            }
                        }
                    }

                    //新窗口向右移
                    for(i=0;i<GUI_RES_Y;i++)
                    {
                        for (j = 0; j < top_pix; j++)
                        {
                            disp_fb[i*GUI_RES_X+j] = fill_fb[i*GUI_RES_X+GUI_RES_X-top_pix+j];
                        }
                    }
                }
                else //if(load_gwin==last_gwin)//反向
                {
                    //旧窗口向左移动
                    if(diff_pix<GUI_RES_X)
                    {
                        for(i=0;i<GUI_RES_Y;i++)
                        {
                            for (j = 0; j < top_pix; j++)
                            {
                                disp_fb[i*GUI_RES_X+j] = disp_fb[i*GUI_RES_X+j+diff_pix];
                            }
                        }
                    }

                    //新窗口向左移
                    for(i=0;i<GUI_RES_Y;i++)
                    {
                        for (j = 0; j < GUI_RES_X-top_pix; j++)
                        {
                            disp_fb[i*GUI_RES_X+j+top_pix] = fill_fb[i*GUI_RES_X+j];
                        }
                    }
                }

                break;
            }

            /* 从右->左 滚动 */
            case GUI_EFFECT_ROLL_R2L:
            {
                if(load_gwin==gwin)//正向
                {
                    //旧窗口向左移动
                    if(diff_pix<GUI_RES_X)
                    {
                        for(i=0;i<GUI_RES_Y;i++)
                        {
                            for (j = 0; j < GUI_RES_X-top_pix; j++)
                            {
                                disp_fb[i*GUI_RES_X+j] = disp_fb[i*GUI_RES_X+j+diff_pix];
                            }
                        }
                    }

                    //新窗口向左移动
                    for(i=0;i<GUI_RES_Y;i++)
                    {
                        for (j = 0; j < top_pix; j++)
                        {
                            disp_fb[i*GUI_RES_X+j+GUI_RES_X-top_pix] = fill_fb[i*GUI_RES_X+j];
                        }
                    }
                }
                else //if(load_gwin==last_gwin)//反向
                {
                    //旧窗口向右移动
                    if(diff_pix<GUI_RES_X)
                    {
                        for(i=0;i<GUI_RES_Y;i++)
                        {
                            for (j = 0; j < top_pix; j++)
                            {
                                disp_fb[i*GUI_RES_X+GUI_RES_X-1-j] = disp_fb[i*GUI_RES_X+GUI_RES_X-1-j-diff_pix];
                            }
                        }
                    }

                    //新窗口向右移
                    for(i=0;i<GUI_RES_Y;i++)
                    {
                        for (j = 0; j < GUI_RES_X-top_pix; j++)
                        {
                            disp_fb[i*GUI_RES_X+j] = fill_fb[i*GUI_RES_X+top_pix+j];
                        }
                    }
                }

                break;
            }

            /* 从上->下刷 */
            case GUI_EFFECT_CONVERT_U2D:
            {
                if(load_gwin==gwin)//正向
                {
                    //旧窗口不移动

                    //新窗口向下移动（显示下半部分）
                    for(i=0;i<top_pix;i++)
                    {
#if USE_DMA_SPEED_UP_RENDER_MIX
                        dma_start((uint32_t)&fill_fb[(GUI_RES_Y-top_pix+i)*GUI_RES_X],(uint32_t)&disp_fb[i*GUI_RES_X],
                            GUI_RES_X*GUI_BYTES_PER_PIX);
#else
                        for (j = 0; j < GUI_RES_X; j++)
                        {
                            disp_fb[i*GUI_RES_X+j] = fill_fb[(GUI_RES_Y-top_pix+i)*GUI_RES_X+j];
                        }
#endif
                    }
                }
                else //if(load_gwin==last_gwin)//反向
                {
                    //旧窗口向上移动
                    if(diff_pix<GUI_RES_Y)
                    {
                        for(i=0;i<top_pix;i++)
                        {
#if USE_DMA_SPEED_UP_RENDER_MIX
                            dma_start((uint32_t)&disp_fb[(diff_pix+i)*GUI_RES_X],(uint32_t)&disp_fb[i*GUI_RES_X],
                                GUI_RES_X*GUI_BYTES_PER_PIX);
#else
                            for (j = 0; j < GUI_RES_X; j++)
                            {
                                disp_fb[i*GUI_RES_X+j] = disp_fb[(diff_pix+i)*GUI_RES_X+j];
                            }
#endif
                        }
                    }

                    //新窗不移动（显示下半部分）
                    for(i=0;i<GUI_RES_Y-top_pix;i++)
                    {
#if USE_DMA_SPEED_UP_RENDER_MIX
                        dma_start((uint32_t)&fill_fb[(top_pix+i)*GUI_RES_X],(uint32_t)&disp_fb[(top_pix+i)*GUI_RES_X],
                            GUI_RES_X*GUI_BYTES_PER_PIX);
#else
                        for (j = 0; j < GUI_RES_X; j++)
                        {
                            disp_fb[(top_pix+i)*GUI_RES_X+j] = fill_fb[(top_pix+i)*GUI_RES_X+j];
                        }
#endif
                    }
                }
                break;
            }

            /* 从下->上刷 */
            case GUI_EFFECT_CONVERT_D2U:
            {
                if(load_gwin==gwin)//正向
                {
                    //旧窗口不移动

                    //新窗口向上移动（显示上半部分）
                    for(i=0;i<top_pix;i++)
                    {
#if USE_DMA_SPEED_UP_RENDER_MIX
                        dma_start((uint32_t)&fill_fb[i*GUI_RES_X],(uint32_t)&disp_fb[(GUI_RES_Y-top_pix+i)*GUI_RES_X],
                            GUI_RES_X*GUI_BYTES_PER_PIX);
#else
                        for (j = 0; j < GUI_RES_X; j++)
                        {
                            disp_fb[(GUI_RES_Y-top_pix+i)*GUI_RES_X+j] = fill_fb[i*GUI_RES_X+j];
                        }
#endif
                    }
                }
                else //if(load_gwin==last_gwin)//反向
                {
                    //旧窗口向下移动
                    if(diff_pix<GUI_RES_Y)
                    {
                        for(i=0;i<top_pix;i++)
                        {
#if USE_DMA_SPEED_UP_RENDER_MIX
                            dma_start((uint32_t)&disp_fb[(GUI_RES_Y-1-diff_pix-i)*GUI_RES_X],(uint32_t)&disp_fb[(GUI_RES_Y-1-i)*GUI_RES_X],
                                GUI_RES_X*GUI_BYTES_PER_PIX);
#else
                            for (j = 0; j < GUI_RES_X; j++)
                            {
                                disp_fb[(GUI_RES_Y-1-i)*GUI_RES_X+j] = disp_fb[(GUI_RES_Y-1-diff_pix-i)*GUI_RES_X+j];
                            }
#endif
                        }
                    }

                    //新窗不移动（显示上半部分）
                    for(i=0;i<GUI_RES_Y-top_pix;i++)
                    {
#if USE_DMA_SPEED_UP_RENDER_MIX
                        dma_start((uint32_t)&fill_fb[i*GUI_RES_X],(uint32_t)&disp_fb[i*GUI_RES_X],
                            GUI_RES_X*GUI_BYTES_PER_PIX);
#else
                        for (j = 0; j < GUI_RES_X; j++)
                        {
                            disp_fb[i*GUI_RES_X+j] = fill_fb[i*GUI_RES_X+j];
                        }
#endif
                    }
                }

                break;
            }

            /* 从左->右 刷 */
            case GUI_EFFECT_CONVERT_L2R:
            {
                //TODO：实现
                break;
            }

            /* 从右->左刷 */
            case GUI_EFFECT_CONVERT_R2L:
            {
                if(load_gwin==gwin)//正向
                {
                    //旧窗口不移动

                    //新窗口向左移动（显示左半部分）
                    for(i=0;i<GUI_RES_Y;i++)
                    {
                        for (j = 0; j < top_pix ; j++)
                        {
                            disp_fb[i*GUI_RES_X+(GUI_RES_X-top_pix)+j] = fill_fb[i*GUI_RES_X+j];
                        }
                    }
                }
                else //if(load_gwin==last_gwin)//反向
                {
                    //旧窗口向右移动
                    if(diff_pix<GUI_RES_X)
                    {
                        for(i=0;i<GUI_RES_Y;i++)
                        {
                            for (j = 0; j < top_pix; j++)
                            {
                                disp_fb[i*GUI_RES_X+(GUI_RES_X-1-j)] = disp_fb[i*GUI_RES_X+(GUI_RES_X-1-j-diff_pix)];
                            }
                        }
                    }

                    //新窗不移动（显示左半部分）
                    for(i=0;i<GUI_RES_Y;i++)
                    {
                        for (j = 0; j < GUI_RES_X-top_pix; j++)
                        {
                            disp_fb[i*GUI_RES_X+j] = fill_fb[i*GUI_RES_X+j];
                        }
                    }
                }

                break;
            }

            /* 从下->上刷  的反向*/
            case GUI_EFFECT_CONVERT_D2U_REVERSE:
            {
                if(load_gwin==gwin)//正向
                {
                    //旧窗口向上移动
                    if(diff_pix<GUI_RES_Y)
                    {
                        for(i=0;i<GUI_RES_Y-top_pix;i++)
                        {
#if USE_DMA_SPEED_UP_RENDER_MIX
                            dma_start((uint32_t)&disp_fb[(GUI_RES_Y-1-i-diff_pix)*GUI_RES_X],(uint32_t)&disp_fb[(GUI_RES_Y-1-i)*GUI_RES_X],
                                GUI_RES_X*GUI_BYTES_PER_PIX);
#else
                            for (j = 0; j < GUI_RES_X; j++)
                            {
                                disp_fb[(GUI_RES_Y-1-i)*GUI_RES_X+j] = disp_fb[(GUI_RES_Y-1-i-diff_pix)*GUI_RES_X+j];
                            }
#endif
                        }
                    }

                    //新窗口不移动（显示上半部分）
                    for(i=0;i<top_pix;i++)
                    {
#if USE_DMA_SPEED_UP_RENDER_MIX
                        dma_start((uint32_t)&fill_fb[i*GUI_RES_X],(uint32_t)&disp_fb[i*GUI_RES_X],
                            GUI_RES_X*GUI_BYTES_PER_PIX);
#else
                        for (j = 0; j < GUI_RES_X; j++)
                        {
                            disp_fb[i*GUI_RES_X+j] = fill_fb[i*GUI_RES_X+j];
                        }
#endif
                    }
                }
                else //if(load_gwin==last_gwin)//反向
                {
                    //旧窗口不移动

                    //新窗向上移动（显示上半部分）
                    for(i=0;i<GUI_RES_Y-top_pix;i++)
                    {
#if USE_DMA_SPEED_UP_RENDER_MIX
                        dma_start((uint32_t)&fill_fb[i*GUI_RES_X],(uint32_t)&disp_fb[(top_pix+i)*GUI_RES_X],
                            GUI_RES_X*GUI_BYTES_PER_PIX);
#else
                        for (j = 0; j < GUI_RES_X; j++)
                        {
                            disp_fb[(top_pix+i)*GUI_RES_X+j] = fill_fb[i*GUI_RES_X+j];
                        }
#endif
                    }
                }
                break;
            }

            /* 从上->下刷  的反向 */
            case GUI_EFFECT_CONVERT_U2D_REVERSE:
            {
                if(load_gwin==gwin)//正向
                {
                    //旧窗口向上移动
                    if(diff_pix<GUI_RES_Y)
                    {
                        for(i=0;i<GUI_RES_Y-top_pix;i++)
                        {
#if USE_DMA_SPEED_UP_RENDER_MIX
                            dma_start((uint32_t)&disp_fb[(diff_pix+i)*GUI_RES_X],(uint32_t)&disp_fb[i*GUI_RES_X],
                                GUI_RES_X*GUI_BYTES_PER_PIX);
#else
                            for (j = 0; j < GUI_RES_X; j++)
                            {
                                disp_fb[i*GUI_RES_X+j] = disp_fb[(diff_pix+i)*GUI_RES_X+j];
                            }
#endif
                        }
                    }

                    //新窗口不移动（显示下半部分）
                    for(i=0;i<top_pix;i++)
                    {
#if USE_DMA_SPEED_UP_RENDER_MIX
                        dma_start((uint32_t)&fill_fb[(GUI_RES_Y-top_pix+i)*GUI_RES_X],(uint32_t)&disp_fb[(GUI_RES_Y-top_pix+i)*GUI_RES_X],
                            GUI_RES_X*GUI_BYTES_PER_PIX);
#else
                        for (j = 0; j < GUI_RES_X; j++)
                        {
                            disp_fb[(GUI_RES_Y-top_pix+i)*GUI_RES_X+j] = fill_fb[(GUI_RES_Y-top_pix+i)*GUI_RES_X+j];
                        }
#endif
                    }
                }
                else //if(load_gwin==last_gwin)//反向
                {
                    //旧窗口不移动

                    //新窗向下移动（显示下半部分）
                    for(i=0;i<GUI_RES_Y-top_pix;i++)
                    {
#if USE_DMA_SPEED_UP_RENDER_MIX
                        dma_start((uint32_t)&fill_fb[(top_pix+i)*GUI_RES_X],(uint32_t)&disp_fb[i*GUI_RES_X],
                            GUI_RES_X*GUI_BYTES_PER_PIX);
#else
                        for (j = 0; j < GUI_RES_X; j++)
                        {
                            disp_fb[i*GUI_RES_X+j] = fill_fb[(top_pix+i)*GUI_RES_X+j];
                        }
#endif
                    }
                }

                break;
            }

            /* 从右->左刷  的反向 */
            case GUI_EFFECT_CONVERT_R2L_REVERSE:
            {
                if(load_gwin==gwin)//正向
                {
                    //旧窗口向右移动
                    if(diff_pix<GUI_RES_X)
                    {
                        for(i=0;i<GUI_RES_Y;i++)
                        {
                            for (j = 0; j < GUI_RES_X-top_pix; j++)
                            {
                                disp_fb[i*GUI_RES_X+(GUI_RES_X-1-j)] = disp_fb[i*GUI_RES_X+(GUI_RES_X-1-j-diff_pix)];
                            }
                        }
                    }

                    //新窗不移动（显示左半部分）
                    for(i=0;i<GUI_RES_Y;i++)
                    {
                        for (j = 0; j < top_pix; j++)
                        {
                            disp_fb[i*GUI_RES_X+j] = fill_fb[i*GUI_RES_X+j];
                        }
                    }

                }
                else //if(load_gwin==last_gwin)//反向
                {
                    //旧窗口不移动

                    //新窗口向左移动（显示左半部分）
                    for(i=0;i<GUI_RES_Y;i++)
                    {
                        for (j = 0; j < GUI_RES_X-top_pix ; j++)
                        {
                            disp_fb[i*GUI_RES_X+top_pix+j] = fill_fb[i*GUI_RES_X+j];
                        }
                    }
                }

                break;
            }

            /* 从左->右刷  的反向 */
            case GUI_EFFECT_CONVERT_L2R_REVERSE:
            {
                //TODO：实现
                break;
            }

            default:
            {
                gui_unlock();
                return ;
            }
        }
    }


#else

    if(diff_pix>0)
    {
        //两个窗口组合
        switch((int)effect)
        {
            case GUI_EFFECT_NONE://直接显示 disp_fb
            {
                break;
            }

            /* 从左->右 滚动 */
            case GUI_EFFECT_ROLL_L2R:
            {
                if(load_gwin==gwin)//正向
                {
                    //旧窗口向右移动：整体向右移动offset_pix个像素
                    #if USE_DMA_SPEED_UP_RENDER_MIX
                    {
                        if(diff_pix<GUI_RES_X)
                        {
                            //用DMA加速传输 ,
                            uint16_t *new_disp_fb=(uint16_t *)(disp_fb-GUI_RES_X);

                            dma_start_k((uint32_t)&disp_fb[0],(uint32_t)&new_disp_fb[diff_pix],
                                (GUI_RES_X*GUI_RES_Y/2-diff_pix)*GUI_BYTES_PER_PIX,true,true,2);

                            dma_start_k((uint32_t)&disp_fb[GUI_RES_X*GUI_RES_Y/2*1],
                                (uint32_t)&new_disp_fb[GUI_RES_X*GUI_RES_Y/2*1+diff_pix],
                                (GUI_RES_X*GUI_RES_Y/2-diff_pix)*GUI_BYTES_PER_PIX,true,true,2);

                           for(i=0;i<GUI_RES_Y;i++)
                           {
                                dma_start((uint32_t)&new_disp_fb[(GUI_RES_Y-1-i)*GUI_RES_X],
                                    (uint32_t)&disp_fb[(GUI_RES_Y-1-i)*GUI_RES_X],
                                    GUI_RES_X*GUI_BYTES_PER_PIX);
                           }
                        }
                    }
                    #else
                    {
                        if(diff_pix<GUI_RES_X)
                        {
                            for(i=0;i<GUI_RES_Y;i++)
                            {
                                for (j = 0; j < GUI_RES_X-diff_pix; j++)
                                {
                                    disp_fb[i*GUI_RES_X+GUI_RES_X-1-j] = disp_fb[i*GUI_RES_X+GUI_RES_X-1-j-diff_pix];
                                }
                            }
                        }
                    }
                    #endif

                    //新窗口向右移：只读取offset_pix个像素
                    __gui_window_fb_fill(load_gwin,disp_fb,GUI_RES_X-top_pix,diff_pix,0,GUI_RES_Y);
                }
                else //if(load_gwin==last_gwin)//反向
                {
                    //旧窗口向左移动：整体向左移动offset_pix个像素
                    #if USE_DMA_SPEED_UP_RENDER_MIX
                    {
                        if(diff_pix<GUI_RES_X)
                        {
                            dma_start_k((uint32_t)&disp_fb[diff_pix+GUI_RES_X*GUI_RES_Y/2*0],
                                (uint32_t)&disp_fb[0+GUI_RES_X*GUI_RES_Y/2*0],
                                (GUI_RES_X*GUI_RES_Y/2-diff_pix)*GUI_BYTES_PER_PIX,true,true,2);

                            dma_start_k((uint32_t)&disp_fb[diff_pix+GUI_RES_X*GUI_RES_Y/2*1],
                                (uint32_t)&disp_fb[0+GUI_RES_X*GUI_RES_Y/2*1],
                                (GUI_RES_X*GUI_RES_Y/2-diff_pix)*GUI_BYTES_PER_PIX,true,true,2);
                        }
                    }
                    #else
                    {
                        if(diff_pix<GUI_RES_X)
                        {
                            for(i=0;i<GUI_RES_Y;i++)
                            {
                                for (j = 0; j < GUI_RES_X-diff_pix; j++)
                                {
                                    disp_fb[i*GUI_RES_X+j] = disp_fb[i*GUI_RES_X+j+diff_pix];
                                }
                            }
                        }
                    }
                    #endif

                    //新窗口向左移
                    __gui_window_fb_fill(load_gwin,&disp_fb[GUI_RES_X-diff_pix],
                        GUI_RES_X-top_pix-diff_pix,diff_pix,0,GUI_RES_Y);
                }

                break;
            }

            /* 从右->左 滚动 */
            case GUI_EFFECT_ROLL_R2L:
            {
                if(load_gwin==gwin)//正向
                {
                    //旧窗口向左移动
                    #if USE_DMA_SPEED_UP_RENDER_MIX
                    {
                        if(diff_pix<GUI_RES_X)
                        {
                            dma_start_k((uint32_t)&disp_fb[diff_pix+GUI_RES_X*GUI_RES_Y/2*0],
                                (uint32_t)&disp_fb[0+GUI_RES_X*GUI_RES_Y/2*0],
                                (GUI_RES_X*GUI_RES_Y/2-diff_pix)*GUI_BYTES_PER_PIX,true,true,2);

                            dma_start_k((uint32_t)&disp_fb[diff_pix+GUI_RES_X*GUI_RES_Y/2*1],
                                (uint32_t)&disp_fb[0+GUI_RES_X*GUI_RES_Y/2*1],
                                (GUI_RES_X*GUI_RES_Y/2-diff_pix)*GUI_BYTES_PER_PIX,true,true,2);
                        }
                    }
                    #else
                    {
                        if(diff_pix<GUI_RES_X)
                        {
                            for(i=0;i<GUI_RES_Y;i++)
                            {
                                for (j = 0; j < GUI_RES_X-diff_pix; j++)
                                {
                                    disp_fb[i*GUI_RES_X+j] = disp_fb[i*GUI_RES_X+j+diff_pix];
                                }
                            }
                        }
                    }
                    #endif

                    //新窗口向左移动
                    __gui_window_fb_fill(load_gwin,&disp_fb[GUI_RES_X-diff_pix],top_pix-diff_pix,diff_pix,0,GUI_RES_Y);
                }
                else //if(load_gwin==last_gwin)//反向
                {
                    //旧窗口向右移动
                    #if USE_DMA_SPEED_UP_RENDER_MIX
                    {
                        if(diff_pix<GUI_RES_X)
                        {
                            //用DMA加速传输 ,
                            uint16_t *new_disp_fb=(uint16_t *)(disp_fb-GUI_RES_X);

                            dma_start_k((uint32_t)&disp_fb[0],(uint32_t)&new_disp_fb[diff_pix],
                                (GUI_RES_X*GUI_RES_Y/2-diff_pix)*GUI_BYTES_PER_PIX,true,true,2);

                            dma_start_k((uint32_t)&disp_fb[GUI_RES_X*GUI_RES_Y/2*1],
                                (uint32_t)&new_disp_fb[GUI_RES_X*GUI_RES_Y/2*1+diff_pix],
                                (GUI_RES_X*GUI_RES_Y/2-diff_pix)*GUI_BYTES_PER_PIX,true,true,2);

                           for(i=0;i<GUI_RES_Y;i++)
                           {
                                dma_start((uint32_t)&new_disp_fb[(GUI_RES_Y-1-i)*GUI_RES_X],
                                    (uint32_t)&disp_fb[(GUI_RES_Y-1-i)*GUI_RES_X],
                                    GUI_RES_X*GUI_BYTES_PER_PIX);
                           }
                        }
                    }
                    #else
                    {
                        for(i=0;i<GUI_RES_Y;i++)
                        {
                            for (j = 0; j < GUI_RES_X-diff_pix; j++)
                            {
                                disp_fb[i*GUI_RES_X+GUI_RES_X-1-j] = disp_fb[i*GUI_RES_X+GUI_RES_X-1-j-diff_pix];
                            }
                        }
                    }
                    #endif

                    //新窗口向右移
                    __gui_window_fb_fill(load_gwin,disp_fb,top_pix,diff_pix,0,GUI_RES_Y);
                }

                break;
            }

            /* 从上->下刷 （下拉菜单）*/
            case GUI_EFFECT_CONVERT_U2D:
            {
                if(load_gwin==gwin)//正向
                {
                    //旧窗口不移动

                    //新窗口向下移动（显示下半部分）
                    #if USE_DMA_SPEED_UP_RENDER_MIX
                    {
                        for(i=0;i<top_pix-diff_pix;i++)
                        {
                            dma_start((uint32_t)&disp_fb[(top_pix-1-diff_pix-i)*GUI_RES_X],
                                (uint32_t)&disp_fb[(top_pix-1-i)*GUI_RES_X],
                                GUI_RES_X*GUI_BYTES_PER_PIX);
                        }
                    }
                    #else
                    {
                        for(i=0;i<top_pix-diff_pix;i++)
                        {
                            for (j = 0; j < GUI_RES_X; j++)
                            {
                                disp_fb[(top_pix-1-i)*GUI_RES_X+j] = disp_fb[(top_pix-1-diff_pix-i)*GUI_RES_X+j];
                            }
                        }
                    }
                    #endif

                    __gui_window_fb_fill(load_gwin,disp_fb,0,GUI_RES_X,GUI_RES_Y-top_pix,diff_pix);
                }
                else //if(load_gwin==last_gwin)//反向
                {
                    //旧窗口向上移动   TODO:需要修复 top_pix>360的情况，此时DMA传输长度太大
                    #if USE_DMA_SPEED_UP_RENDER_MIX
                    {
                        if(top_pix>0)
                        {
                            dma_start((uint32_t)&disp_fb[(diff_pix)*GUI_RES_X],(uint32_t)&disp_fb[0],
                                top_pix*GUI_RES_X*GUI_BYTES_PER_PIX);
                        }

                    }
                    #else
                    {
                        for(i=0;i<top_pix;i++)
                        {
                            for (j = 0; j < GUI_RES_X; j++)
                            {
                                disp_fb[i*GUI_RES_X+j] = disp_fb[(diff_pix+i)*GUI_RES_X+j];
                            }
                        }
                    }
                    #endif
                    //新窗不移动（显示下半部分）
                    __gui_window_fb_fill(load_gwin,&disp_fb[top_pix*GUI_RES_X],
                        0,GUI_RES_X,top_pix,diff_pix);

                }
                break;
            }

            /* 从上->下刷  的反向 （下拉菜单上滑）*/
            case GUI_EFFECT_CONVERT_U2D_REVERSE:
            {
                if(load_gwin==gwin)//正向
                {
                    //旧窗口向上移动   TODO:需要修复 GUI_RES_Y-top_pix>360的情况，此时DMA传输长度太大
                    #if USE_DMA_SPEED_UP_RENDER_MIX
                    {
                        if(GUI_RES_Y-top_pix>0)
                        {
                            dma_start((uint32_t)&disp_fb[(diff_pix)*GUI_RES_X],(uint32_t)&disp_fb[0],
                                (GUI_RES_Y-top_pix)*GUI_RES_X*GUI_BYTES_PER_PIX);
                        }

                    }
                    #else
                    {
                        for(i=0;i<(GUI_RES_Y-top_pix);i++)
                        {
                            for (j = 0; j < GUI_RES_X; j++)
                            {
                                disp_fb[i*GUI_RES_X+j] = disp_fb[(diff_pix+i)*GUI_RES_X+j];
                            }
                        }
                    }
                    #endif

                    //新窗不移动（显示下半部分）
                    __gui_window_fb_fill(load_gwin,&disp_fb[(GUI_RES_Y-top_pix)*GUI_RES_X],
                        0,GUI_RES_X,(GUI_RES_Y-top_pix),diff_pix);
                }
                else //if(load_gwin==last_gwin)//反向
                {
                    //旧窗口不移动

                    //新窗口向下移动（显示下半部分）
                    #if USE_DMA_SPEED_UP_RENDER_MIX
                    {
                        for(i=0;i<GUI_RES_Y-top_pix-diff_pix;i++)
                        {
                            dma_start((uint32_t)&disp_fb[(GUI_RES_Y-top_pix-diff_pix-1-i)*GUI_RES_X],
                                (uint32_t)&disp_fb[(GUI_RES_Y-top_pix-1-i)*GUI_RES_X],
                                GUI_RES_X*GUI_BYTES_PER_PIX);
                        }
                    }
                    #else
                    {
                        for(i=0;i<GUI_RES_Y-top_pix-diff_pix;i++)
                        {
                            for (j = 0; j < GUI_RES_X; j++)
                            {
                                disp_fb[(GUI_RES_Y-top_pix-1-i)*GUI_RES_X+j] =
                                    disp_fb[(GUI_RES_Y-top_pix-diff_pix-1-i)*GUI_RES_X+j];
                            }
                        }
                    }
                    #endif

                    __gui_window_fb_fill(load_gwin,&disp_fb[0],0,GUI_RES_X,top_pix,diff_pix);
                }

                break;
            }

            /* 从下->上刷  （上拉菜单）*/
            case GUI_EFFECT_CONVERT_D2U:
            {
                if(load_gwin==gwin)//正向
                {
                    //旧窗口不移动

                    //新窗口向上移动（显示上半部分）  TODO:需要修复 top_pix-diff_pix>360的情况，此时DMA传输长度太大
                    #if USE_DMA_SPEED_UP_RENDER_MIX
                    {
                        if(top_pix-diff_pix>0)
                        {
                            dma_start((uint32_t)&disp_fb[(GUI_RES_Y-top_pix+diff_pix)*GUI_RES_X],
                                (uint32_t)&disp_fb[(GUI_RES_Y-top_pix)*GUI_RES_X],
                                (top_pix-diff_pix)*GUI_RES_X*GUI_BYTES_PER_PIX);
                        }

                    }
                    #else
                    {
                        for(i=0;i<top_pix-diff_pix;i++)
                        {
                            for (j = 0; j < GUI_RES_X; j++)
                            {
                                disp_fb[(GUI_RES_Y-top_pix+i)*GUI_RES_X+j] = disp_fb[(GUI_RES_Y-top_pix+diff_pix+i)*GUI_RES_X+j];
                            }
                        }
                    }
                    #endif
                    __gui_window_fb_fill(load_gwin,&disp_fb[(GUI_RES_Y-diff_pix)*GUI_RES_X],
                            0,GUI_RES_X,top_pix-diff_pix,diff_pix);

                }
                else //if(load_gwin==last_gwin)//反向
                {
                    //旧窗口向下移动
                    #if USE_DMA_SPEED_UP_RENDER_MIX
                    {
                        for(i=0;i<top_pix;i++)
                        {
                            dma_start((uint32_t)&disp_fb[(GUI_RES_Y-1-diff_pix-i)*GUI_RES_X],
                                (uint32_t)&disp_fb[(GUI_RES_Y-1-i)*GUI_RES_X],
                                GUI_RES_X*GUI_BYTES_PER_PIX);
                        }
                    }
                    #else
                    {
                        if(diff_pix<GUI_RES_Y)
                        {
                            for(i=0;i<top_pix;i++)
                            {
                                for (j = 0; j < GUI_RES_X; j++)
                                {
                                    disp_fb[(GUI_RES_Y-1-i)*GUI_RES_X+j] = disp_fb[(GUI_RES_Y-1-diff_pix-i)*GUI_RES_X+j];
                                }
                            }
                        }
                    }
                    #endif

                    //新窗不移动
                    __gui_window_fb_fill(load_gwin,(uint16_t*)&disp_fb[(GUI_RES_Y-top_pix-diff_pix)*GUI_RES_X],
                            0,GUI_RES_X,GUI_RES_Y-top_pix-diff_pix,diff_pix);
                }

                break;
            }

            /* 从下->上刷  的反向 （上拉菜单下滑）*/
            case GUI_EFFECT_CONVERT_D2U_REVERSE:
            {
                if(load_gwin==gwin)//正向
                {
                    //旧窗口向下移动
                    #if USE_DMA_SPEED_UP_RENDER_MIX
                    {
                        for(i=0;i<GUI_RES_Y-top_pix;i++)
                        {
                            dma_start((uint32_t)&disp_fb[(GUI_RES_Y-1-diff_pix-i)*GUI_RES_X],
                                (uint32_t)&disp_fb[(GUI_RES_Y-1-i)*GUI_RES_X],
                                GUI_RES_X*GUI_BYTES_PER_PIX);
                        }
                    }
                    #else
                    {
                        if(diff_pix<GUI_RES_Y)
                        {
                            for(i=0;i<GUI_RES_Y-top_pix;i++)
                            {
                                for (j = 0; j < GUI_RES_X; j++)
                                {
                                    disp_fb[(GUI_RES_Y-1-i)*GUI_RES_X+j] = disp_fb[(GUI_RES_Y-1-i-diff_pix)*GUI_RES_X+j];
                                }
                            }
                        }
                    }
                    #endif

                    //新窗口不移动
                    __gui_window_fb_fill(load_gwin,(uint16_t*)&disp_fb[(top_pix-diff_pix)*GUI_RES_X],
                            0,GUI_RES_X,top_pix-diff_pix,diff_pix);
                }
                else //if(load_gwin==last_gwin)//反向
                {
                    //旧窗口不移动

                    //新窗向上移动    TODO:需要修复 GUI_RES_Y-top_pix-diff_pix>360的情况，此时DMA传输长度太大
                    #if USE_DMA_SPEED_UP_RENDER_MIX
                    {
                        if(GUI_RES_Y-top_pix-diff_pix>0)
                        {
                            dma_start((uint32_t)&disp_fb[(top_pix+diff_pix)*GUI_RES_X],
                                (uint32_t)&disp_fb[(top_pix)*GUI_RES_X],
                                (GUI_RES_Y-top_pix-diff_pix)*GUI_RES_X*GUI_BYTES_PER_PIX);
                        }

                    }
                    #else
                    {
                        for(i=0;i<GUI_RES_Y-top_pix-diff_pix;i++)
                        {
                            for (j = 0; j < GUI_RES_X; j++)
                            {
                                disp_fb[(top_pix+i)*GUI_RES_X+j] = disp_fb[(top_pix+diff_pix+i)*GUI_RES_X+j];
                            }
                        }
                    }
                    #endif
                    __gui_window_fb_fill(load_gwin,&disp_fb[(GUI_RES_Y-diff_pix)*GUI_RES_X],
                            0,GUI_RES_X,GUI_RES_Y-top_pix-diff_pix,diff_pix);
                }
                break;
            }

            /* 从右->左刷  （右滑进入）*/
            case GUI_EFFECT_CONVERT_R2L:
            {
                break;
            }

            /* 从右->左刷  的反向 （右滑退出）*/
            case GUI_EFFECT_CONVERT_R2L_REVERSE:
            {
                if(load_gwin==gwin)//正向
                {
                    //旧窗口向右移动
                    if(diff_pix<GUI_RES_X)
                    {
                        for(i=0;i<GUI_RES_Y;i++)
                        {
                            for (j = 0; j < GUI_RES_X-top_pix; j++)
                            {
                                disp_fb[i*GUI_RES_X+(GUI_RES_X-1-j)] = disp_fb[i*GUI_RES_X+(GUI_RES_X-1-j-diff_pix)];
                            }
                        }
                    }

                    //新窗不移动（显示左半部分）

                    __gui_window_fb_fill(load_gwin,&disp_fb[top_pix-diff_pix],
                            top_pix-diff_pix,diff_pix,0,GUI_RES_Y);

                }
                else //if(load_gwin==last_gwin)//反向
                {
                    //旧窗口不移动

                    //新窗口向左移动（显示左半部分）
                    if(GUI_RES_X-top_pix-diff_pix>0)
                    {
                        for(i=0;i<GUI_RES_Y;i++)
                        {
                            for (j = 0; j < GUI_RES_X-top_pix-diff_pix ; j++)
                            {
                                disp_fb[i*GUI_RES_X+top_pix+j] = disp_fb[i*GUI_RES_X+top_pix+diff_pix+j];
                            }
                        }
                    }
                    __gui_window_fb_fill(load_gwin,&disp_fb[GUI_RES_X-diff_pix],
                        GUI_RES_X-top_pix-diff_pix,diff_pix,0,GUI_RES_Y);
                }

                break;
            }

            /* 从左->右 刷 */
            case GUI_EFFECT_CONVERT_L2R:
            {
                //TODO：实现
                break;
            }

            /* 从左->右刷  的反向 */
            case GUI_EFFECT_CONVERT_L2R_REVERSE:
            {
                //TODO：实现
                break;
            }

            default:
            {
                gui_unlock();
                return ;
            }
        }
    }

#endif

    last_top_pix=top_pix;

#if MEASURE_RENDER_MIX_TIME
    t3=OS_GET_TICK_COUNT();
#endif

    gui_flush(disp_fb,0,0,GUI_RES_X,GUI_RES_Y,false);


#if MEASURE_RENDER_MIX_TIME
    t4=OS_GET_TICK_COUNT();
#endif

//    OS_INFO("pix=%d,off=%d,win=%x ",top_pix,diff_pix,(uint32_t)load_gwin);

    //耗时50-90ms，由窗口内容复杂度决定
#if MEASURE_RENDER_MIX_TIME
    OS_LOG("fi=%d ms,cpy=%d ms,drw=%d ms,to=%d ms\r\n",
            OS_TICKS_2_MS(t2-t1),
            OS_TICKS_2_MS(t3-t2),
            OS_TICKS_2_MS(t4-t3),
            OS_TICKS_2_MS(t4-t1));
#endif

    gui_unlock();

}

void gui_window_show_mix(uint16_t top_pix,gui_window_effect_t effect)
{
    if(gui_display_is_close())
        return ;

    /*
     * 分屏情况下暂时不能支持这种方式刷屏，速度太慢
     */
#if GUI_BUF_NEED_DIV
    return ;
#endif

    gui_window_render_mix(top_pix,effect);
}



