/*
 * gui_window.c
 *
 *  Created on: 2019��7��11��
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

//������ջ
//���أ�0 �ɹ���-1 ʧ��
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

//���ڳ�ջ
//���أ�hdl �ɹ���0 ʧ��
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

//ȡ��ջ���ڶ�λ����
//���أ�hdl �ɹ���0 ʧ��
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

//��ȡջ������
//���أ�hdl �ɹ���0 ʧ��
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

//��ȡջ������
//���أ�hdl �ɹ���0 ʧ��
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

//���Դ�ӡ���д���
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
@brief : ��������

@param :
-x   ������x��ʾƫ��
-y   ������y��ʾƫ��
-w   ���ڿ�
-h   ���ڸ�

@retval:
- hdl   �ɹ�
- 0     ʧ��
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

    //������ GUI_RES_X ������
//    gwin->w=(gwin->w+GUI_RES_X-1)/GUI_RES_X*GUI_RES_X;
//    gwin->h=(gwin->h+GUI_RES_Y-1)/GUI_RES_Y*GUI_RES_Y;

    //���ڲ��ܱ�LCDС
    if(gwin->w<GUI_RES_X)
        gwin->w=GUI_RES_X;
    if(gwin->h<GUI_RES_Y)
        gwin->h=GUI_RES_Y;

    //��ʾ���ݶ��ڴ�����
//    if(gwin->x + GUI_RES_X > gwin->w)
//        gwin->x=gwin->w-GUI_RES_X;
//    if(gwin->y + GUI_RES_Y > gwin->h)
//        gwin->y=gwin->h-GUI_RES_Y;

    if(gwin->x >= gwin->w)
        gwin->x=gwin->w-1;
    if(gwin->y >= gwin->h)
        gwin->y=gwin->h-1;

    gwin->bg_color=INVALID_COLOR;/* Ĭ�ϲ���䱳����ɫ���Ա�ӿ촰��ˢ��ʱ�� */
    gwin->bg_bmp=NULL;
    gwin->scroll=GUI_DISABLE_SCROLL;
    gwin->effect=GUI_EFFECT_NONE;
    gwin->layer_cnt=0;
    gwin->event_cnt=0;
    gwin->layers=NULL;
    gwin->events=NULL;

    gwin->loaded=false;

    //�ŵ�GUI����ջ
    if(gui_window_push((gui_window_hdl_t*)gwin))
    {
        OS_FREE(gwin);
        return 0;
    }

    GUI_WID_LOG("create success,addr=%08x  ,window num=%d\r\n",(uint32_t)gwin,m_gui_window_cnt);

    //���ԣ��۲촰���ڴ��Ƿ��ͷ�
    if(m_gui_window_cnt>2)
    {
        OS_LOG("create win addr=%08x ,window num=%d, last window not free !!!\r\n",(uint32_t)gwin,m_gui_window_cnt);
    }


    return (gui_window_hdl_t*)gwin;
}

/**
@brief : �������ڣ���С������Ļ�Զ���ֵ���

@param :
-x   ������x��ʾƫ��
-y   ������y��ʾƫ��

@retval:
- hdl   �ɹ�
- 0     ʧ��
*/
gui_window_hdl_t gui_window_create_maximum(uint16_t x,uint16_t y)
{
    return gui_window_create(x,y,GUI_RES_X,GUI_RES_Y);
}

/**
@brief : ���ô�����ʾ��ʼλ�ã�ƫ�ƣ�

@param :
-hdl ���ھ��
-x   ������x��ʾƫ��
-y   ������y��ʾƫ��

@retval:
- 0     �ɹ�
- -1    ʧ��
*/
int gui_window_set_position(gui_window_hdl_t hdl,uint16_t x,uint16_t y)
{
    if(!hdl)
        return -1;
    gui_window_t *gwin=(gui_window_t *)hdl;

    gui_lock();
    gwin->x = x;
    gwin->y = y;

    //���ڲ��ܱ�LCDС
//    if(gwin->w<GUI_RES_X)
//        gwin->w=GUI_RES_X;
//    if(gwin->h<GUI_RES_Y)
//        gwin->h=GUI_RES_Y;

    //��ʾ���ݶ��ڴ�����
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

//��ȡ��ʾ��yƫ��
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
@brief : ���ô��ڴ�С

@param :
-hdl ���ھ��
-w   ���ڿ�
-h   ���ڸ�

@retval:
- 0 �ɹ�
- -1 ʧ��
*/
int gui_window_set_size(gui_window_hdl_t hdl,uint16_t w,uint16_t h)
{
    if(!hdl || w==0 || h==0)
        return -1;
    gui_window_t *gwin=(gui_window_t *)hdl;

    gui_lock();
    gwin->w = w;
    gwin->h = h;

    //������ GUI_RES_X ������
//    gwin->w=(gwin->w+GUI_RES_X-1)/GUI_RES_X*GUI_RES_X;
//    gwin->h=(gwin->h+GUI_RES_Y-1)/GUI_RES_Y*GUI_RES_Y;

    //���ڲ��ܱ�LCDС
    if(gwin->w<GUI_RES_X)
        gwin->w=GUI_RES_X;
    if(gwin->h<GUI_RES_Y)
        gwin->h=GUI_RES_Y;

    //��ʾ���ݶ��ڴ�����
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
@brief : ���ù�������

@param :
-hdl      ���ھ��
-scroll   ���ڹ�������ref@gui_window_scroll_t

@retval:
- 0 �ɹ�
- -1 ʧ��
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
@brief : ���ñ���ͼƬ��ϵͳ�ڲ�ʹ�ã�

@param :
-hdl      ���ھ��
-res_id   ͼƬ��Դ��ID
-app_id   ͼƬ��Դ����APP

@retval:
- 0 �ɹ�
- -1 ʧ��
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
@brief : ���ñ���ͼƬ
                        ע�⣺�ñ���ͼ��ֻ���ǵ�ǰ����������APP����Դ����ʱ��������Ϊ����APP����Դ��

@param :
-hdl      ���ھ��
-res_id   ͼƬ��Դ��ID

@retval:
- 0 �ɹ�
- -1 ʧ��
*/
int gui_window_set_bg_bmp(gui_window_hdl_t hdl,uint16_t  res_id)
{
    uint16_t app_id;
    app_id=os_app_get_runing();

    return gui_window_set_bg_bmp_inner(hdl,res_id,app_id);
}

/**
@brief : ���ñ�����ɫ

@param :
-hdl      ���ھ��
-bg_color ������ɫֵ��ֻ֧��RGB565��ɫ

@retval:
- 0 �ɹ�
- -1 ʧ��
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
@brief : ���ö���Ч������δʹ��--����δʵ�֣�

@param :
-hdl      ���ھ��
-effect   ���ڼ���ʱ��Ч��

@retval:
- 0 �ɹ�
- -1 ʧ��
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
@brief : ���ô��ڼ������

@param :
-hdl    ���ھ����  ע�⣺���hdl==NULL����ô������ջ���Ĵ���

@retval:
- 0 �ɹ�
- -1 ʧ��
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
@brief : ��ȡ�����Ƿ�������

@param :
-hdl    ���ھ����  ע�⣺���hdl==NULL����ô����ȡջ���Ĵ���

@retval:
- 0 ����δ���
- -1 �������
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
@brief : ɾ�����ڣ��ͷ��ڴ�

@param :
-hdl    ���ھ��

@retval:
- 0 �ɹ�
- -1 ʧ��
*/
static int __gui_window_destroy(gui_window_hdl_t hdl)
{
    if(!hdl)
        return -1;
    gui_window_t *gwin=(gui_window_t *)hdl;
    int i;

//    gui_lock();

    //�����¼�
    if(gwin->event_cnt>0)
    {
        for(i=0;i<gwin->event_cnt;i++)
        {
            OS_FREE(gwin->events[i]);
        }
        OS_FREE(gwin->events);
    }

    //����ͼ��:ͼ���������¼�����ͼ������
    if(gwin->layer_cnt>0)
    {
        for(i=0;i<gwin->layer_cnt;i++)
        {
            gui_layer_destroy(gwin->layers[i]);
        }
        OS_FREE(gwin->layers);
    }

    //���ڱ���
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
@brief : ɾ��ջ�����ڣ��ͷ��ڴ�

@param :
-

@retval:
- 0 �ɹ�
- -1 ʧ��
*/
int gui_window_destroy_current()
{
    gui_window_hdl_t hdl=gui_window_pop();
    return __gui_window_destroy(hdl);
}

/**
@brief : ɾ��ջ����2�����ڣ��ͷ��ڴ�

@param :
-

@retval:
- 0 �ɹ�
- -1 ʧ��
*/
int gui_window_destroy_last()
{
    //ɾ����ջ�������д���
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
@brief : ���ͼ��

@param :
-hdl    ���ھ��
-layer  ͼ����

@retval:
- 0 �ɹ�
- -1 ʧ��
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
        OS_FREE(layer);//TODO:�˴����ܲ�Ӧ���ͷſռ�
        gui_unlock();
        return -1;
    }

    if (gwin->layer_cnt>0)
    {
        memcpy(new_layer, gwin->layers, sizeof(gui_layer_hdl_t)*(gwin->layer_cnt));
        OS_FREE(gwin->layers);
    }

    //˳�����
    new_layer[gwin->layer_cnt] = layer;

    gwin->layer_cnt++;
    gwin->layers=new_layer;

    gui_unlock();
    return 0;
}

/**
@brief : ��Ӵ����¼�

@param :
-hdl    ���ھ��
-event  �����¼�����

@retval:
- 0 �ɹ�
- -1 ʧ��
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
@brief : ���Ĵ����¼�

@param :
-hdl        ���ھ��
-evt_type   �����¼����ͣ�ref@gui_event_type_t
-cb         �¼��ص���������ʽref@gui_event_call_back_t

@retval:
- 0 �ɹ�
- -1 ʧ��
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
@brief : �����¼��ص�����

@param :
-hdl        ���ھ��
-evt_type   �����¼����ͣ�ref@gui_event_type_t
-pdata      �¼������ݣ����ݵĸ�ʽΪref@ gui_evt_tp_data_t
-size       �¼������ݴ�С���ֽڣ�

@retval:
- 0 �ɹ�
- -1 ʧ��
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


    //�����¼�
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

    //ͼ���¼�
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

            //�̶�ͼ�㣬����Ϊ��ǰLCD���꣬���Ǵ���ƫ�ƺ������
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

            //�����ǰ������tp������ͼ�����귶Χ�ڣ��ϱ��¼�
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

//һ�����ݵĻ���
static uint16_t one_line_buf[GUI_RES_X];

typedef struct{
    uint16_t disp_w,disp_h;         //���ڶ�ȡ��ʵ�ʴ�С������ֵ��
    uint16_t start_x,start_y;       //���ڵĶ�ȡ��ʼxy���꣨����ֵ����ʾ�����ڴ����ڵ����꣩
    uint16_t offset_x,offset_y;     //ͼ����fb�ڵ�ƫ�ƣ����ֵ����ʾ����ͼ����ʾ��ʼ����--��layer_start_x/y ���start_x/y�Ĳ�ֵ��
}draw_win_range_t;

typedef struct{
    uint16_t layer_start_x,layer_start_y,   //ʵ����ʾ��ͼ�㷶Χ������ֵ����ʾ�����ڴ����ڵ����꣩
             layer_end_x,layer_end_y;
    uint16_t layer_w,layer_h;               //ͼ���ߣ�����ֵ��
}draw_layer_range_t;

typedef struct{
    uint16_t read_w,read_h,             //ʵ�ʶ�ȡ��ͼ��Ŀ�ߣ�����ֵ��
        read_offset_x,read_offset_y;    //ʵ�ʶ�ȡ��ͼ���ڵ�xyƫ�ƣ����ֵ�������layer_start_x/y�Ĳ�ֵ��
}draw_pix_range_t;

static inline int __fill_layer(gui_window_t *gwin,uint16_t *fill_fb,draw_win_range_t *win_rg);

/**
@brief : ���ش������ݵ�fb

@param :
-hdl       ���ھ��
-fill_fb   ������������fb
-win_off_x  ���صĴ��ڸ���ƫ�ƣ����մ���ƫ��Ϊ gwin->x+offset_x             ,ע�⣺0<= win_off_x <GUI_RES_X
-win_load_w ���صĴ���ʵ�ʿ����մ���Ϊ w=load_w>gwin->w?gwin->w:load_w  ,ע�⣺0<= win_load_w <GUI_RES_X
-win_off_y  ���صĴ��ڸ���ƫ�ƣ����մ���ƫ��Ϊ gwin->y+offset_y             ,ע�⣺0<= win_off_y <GUI_RES_Y
-win_load_h ���صĴ���ʵ�ʸߣ����մ���Ϊ h=load_h>gwin->h?gwin->h:load_h  ,ע�⣺0<= win_load_h <GUI_RES_Y

@retval:
- 0 �ɹ�
- -1 ʧ��
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

    //�������ʲô��û�У���ôֱ�ӷ���
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
     * ���ݹ������ԣ�ȷ����ʾ�Ĵ��ڷ�Χ
     */
    switch(gwin->scroll)
    {
        /*
         * ���û��ʹ�ܹ�������ô��0���꿪ʼ��ʾ
         * ��� win_rg.start_x>= GUI_RES_X����ô����Ҫ���ش�������
         * ��� win_rg.start_x+win_rg.disp_w >= GUI_RES_X����ô���ڼ��ؿ�Ϊ win_rg.disp_w=GUI_RES_X-win_rg.start_x
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
     * ��ȡ����
     */
#if 1
    uint16_t i,j;
    if(gwin->bg_bmp)
    {
        /*
         * ����ͼƬ�����Ͻǵ�(0,0)��ʼ��ʾ
         * ����䲻����ʾ�����ڣ�����Ҫ���
         *
         * TODO:Ŀǰ��֧�ֱ�����������Ҫ����
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
        *  ʵ��240*240�ı���ͼƬ����ȡ������Ҫ20ms��
        *
        *TODO:
        *  ��������һ��һ�л�ȡ��Ч�ʵͣ��ǵ���ˢ������ԭ�򣬿��Ż���
        *
        */

        for(i=0;i<bg_bmp_h;i++)
        {
           gui_res_get_data(gwin->bg_bmp,
                   (gwin->bg_bmp->w*(i+bg_bmp_y)+bg_bmp_x)*GUI_BYTES_PER_PIX,
                   bg_bmp_w*GUI_BYTES_PER_PIX,
                   (uint8_t*)&fill_fb[i*GUI_RES_X] //������Ǵ�(0,0)λ�ÿ�ʼ
               );
        }

        /*
        * �ѱ���ͼ��δ���ǵ��������ñ�����ɫ���
        * ��Ҫ������ұߺ͵ױ�
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
          * ʵ��240*240�Ĵ��ڣ������ɫ��Ҫ9ms��  ����DMA����2�ֽڿ� ,��ʱΪ 1 ms,4�ֽڿ�0 ms
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
     *  ����ͼ������
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

//����ͼ����Ч��Χ������ͼ���Ӧ���ڵ�ƫ��
static inline int __get_range(
    draw_win_range_t *win_rg,
    draw_layer_range_t *lyr_rg,
    draw_pix_range_t *pix_rg
    )
{
    // ͼ�㿪ʼλ���� ��ʾ�����ڣ�4�����������ͼ���ڴ����ڣ�ͼ����벿���ڴ����ڣ�ͼ���ϰ벿���ڴ����ڣ�ͼ�����ϲ����ڴ����ڣ�
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
    // ͼ�����λ���� ��ʾ������
    //��3�������ͼ�������ڴ����ڣ�ͼ���Ұ벿���ڴ����ڣ�ͼ���°벿���ڴ����ڣ�
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
    // ͼ�����λ���� ��ʾ�����⣬��ʼλ��Ҳ�ڴ�����
    //��2�������ͼ�����ϲ����ڴ����ڣ�ͼ�����²����ڴ����ڣ�
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
     * ͼ��w��h���ڴ��ڣ���Խ���ڵ������
     * ��7���������
     *
     */
    //1 ͼ��x��ʼλ�úͽ���λ�ö��ڴ����ڣ�ͼ��y��ʼλ�úͽ���λ�ö��ڴ�����
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
    //2 ͼ��x����λ���ڴ����ڣ�ͼ��y��ʼλ�úͽ���λ�ö��ڴ�����
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
    //3 ͼ��x��ʼλ���ڴ����ڣ�ͼ��y��ʼλ�úͽ���λ�ö��ڴ�����
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
    //4 ͼ��y��ʼλ�úͽ���λ�ö��ڴ����ڣ�ͼ��x��ʼλ�úͽ���λ�ö��ڴ�����
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
    //5 ͼ��y����λ���ڴ����ڣ�ͼ��x��ʼλ�úͽ���λ�ö��ڴ�����
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
    //6 ͼ��x����λ���ڴ����ڣ�ͼ��x��ʼλ�úͽ���λ�ö��ڴ�����
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
    //7 ͼ��x��y��ʼλ�úͽ���λ�ö��ڴ�����--ͼ�������ʾ����
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
    //TODO:�޸��������
    else
    {
        pix_rg->read_w=pix_rg->read_h=0;
        win_rg->offset_x=win_rg->offset_y=0;

        return -1;
    }
    return 0;
}

//��layer �����ݼ��ص�fb
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
            lyr_rg.layer_start_x=layer->x%GUI_RES_X+gwin->x;//ȡ�봰�ڶ�����ƫ��
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
            /* ͼ��ͼ�� */
            case LAYER_TYPE_BITMAP:
            {
                gui_bmp_layer_attr_t *bmp_attr=(gui_bmp_layer_attr_t *)layer->attr;

                //ͼƬ��ͼ���ֻ��ʾͼ���С�����ݣ�������ͼƬ��СΪ׼
                if(lyr_rg.layer_w>bmp_attr->w)
                    lyr_rg.layer_w=bmp_attr->w;
                if(lyr_rg.layer_h>bmp_attr->h)
                    lyr_rg.layer_h=bmp_attr->h;

                lyr_rg.layer_end_x=lyr_rg.layer_start_x+lyr_rg.layer_w-1;
                lyr_rg.layer_end_y=lyr_rg.layer_start_y+lyr_rg.layer_h-1;

                //ͼ����ת
                if(layer->angle==0
                    //|| gwin->loaded==false //���л�����ʱ������תͼ�񣬼ӿ촦���ٶ�
                )
                {
                    if(__get_range(win_rg,&lyr_rg,&pix_rg))
                    {
                        break;
                    }

                    //���FB
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
                                       //͸������ɫ����ʾ
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

                    //TODO:�е�ͼ���ص�����Ҫÿ��ͼ�����ݶ���ȡ��ֻ��ȡ��ǰ���ͼ����ʾ�������ǵ�ͼ�㲻Ҫ��ȡ�Խ�ʡʱ��

                    break;
                }

#if MEASURE_FILL_TIME
                uint32_t t1,t2;
                t1=OS_GET_TICK_COUNT();
#endif

                pix_rg.read_w=lyr_rg.layer_w;
                pix_rg.read_h=lyr_rg.layer_h;

                //TODO������������ͼ���СС��GUI_RES_X*GUI_RES_Y�����������Ҫ���޸�
                if(pix_rg.read_w>GUI_RES_X)
                    pix_rg.read_w=GUI_RES_X;
                if(pix_rg.read_h>GUI_RES_Y)
                    pix_rg.read_h=GUI_RES_Y;

                /*
                 * ͼ����ת: ʹ�����Ǻ�����ͼ������һ����ת��
                 * TODO��
                 *      1 ����ÿ�����ص㶼Ҫ�������꣬�ȽϺ�ʱ����Ҫ�Ż�
                 *      2 ����ͼ����ת��������Σ����־�ݣ�������ͼ����תֻ������ת90�ȵı��� �� 45�ȶԳ�
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
                                                             ��תͼ��������
                                                                ��Ļ�����ż���Ŀ�ߣ���ô��ת���ı���ȡ4���㣬��ת֮���ͼ����ܺ�ԭ���غϡ�
                                                                ���ֻȡһ���㣬����ƫ�����Ϊ��ȡ��ת����Ϊ4�����е����½ǵĵ㣩��
                                                                    ��1���ޣ�ͼ����ת��ͼ��ƫ��
                                                                    ��2���ޣ�ͼ����ת90��ͼ��ƫ��x+1��������ת����x-1
                                                                    ��3���ޣ�ͼ����ת180��ͼ��ƫ�� x+1,y+1����ת����x-1,y-1
                                                                    ��4���ޣ�ͼ����ת270��ͼ��ƫ�� y+1����ת����y-1
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
                                  //͸������ɫ����ʾ
                                   if(pix_data[k]!=layer->trans_color)
                                   {
                                       x=lyr_rg.layer_start_x+k;
                                       y=lyr_rg.layer_start_y+j;

                                       /*
                                        * ��Χ����ת��ԭ��Ϊ(X0,Y0)����Ҫ��ת�ĵ�Ϊ(X,Y),��ת��ĵ�Ϊ(Xt,Yt)����ת�Ƕ�Ϊa
                                        * ��ת��ʽ��
                                        *   Xt= (X - X0)*cos(a) - (Y - Y0)*sin(a) + X0
                                        *   Yt= (X - X0)*sin(a) + (Y - Y0)*cos(a) + Y0
                                        */

                                       x1=(x - x0)*cos_val- (y - y0)*sin_val + x0 +0.5f;
                                       y1=(x - x0)*sin_val + (y - y0)*cos_val + y0 +0.5f;

                                       //ͼ������
                                       x1+=fix_x;
                                       y1+=fix_y;

                                       GUI_WID_LOG("xy={%d,%d} ==> xy={%d,%d}\r\n",(int16_t)x,(int16_t)y,(int16_t)x1,(int16_t)y1);

                                       if(x1>=win_rg->start_x && x1<win_rg->start_x+win_rg->disp_w
                                           && y1>=win_rg->start_y && y1<win_rg->start_y+win_rg->disp_h)
                                       {
                                           //ʵ���ϣ����ܴ������ƫ�ƣ�������䵽fb�Ķ��Ǵ�(0,0)��ʼ����������Ҫ��ȥ���ڵ���ʼֵ���õ���(0,0)��ƫ��
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
             * ����ͼ��
             *
             * TODO:����ͼ���������� w h ���ԣ���������layer�� w h ����
             */
            case LAYER_TYPE_PIXEL:
            {
                //������תʱͼ������ٶȹ������������л�����ʱ��Ҫ����
//                if(gwin->loaded==false)
//                {
//                    break;
//                }

                gui_pixel_layer_attr_t *pix_attr=(gui_pixel_layer_attr_t *)layer->attr;

//                GUI_WID_LOG("layer[%d] read_w=%d,pix_rg.read_h=%d,x=%d,y=%d,angle=%d,pix_type=%x,xy={%d,%d}\r\n",
//                    i,layer->w,layer->h,layer->x,layer->y,pix_attr->angle,pix_attr->type,
//                    pix_attr->rotate_center_x,pix_attr->rotate_center_y);

                //���� Ŀǰֻ֧��RGB565 rgb1(�ڰ�)
                if(pix_attr->type!=GUI_PIX_TYPE_RGB565 && pix_attr->type!=GUI_PIX_TYPE_RGB1 )
                    continue;

                //ͼ����ת
                if(layer->angle==0
                    //||gwin->loaded==false //���л�����ʱ������תͼ�񣬼ӿ촦���ٶ�
                )
                {
                    if(__get_range(win_rg,&lyr_rg,&pix_rg))
                    {
                        break;
                    }

                    //���FB
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
                                        //͸������ɫ����ʾ
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

                //���� Ŀǰֻ֧��RGB565
                if(pix_attr->type!=GUI_PIX_TYPE_RGB565)
                    continue;

#if MEASURE_FILL_TIME
                uint32_t t1,t2;
                t1=OS_GET_TICK_COUNT();
#endif

                /*
                 * ͼ����ת: ʹ�����Ǻ�����ͼ������һ����ת��
                 * TODO��
                 *      1 ����ÿ�����ص㶼Ҫ�������꣬�ȽϺ�ʱ����Ҫ�Ż�
                 *      2 ����ͼ����ת��������Σ����־�ݣ�������ͼ����תֻ������ת90�ȵı��� �� 45�ȶԳ�
                 *
                 * TODO������Ĵ����������ظ��ˣ����Ǻϲ���
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
                                                             ��תͼ��������
                                                                ��Ļ�����ż���Ŀ�ߣ���ô��ת���ı���ȡ4���㣬��ת֮���ͼ����ܺ�ԭ���غϡ�
                                                                ���ֻȡһ���㣬����ƫ�����Ϊ��ȡ��ת����Ϊ4�����е����½ǵĵ㣩��
                                                                    ��1���ޣ�ͼ����ת��ͼ��ƫ��
                                                                    ��2���ޣ�ͼ����ת90��ͼ��ƫ��x+1��������ת����x-1
                                                                    ��3���ޣ�ͼ����ת180��ͼ��ƫ�� x+1,y+1����ת����x-1,y-1
                                                                    ��4���ޣ�ͼ����ת270��ͼ��ƫ�� y+1����ת����y-1
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
                                  //͸������ɫ����ʾ
                                   if(pix_data[layer->w*(j)+k]!=layer->trans_color)
                                   {
                                       x=lyr_rg.layer_start_x+k;
                                       y=lyr_rg.layer_start_y+j;

                                       /*
                                        * ��Χ����ת��ԭ��Ϊ(X0,Y0)����Ҫ��ת�ĵ�Ϊ(X,Y),��ת��ĵ�Ϊ(Xt,Yt)����ת�Ƕ�Ϊa
                                        * ��ת��ʽ��
                                        *   Xt= (X - X0)*cos(a) - (Y - Y0)*sin(a) + X0
                                        *   Yt= (X - X0)*sin(a) + (Y - Y0)*cos(a) + Y0
                                        */

                                       x1=(x - x0)*cos_val- (y - y0)*sin_val + x0 +0.5f;
                                       y1=(x - x0)*sin_val + (y - y0)*cos_val + y0 +0.5f;

                                       //ͼ������
                                       x1+=fix_x;
                                       y1+=fix_y;

                                       GUI_WID_LOG("xy={%d,%d} ==> xy={%d,%d}\r\n",(int16_t)x,(int16_t)y,(int16_t)x1,(int16_t)y1);

                                       /*if(x1>=0 && x1<GUI_RES_X && y1>=0 && y1<GUI_RES_Y)//�������ʾ�ڴ���0ƫ�Ƶģ�������
                                       {
                                           fill_fb[(int16_t)y1*GUI_RES_X+(int16_t)x1]=pix_data[layer->w*(j)+k];
                                       }*/
                                       if(x1>=win_rg->start_x && x1<win_rg->start_x+win_rg->disp_w
                                           && y1>=win_rg->start_y && y1<win_rg->start_y+win_rg->disp_h)
                                       {
                                           //ʵ���ϣ����ܴ������ƫ�ƣ�������䵽fb�Ķ��Ǵ�(0,0)��ʼ����������Ҫ��ȥ���ڵ���ʼֵ���õ���(0,0)��ƫ��
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

            /* �ı�ͼ�� */
            case LAYER_TYPE_TEXT:
            {
                gui_text_layer_attr_t *text_attr=(gui_text_layer_attr_t *)layer->attr;
                uint16_t font_h=0,bytes=0;

#if MEASURE_FILL_TIME
                uint32_t t1,t2;
                t1=OS_GET_TICK_COUNT();
#endif
                //ȡ����һ������ĸ߶ȣ�ȷ���и߶���; ȡ���������ռ�ֽڣ�����ռ�
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

                         uint16_t t_w, t_h,                  //ʵ����ʾ�����ֿ��
                                  t_offset_x,t_offset_y,     //ʵ�ʶ�ȡ������xyƫ�� --�൱��ͼ��ƫ��
                                  lyr_offset_x,lyr_offset_y; //������ͼ���ڵ�ƫ��  --�൱�ڴ���ƫ��

                         uint16_t text_code;

                         font_data_read_t font_data_read;

                        for (j = 0; j < text_attr->text_len; j++)
                        {
                            text_code=text_attr->text_code[j];
                            if (IS_ASCII(text_code))
                            {
                                //�������ַ�תΪ�ո�
                                if (text_code < FONT_ASCII_MIN)
                                {
                                    text_code = FONT_ASCII_MIN;
                                }
                                font=text_attr->fonts[CHARACTER_SET_ASCII];
                                font_data_read=font_ascii_data;
                            }

                            //����
                            else
                            {
                                font=text_attr->fonts[CHARACTER_SET_GB2312];
                                font_data_read=font_gb2312_data;
                            }

                            //�������Ŀ����ͼ�����ô����ʾ��������ʾ������֣�ֻ��������ʾ�����������֣�
                            if ( font== NULL || font->font_attr.width > lyr_rg.layer_w)
                            {
                                continue;            //TODO:����ѭ��
                            }

                            if (text_x + font->font_attr.width > lyr_rg.layer_w)
                            {
                                text_x = 0;
                                text_y += font_h;
                            }

                            //����������ͼ���ڵ�λ��
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
                                break;//����ѭ��
                            }

                            //��������ƫ��
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

            /* ����ͼ�� */
            case LAYER_TYPE_GEMETRY:
            {
                gui_geometry_layer_attr_t *geom_attr=(gui_geometry_layer_attr_t *)layer->attr;

                if(__get_range(win_rg,&lyr_rg,&pix_rg))
                {
                    break;
                }

                //TODO:���������ж����˱����ķ�ʽ��̫��ʱ�ˣ���Ҫ�Ľ�
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

                        //��ֱ ֱ��
                        if(line->points[0].x==(float)line->points[1].x)
                        {
                            for (j = 0; j < pix_rg.read_h; j++)
                            {
                                fill_fb[(j + win_rg->offset_y) * GUI_RES_X + win_rg->offset_x ] = layer->front_color;
                            }
                            break;
                        }

                        //ˮƽ ֱ��
                        else if(line->points[0].y==(float)line->points[1].y)
                        {
                            for (k = 0; k < pix_rg.read_w; k++)
                            {
                                fill_fb[ win_rg->offset_y * GUI_RES_X + win_rg->offset_x +k] = layer->front_color;
                            }
                            break;
                        }

                        //б��
                        else
                        {
                            /*
                             * y=kx+b,k=(y1-y0)/(x1-x0)
                             * ==>b=y-kx
                             * ==>x=(y-b)/k
                             */

                            float dx=(float)line->points[0].x-(float)line->points[1].x;
                            float dy=(float)line->points[0].y-(float)line->points[1].y;
                            float line_k=dy/dx;//TODO:�޸���б������С�������޴�����
                            float line_b=(float)line->points[0].y-line_k*(float)line->points[0].x;

                            //ȡ��ֵ������Ϊ������������Ϊ��ֵС���Ǹ�����ܻ�����ͬ��ֵ
                            if(fabs(dx)>=fabs(dy))
                            {
                                for (k = 0; k < pix_rg.read_w; k++)
                                {
                                    //����j��ʾ y, k��ʾ x
                                    //y=kx+b
                                    j=line_k*(k+pix_rg.read_offset_x+lyr_rg.layer_start_x)+line_b+0.5f;//0.5 Ϊ4��5��

                                   if(j>=win_rg->start_y && j<win_rg->start_y+win_rg->disp_h)
                                   {
                                       fill_fb[(j-win_rg->start_y)*GUI_RES_X+k+win_rg->offset_x]
                                               =layer->front_color;
                                   }

                                   //�������Ч
                                   /*fill_fb[(j-lyr_rg.layer_start_y-pix_rg.read_offset_y + win_rg->offset_y)
                                           * GUI_RES_X + win_rg->offset_x + k] =layer->front_color;*/
                                }
                            }
                            else
                            {
                                for (j = 0; j < pix_rg.read_h; j++)
                                {
                                    //����j��ʾ y, k��ʾ x
                                    //(y-b)/k
                                    k=(j+pix_rg.read_offset_y+lyr_rg.layer_start_y-line_b)/line_k+0.5f;//0.5 Ϊ4��5��

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
                        //TODO:��ʵ��
                        break;
                    }
                    case GUI_GEOMETRY_CIRCLE:
                    {
//                        gui_geometry_circle_t *circle=(gui_geometry_circle_t *)geom_attr->element;
                        //TODO:��ʵ��
                        break;
                    }
                    case GUI_GEOMETRY_ARC:
                    {
//                        gui_geometry_circle_t *circle=(gui_geometry_circle_t *)geom_attr->element;
                        //TODO:��ʵ��
                        break;
                    }

                    default:/* ��֧�ֵ����� */
                    {
                        break;
                    }
                }

                break;
            }

            /*
             * ��״ͼ
             */
            case LAYER_TYPE_HISTOGRAM:
            {
                gui_histogram_layer_attr_t *histogram_attr=(gui_histogram_layer_attr_t *)layer->attr;
                if(__get_range(win_rg,&lyr_rg,&pix_rg))
                {
                    break;
                }

                uint16_t _w, _h,                   //ʵ����ʾ�����ӿ��
                          _offset_x,_offset_y,     //ʵ�ʶ�ȡ������xyƫ�� --�൱��ͼ��ƫ��
                          lyr_offset_x,lyr_offset_y; //������ͼ���ڵ�ƫ��  --�൱�ڴ���ƫ��

                for(j=0;j<histogram_attr->num;j++)
                {
                    gui_pillar_t *pillar=&histogram_attr->pillars[j];

                    //����������ͼ���ڵ�λ��
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

                    //����ÿ�����ӵ�ƫ��
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
 * ��/�ر���ʾˢ��
 * �ڹر���Ļʱ�����ô���״̬Ϊ GUI_WIN_CLOSE ������ʱ����Ϊ GUI_WIN_OPEN���Ա���һֱ��ˢ����Ļ
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
 * ��¼��ǰˢ�µĴ����Ƿ�ı䣬���ڼ�����ʾ
 */
static gui_window_t *last_disp_gwin=NULL;

/**
@brief : ��ʾ���ڣ�ʵʱ���ش���������ʾ��
                            �˷����ʺ����ڴ��������и��µ���ʾ�����ڴ�������û�и���ֻ�Ǵ�������仯�ģ���ʹ�� gui_window_show_pre ����

                    ע���������δ������ɣ�����ʾ

@param :
-hdl       ���ھ�������Ϊhdl=NULL����ʾջ������

@retval:
- ��
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
                    //��DMA���ٴ��� ,��ʱΪ 0 ms
                    dma_start((uint32_t)fill_fb,(uint32_t)disp_fb,
                        GUI_RES_Y_DIV*GUI_RES_X*GUI_BYTES_PER_PIX);
                }
                #else
                {
                    //�����ʱ 7ms
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
                //��DMA���ٴ��� ,��ʱΪ 0 ms
                dma_start((uint32_t)fill_fb,(uint32_t)disp_fb,GUI_RES_Y*GUI_RES_X*GUI_BYTES_PER_PIX);
            }
            #else
            {
                //�����ʱ 7ms
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
@brief : ��ʾ����:ʹ��Ԥ���صĴ������������ʾ��Ԥ���ص����ݱ����� back_fb ����ʾ������ʹ�� front_fb

         ע�⣺   �˷����ʺ����ڴ�������û�и���ֻ�Ǵ�������仯�ģ������Ч��������Ч���������ڴ��������и��µ���ʾ��ʹ�� gui_window_show ����

        ͬһ������ʾ������ƣ�
              ��¼��һ����ʾ������ ��last_disp_x ��last_disp_y�����뱾������Ƚ� ��gwin->x��gwin->y��
              �õ���ֵ�� diff_x,diff_y��������һ�ε���ʾ���� back_fb �ƶ��� diff_x,diff_y����
              Ȼ���flash�ж�ȡ�µ�ƫ������


@param :
-hdl       ���ھ�������Ϊhdl=NULL����ʾջ������

@retval:
- ��
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

#define DMA_START_PIX (100)    //ʹ��DMAʱ�����������ݵ�������С���ص㣬С�ڸ�ֵ��ʹ��DMA

    //���ڱ仯��Ҫ���¼���
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
         * ���������ƶ�
         */
        if(_x!=0)
        {
            if(_x>0)//���������ƶ�
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
                    for(i=0;i<GUI_RES_Y;i++)//�����ַ�ǵ��ŵģ���������DMA
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
         * ���������ƶ�
         */
        if(_y!=0)
        {
           if(_y>0)//���������ƶ�
           {
               for(i=0;i<GUI_RES_Y-_y;i++)
               {
                   #if USE_DMA_SPEED_UP_RENDER_PRE
                   {
                        //��DMA���ٴ��� ,
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
                        //��DMA���ٴ��� ,
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
                        //��DMA���ٴ��� ,
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
                        //��DMA���ٴ��� ,
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
         * ���������ƶ�
         */
        if(_x!=0)
        {
            if(_x>0)//���������ƶ�
            {
                #if USE_DMA_SPEED_UP_RENDER_PRE
                {
                    /*
                     * DMA�����г������ƣ�Ϊ����һ�δ������ 0xffff ��
                     * ������������δ���
                     *
                     * ע�⣺
                     *  �����2�δ���ֻ�ʺ�360*360��240*240�ֱ��ʵ�����������ֱ���Ҫ�����¿��᲻����ʲôbug
                     *  ����Ҫ��DMA������ᴫ��ʧ�ܣ�
                     *
                     * ��ʱ1-3ms
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
                    //�����ַ�ǵ��ŵģ�����ֱ����DMA����������DMA�� disp_fb ��ǰ�ƣ� disp_fb �ڶ���bufʱǰ�����Ҫ����Ԥ����һ�У��������������ƶ�
                    //��ʱ11ms

                    uint16_t *new_disp_fb=(uint16_t *)(disp_fb-GUI_RES_X);

                    dma_start_k((uint32_t)&disp_fb[0],(uint32_t)&new_disp_fb[_x],
                        (GUI_RES_X*GUI_RES_Y/2-_x)*GUI_BYTES_PER_PIX,true,true,2);

                    dma_start_k((uint32_t)&disp_fb[GUI_RES_X*GUI_RES_Y/2*1],
                        (uint32_t)&new_disp_fb[GUI_RES_X*GUI_RES_Y/2*1+_x],
                        (GUI_RES_X*GUI_RES_Y/2-_x)*GUI_BYTES_PER_PIX,true,true,2);

                   for(i=0;i<GUI_RES_Y;i++)
                   {
                        //��DMA���ٴ��� ,
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
         * ���������ƶ�
         */
        if(_y!=0)
        {
           if(_y>0)//���������ƶ�
           {
               for(i=0;i<GUI_RES_Y-_y;i++)
               {
                   #if USE_DMA_SPEED_UP_RENDER_PRE
                   {
                        //��DMA���ٴ��� ,
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
                        //��DMA���ٴ��� ,
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
     * �����������ʱ����֧�����ַ�ʽˢ�����ٶ�̫��
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
@brief : ��ʾջ��2��������ϳɵĴ��ڣ����û��2�����ڣ���ô��������

@param :
-top_pix    ջ��������ʾ�����أ�ջ���ڶ���������Ϊ GUI_RES_X-top_pix��
-effect     ����������ϵ�Ч��

@retval:
- ��

 * ע��
 *
 * �л�����������ʾ������ƣ�
 *
 * �Ѿɴ��ڣ���һ�����е�APP���ڣ�last_gwin�������ݼ��ص� front_fb  ���´��ڣ��������е�APP���ڣ�gwin�������ݼ��ص� back_fb
 * 1 ���TP�ƶ����򲻸ı䣬��ô��ʾ��LCD�����ݣ�front_fb����ͬһ�����ƶ�ͬʱ��back_fb��䣬�����в���Ҫ���¶�ȡflash
 * 2 ���TP����ı䣬��ôlast_gwin��gwinҪ������ͬʱ��Ҫ����gwin��back_fb������ʹ�ò���1����
 *
 * TODO & BUG��
 *      1�� �÷�����TP�������һ���ʱ����Ȼ���Ըо�����ʾ�п��٣���Ϊ�״ζ�ȡ���ڵ�ʱ�䲢û���Ż���
 *      2������ķ�����֧�� ��buf��������Ҫ������buf
 *
 *      3)Ŀǰ�����ˢ�·�����֧���� GUI_RES_X �� GUI_RES_Y ��ͬ������������߲������Ҫ�޸�ʵ�ַ���
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

    //�״μ���ʱҪ�����ñ�־�����¶�ȡ����
    if(gwin->loaded || last_gwin->loaded)
    {
        window_relaod=true;
    }

    //��������л����ڣ���Ҫ�Ѵ��ڵ���������Ϊ������
    gwin->loaded=false;
    last_gwin->loaded=false;

    /*
     * ������صĴ��ڷ��������
     */
    uint16_t diff_pix;                        /* ������ʾ���ں���һ������ƶ������� */
    gui_window_t *load_gwin;
    static gui_window_t *last_load_gwin=NULL;
    int i,j;

    if(window_relaod )
    {
        last_top_pix=0;
    }

    if(top_pix>last_top_pix)//�������
    {
        load_gwin=gwin;
    }
    else if(top_pix<last_top_pix)
    {
        load_gwin=last_gwin;//�������
    }
    else
    {
        load_gwin=last_load_gwin;
    }

    diff_pix=top_pix>last_top_pix?top_pix-last_top_pix:last_top_pix-top_pix;

    //TODO:
    //    top_pix=GUI_RES_X��last_top_pix=0�ᵼ���޷���ʾ�´���
    //    --��Ŀǰ������������������Ϊtop_pix�����0ֱ������ GUI_RES_X(GUI_RES_Y)
    //
    //TODO:���GUI_RES_X��GUI_RES_Y����ȣ�������Ҫ�������ж�
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
        //�����������
        switch((int)effect)
        {
            case GUI_EFFECT_NONE://ֱ����ʾ disp_fb
            {
                break;
            }

            /* ����->�� ���� */
            case GUI_EFFECT_ROLL_L2R:
            {
                if(load_gwin==gwin)//����
                {
                    //�ɴ��������ƶ�
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

                    //�´���������
                    for(i=0;i<GUI_RES_Y;i++)
                    {
                        for (j = 0; j < top_pix; j++)
                        {
                            disp_fb[i*GUI_RES_X+j] = fill_fb[i*GUI_RES_X+GUI_RES_X-top_pix+j];
                        }
                    }
                }
                else //if(load_gwin==last_gwin)//����
                {
                    //�ɴ��������ƶ�
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

                    //�´���������
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

            /* ����->�� ���� */
            case GUI_EFFECT_ROLL_R2L:
            {
                if(load_gwin==gwin)//����
                {
                    //�ɴ��������ƶ�
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

                    //�´��������ƶ�
                    for(i=0;i<GUI_RES_Y;i++)
                    {
                        for (j = 0; j < top_pix; j++)
                        {
                            disp_fb[i*GUI_RES_X+j+GUI_RES_X-top_pix] = fill_fb[i*GUI_RES_X+j];
                        }
                    }
                }
                else //if(load_gwin==last_gwin)//����
                {
                    //�ɴ��������ƶ�
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

                    //�´���������
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

            /* ����->��ˢ */
            case GUI_EFFECT_CONVERT_U2D:
            {
                if(load_gwin==gwin)//����
                {
                    //�ɴ��ڲ��ƶ�

                    //�´��������ƶ�����ʾ�°벿�֣�
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
                else //if(load_gwin==last_gwin)//����
                {
                    //�ɴ��������ƶ�
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

                    //�´����ƶ�����ʾ�°벿�֣�
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

            /* ����->��ˢ */
            case GUI_EFFECT_CONVERT_D2U:
            {
                if(load_gwin==gwin)//����
                {
                    //�ɴ��ڲ��ƶ�

                    //�´��������ƶ�����ʾ�ϰ벿�֣�
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
                else //if(load_gwin==last_gwin)//����
                {
                    //�ɴ��������ƶ�
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

                    //�´����ƶ�����ʾ�ϰ벿�֣�
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

            /* ����->�� ˢ */
            case GUI_EFFECT_CONVERT_L2R:
            {
                //TODO��ʵ��
                break;
            }

            /* ����->��ˢ */
            case GUI_EFFECT_CONVERT_R2L:
            {
                if(load_gwin==gwin)//����
                {
                    //�ɴ��ڲ��ƶ�

                    //�´��������ƶ�����ʾ��벿�֣�
                    for(i=0;i<GUI_RES_Y;i++)
                    {
                        for (j = 0; j < top_pix ; j++)
                        {
                            disp_fb[i*GUI_RES_X+(GUI_RES_X-top_pix)+j] = fill_fb[i*GUI_RES_X+j];
                        }
                    }
                }
                else //if(load_gwin==last_gwin)//����
                {
                    //�ɴ��������ƶ�
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

                    //�´����ƶ�����ʾ��벿�֣�
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

            /* ����->��ˢ  �ķ���*/
            case GUI_EFFECT_CONVERT_D2U_REVERSE:
            {
                if(load_gwin==gwin)//����
                {
                    //�ɴ��������ƶ�
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

                    //�´��ڲ��ƶ�����ʾ�ϰ벿�֣�
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
                else //if(load_gwin==last_gwin)//����
                {
                    //�ɴ��ڲ��ƶ�

                    //�´������ƶ�����ʾ�ϰ벿�֣�
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

            /* ����->��ˢ  �ķ��� */
            case GUI_EFFECT_CONVERT_U2D_REVERSE:
            {
                if(load_gwin==gwin)//����
                {
                    //�ɴ��������ƶ�
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

                    //�´��ڲ��ƶ�����ʾ�°벿�֣�
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
                else //if(load_gwin==last_gwin)//����
                {
                    //�ɴ��ڲ��ƶ�

                    //�´������ƶ�����ʾ�°벿�֣�
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

            /* ����->��ˢ  �ķ��� */
            case GUI_EFFECT_CONVERT_R2L_REVERSE:
            {
                if(load_gwin==gwin)//����
                {
                    //�ɴ��������ƶ�
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

                    //�´����ƶ�����ʾ��벿�֣�
                    for(i=0;i<GUI_RES_Y;i++)
                    {
                        for (j = 0; j < top_pix; j++)
                        {
                            disp_fb[i*GUI_RES_X+j] = fill_fb[i*GUI_RES_X+j];
                        }
                    }

                }
                else //if(load_gwin==last_gwin)//����
                {
                    //�ɴ��ڲ��ƶ�

                    //�´��������ƶ�����ʾ��벿�֣�
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

            /* ����->��ˢ  �ķ��� */
            case GUI_EFFECT_CONVERT_L2R_REVERSE:
            {
                //TODO��ʵ��
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
        //�����������
        switch((int)effect)
        {
            case GUI_EFFECT_NONE://ֱ����ʾ disp_fb
            {
                break;
            }

            /* ����->�� ���� */
            case GUI_EFFECT_ROLL_L2R:
            {
                if(load_gwin==gwin)//����
                {
                    //�ɴ��������ƶ������������ƶ�offset_pix������
                    #if USE_DMA_SPEED_UP_RENDER_MIX
                    {
                        if(diff_pix<GUI_RES_X)
                        {
                            //��DMA���ٴ��� ,
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

                    //�´��������ƣ�ֻ��ȡoffset_pix������
                    __gui_window_fb_fill(load_gwin,disp_fb,GUI_RES_X-top_pix,diff_pix,0,GUI_RES_Y);
                }
                else //if(load_gwin==last_gwin)//����
                {
                    //�ɴ��������ƶ������������ƶ�offset_pix������
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

                    //�´���������
                    __gui_window_fb_fill(load_gwin,&disp_fb[GUI_RES_X-diff_pix],
                        GUI_RES_X-top_pix-diff_pix,diff_pix,0,GUI_RES_Y);
                }

                break;
            }

            /* ����->�� ���� */
            case GUI_EFFECT_ROLL_R2L:
            {
                if(load_gwin==gwin)//����
                {
                    //�ɴ��������ƶ�
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

                    //�´��������ƶ�
                    __gui_window_fb_fill(load_gwin,&disp_fb[GUI_RES_X-diff_pix],top_pix-diff_pix,diff_pix,0,GUI_RES_Y);
                }
                else //if(load_gwin==last_gwin)//����
                {
                    //�ɴ��������ƶ�
                    #if USE_DMA_SPEED_UP_RENDER_MIX
                    {
                        if(diff_pix<GUI_RES_X)
                        {
                            //��DMA���ٴ��� ,
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

                    //�´���������
                    __gui_window_fb_fill(load_gwin,disp_fb,top_pix,diff_pix,0,GUI_RES_Y);
                }

                break;
            }

            /* ����->��ˢ �������˵���*/
            case GUI_EFFECT_CONVERT_U2D:
            {
                if(load_gwin==gwin)//����
                {
                    //�ɴ��ڲ��ƶ�

                    //�´��������ƶ�����ʾ�°벿�֣�
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
                else //if(load_gwin==last_gwin)//����
                {
                    //�ɴ��������ƶ�   TODO:��Ҫ�޸� top_pix>360���������ʱDMA���䳤��̫��
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
                    //�´����ƶ�����ʾ�°벿�֣�
                    __gui_window_fb_fill(load_gwin,&disp_fb[top_pix*GUI_RES_X],
                        0,GUI_RES_X,top_pix,diff_pix);

                }
                break;
            }

            /* ����->��ˢ  �ķ��� �������˵��ϻ���*/
            case GUI_EFFECT_CONVERT_U2D_REVERSE:
            {
                if(load_gwin==gwin)//����
                {
                    //�ɴ��������ƶ�   TODO:��Ҫ�޸� GUI_RES_Y-top_pix>360���������ʱDMA���䳤��̫��
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

                    //�´����ƶ�����ʾ�°벿�֣�
                    __gui_window_fb_fill(load_gwin,&disp_fb[(GUI_RES_Y-top_pix)*GUI_RES_X],
                        0,GUI_RES_X,(GUI_RES_Y-top_pix),diff_pix);
                }
                else //if(load_gwin==last_gwin)//����
                {
                    //�ɴ��ڲ��ƶ�

                    //�´��������ƶ�����ʾ�°벿�֣�
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

            /* ����->��ˢ  �������˵���*/
            case GUI_EFFECT_CONVERT_D2U:
            {
                if(load_gwin==gwin)//����
                {
                    //�ɴ��ڲ��ƶ�

                    //�´��������ƶ�����ʾ�ϰ벿�֣�  TODO:��Ҫ�޸� top_pix-diff_pix>360���������ʱDMA���䳤��̫��
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
                else //if(load_gwin==last_gwin)//����
                {
                    //�ɴ��������ƶ�
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

                    //�´����ƶ�
                    __gui_window_fb_fill(load_gwin,(uint16_t*)&disp_fb[(GUI_RES_Y-top_pix-diff_pix)*GUI_RES_X],
                            0,GUI_RES_X,GUI_RES_Y-top_pix-diff_pix,diff_pix);
                }

                break;
            }

            /* ����->��ˢ  �ķ��� �������˵��»���*/
            case GUI_EFFECT_CONVERT_D2U_REVERSE:
            {
                if(load_gwin==gwin)//����
                {
                    //�ɴ��������ƶ�
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

                    //�´��ڲ��ƶ�
                    __gui_window_fb_fill(load_gwin,(uint16_t*)&disp_fb[(top_pix-diff_pix)*GUI_RES_X],
                            0,GUI_RES_X,top_pix-diff_pix,diff_pix);
                }
                else //if(load_gwin==last_gwin)//����
                {
                    //�ɴ��ڲ��ƶ�

                    //�´������ƶ�    TODO:��Ҫ�޸� GUI_RES_Y-top_pix-diff_pix>360���������ʱDMA���䳤��̫��
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

            /* ����->��ˢ  ���һ����룩*/
            case GUI_EFFECT_CONVERT_R2L:
            {
                break;
            }

            /* ����->��ˢ  �ķ��� ���һ��˳���*/
            case GUI_EFFECT_CONVERT_R2L_REVERSE:
            {
                if(load_gwin==gwin)//����
                {
                    //�ɴ��������ƶ�
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

                    //�´����ƶ�����ʾ��벿�֣�

                    __gui_window_fb_fill(load_gwin,&disp_fb[top_pix-diff_pix],
                            top_pix-diff_pix,diff_pix,0,GUI_RES_Y);

                }
                else //if(load_gwin==last_gwin)//����
                {
                    //�ɴ��ڲ��ƶ�

                    //�´��������ƶ�����ʾ��벿�֣�
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

            /* ����->�� ˢ */
            case GUI_EFFECT_CONVERT_L2R:
            {
                //TODO��ʵ��
                break;
            }

            /* ����->��ˢ  �ķ��� */
            case GUI_EFFECT_CONVERT_L2R_REVERSE:
            {
                //TODO��ʵ��
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

    //��ʱ50-90ms���ɴ������ݸ��ӶȾ���
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
     * �����������ʱ����֧�����ַ�ʽˢ�����ٶ�̫��
     */
#if GUI_BUF_NEED_DIV
    return ;
#endif

    gui_window_render_mix(top_pix,effect);
}



