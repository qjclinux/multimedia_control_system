
/*
 * app_default_dial1.c
 *
 *  Created on: 2019��12��28��
 *      Author: jace
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <system_state_service.h>
#include <time.h>
#include "osal.h"
#include "console.h"
#include "gui_window.h"
#include "app_mgr.h"
#include "gui.h"
#include "app_config.h"
#include "time_counter_task.h"
#include "app_light_words.h"
#include "remote.h"

#define DEBUG_APP_SETTING 0
#if DEBUG_APP_SETTING
    #define APP_SETTING_LOG(fmt,args...) do{\
            /*printf("%s,%s(),%d:" ,__FILE__, __FUNCTION__,__LINE__);*/\
            os_printk("%s(),%d:" , __FUNCTION__,__LINE__);\
            os_printk(fmt,##args);\
        }while(0)
    #define APP_SETTING_INFO(fmt,args...)        do{\
            os_printk(fmt,##args);\
        }while(0)
#else
    #define APP_SETTING_LOG(fmt,args...)
    #define APP_SETTING_INFO(fmt,args...)
#endif
/*---------------------------------------------------------------------------------------------------*/

/*
 * �����϶��ƶ� ����
 */
static int16_t
    m_win_x=0,        //��ǰ������ʾ�Ŀ�ʼλ��
    m_win_y=0,
    m_last_x=0,       //�ϴ��϶����ڵ�����
    m_last_y=0
    ;

#define WINDOW_ROLL_DELAY_TIME (0)          //���ڶ������Ч��ˢ����ʱ
#define WINDOW_ROLL_VAL 20                  //ͼ���Զ��������ֵ
#define WINDOW_ALIGN_VAL 40                 //ͼ�����y���ֵ
static uint16_t win_height=GUI_RES_Y;       //����������ʾƫ��

#define WINDOW_ADDED (WINDOW_ALIGN_VAL)     //ͷβ�ع��ռ�
#define WIN_DRAG_START_VAL (5)

/*
 * ѡ�� ����
 */
typedef struct{
    void* hdl;
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
}click_item_hdl_t;

static click_item_hdl_t *m_click_item;
static uint8_t click_item_num=0;
static uint8_t selected_item=0;

//RGB ��ɫ���ý�����
#define RED_START_X 68
#define RED_END_X (RED_START_X+120)
#define RED_START_Y (10)

#define GREEN_START_X 68
#define GREEN_END_X (RED_START_X+120)
#define GREEN_START_Y (RED_START_Y+WINDOW_ALIGN_VAL)

#define BLUE_START_X 68
#define BLUE_END_X (RED_START_X+120)
#define BLUE_START_Y (RED_START_Y+WINDOW_ALIGN_VAL*2)

#define RGB_VALUE_MAX (120)
#define RGB_VALUE_TO_RGB(val) ((val)*255/RGB_VALUE_MAX)
static uint8_t m_red_value=60,m_green_value=60,m_blue_value=60;

/*---------------------------------------------------------------------------------------------------*/

static void sys_sta_event_cb(uint32_t event, const void *pdata, uint16_t size)
{
    if (SYS_STA_APP_ON_EXIT & event)
    {
        if(m_click_item)
            OS_FREE(m_click_item);
        m_click_item=NULL;
        click_item_num=0;

        APP_SETTING_LOG("setting on exit\r\n");
    }

}

/*---------------------------------------------------------------------------------------------------*/

//�����϶��¼�
static void gui_event_on_grag(void* hdl,uint8_t *pdata, uint16_t size)
{
    gui_evt_tp_data_t *tp_data=(gui_evt_tp_data_t *)pdata;
//    APP_SETTING_LOG("drag tp={%d,%d,%d}\r\n", tp_data->evt_id,tp_data->x,tp_data->y);

    int16_t diff_y=m_last_y-tp_data->y;

    //�Ѿ����ﶥ��/�׶ˣ���Ҫ����Ļ
    if((m_win_y>=win_height && diff_y>0 )|| (m_win_y==0 && diff_y<0))
        return;

    //��ֵС��5��ˢ��
    else if(abs(diff_y)<WIN_DRAG_START_VAL)
    {
        return;
    }

    m_win_y+=diff_y;

//    APP_SETTING_LOG("drag y=%d dy=%d\r\n", m_win_y,diff_y);
    APP_SETTING_LOG("drag tp={%d,%d,%d} wy=%d dy=%d \r\n",
        tp_data->evt_id,tp_data->x,tp_data->y,
        m_win_y,diff_y);

    if(m_win_y<0)
        m_win_y=0;
    else if(m_win_y>win_height)
        m_win_y=win_height;

    m_last_x=tp_data->x;
    m_last_y=tp_data->y;

    gui_window_set_position(hdl, m_win_x, m_win_y);
    gui_window_show(hdl);
//    gui_window_show_pre(hdl);
}

//TP���£���ʼ�����µ�����
static void gui_event_on_pressed(void* hdl,uint8_t *pdata, uint16_t size)
{
    gui_evt_tp_data_t *tp_data=(gui_evt_tp_data_t *)pdata;
    APP_SETTING_LOG("press tp={%d,%d,%d}\r\n", tp_data->evt_id,tp_data->x,tp_data->y);

    m_last_x=tp_data->x;
    m_last_y=tp_data->y;

    gui_window_set_position(hdl, m_win_x, m_win_y);
    gui_window_show(hdl);
//    gui_window_show_pre(hdl);
}

//���� ������ͼ�����
static void roll_up_align(void* hdl)
{
    if(m_win_y>=win_height)
        return ;

    int16_t _y;
    _y=(m_win_y+WINDOW_ALIGN_VAL-1)/WINDOW_ALIGN_VAL*WINDOW_ALIGN_VAL;

    for(int16_t i=m_win_y;i<_y;i+=WINDOW_ROLL_VAL)
    {
        if(i>win_height)
            break;

        gui_window_set_position(hdl, m_win_x, i);
        gui_window_show(hdl);
//        gui_window_show_pre(hdl);
    }

    gui_window_set_position(hdl, m_win_x, _y);
    gui_window_show(hdl);
//    gui_window_show_pre(hdl);

    m_win_y=_y;
}

//���� ������ͼ�����
static void roll_down_align(void* hdl)
{
    if(m_win_y==0)
        return ;

    int16_t _y;
    _y=(m_win_y)/WINDOW_ALIGN_VAL*WINDOW_ALIGN_VAL;

    for(int16_t i=m_win_y;i>_y;i-=WINDOW_ROLL_VAL)
    {
        if(i<0)
            break;

        gui_window_set_position(hdl, m_win_x, i);
        gui_window_show(hdl);
//        gui_window_show_pre(hdl);
    }

    gui_window_set_position(hdl, m_win_x, _y);
    gui_window_show(hdl);
//    gui_window_show_pre(hdl);

    m_win_y=_y;
}

//���ϻ���ʱ���Ѵ���y��һ������
static void gui_event_on_slip_up(void* hdl,uint8_t *pdata, uint16_t size)
{

}

//���»���ʱ���Ѵ���y��һ������
static void gui_event_on_slip_down(void* hdl,uint8_t *pdata, uint16_t size)
{

}

//TP�ɿ����Ѵ��ڶ���
static void gui_event_on_released(void* hdl,uint8_t *pdata, uint16_t size)
{
//    gui_evt_tp_data_t *tp_data=(gui_evt_tp_data_t *)pdata;
//    APP_SETTING_LOG("release tp={%d,%d,%d}\r\n", tp_data->evt_id,tp_data->x,tp_data->y);

//    int16_t diff_y=m_last_y-tp_data->y;
//
//    APP_SETTING_LOG("rels tp={%d,%d,%d} wy=%d dy=%d \r\n",
//            tp_data->evt_id,tp_data->x,tp_data->y,
//            m_win_y,diff_y);

    //��������׿��ˣ����˻�ȥ
    if(m_win_y>=win_height-WINDOW_ALIGN_VAL)
    {
        int16_t _y=win_height-WINDOW_ALIGN_VAL;

        for(int16_t i=m_win_y;i>_y;i-=WINDOW_ROLL_VAL)
        {
            if(i<0)
                break;

            gui_window_set_position(hdl, m_win_x, i);
            gui_window_show(hdl);
//            gui_window_show_pre(hdl);
        }

        gui_window_set_position(hdl, m_win_x, _y);
        gui_window_show(hdl);
//        gui_window_show_pre(hdl);

        m_win_y=_y;
        return ;
    }
    //�������ˣ��������Ҫ��ˢһ����������������޷���ʧ
    else if(m_win_y==0)
    {
        gui_window_set_position(hdl, m_win_x, 0);
        gui_window_show(hdl);
//        gui_window_show_pre(hdl);
        return ;
    }

    if ((m_win_y % WINDOW_ALIGN_VAL) > WINDOW_ALIGN_VAL/2)
    {
        roll_up_align(hdl);
    }
    else
    {
        roll_down_align(hdl);
    }

}

/*---------------------------------------------------------------------------------------------------*/
static gui_layer_hdl_t select_lyr_hdl=NULL;

static void gui_event_on_click(void* hdl,uint8_t *pdata, uint16_t size)
{
    gui_evt_tp_data_t *tp_data=(gui_evt_tp_data_t *)pdata;
    APP_SETTING_LOG("hdl=%08x tp={%d,%d,%d}\r\n", (uint32_t)hdl,tp_data->evt_id,tp_data->x,tp_data->y);

    uint16_t y=tp_data->y+gui_window_get_y(hdl);//תΪ��������

    for(int i=0;i<click_item_num;i++)
    {

        if(m_click_item[i].x<=tp_data->x && (m_click_item[i].x+m_click_item[i].w)>tp_data->x
                && m_click_item[i].y<=y && (m_click_item[i].y+m_click_item[i].h)>y
                )
        {
            OS_LOG("set x=%d y=%d,i=%d\r\n",m_click_item[i].x,m_click_item[i].y,i);

            selected_item=i;
            gui_layer_set_position(select_lyr_hdl,0,m_click_item[i].y);
//            gui_window_show_pre(hdl);
            gui_window_show(hdl);

            //����ң������
            IR_SEND_MODE(selected_item);

            break;
        }

    }
}

static gui_layer_hdl_t red_proc_lyr_hdl=NULL;
static gui_layer_hdl_t green_proc_lyr_hdl=NULL;
static gui_layer_hdl_t blue_proc_lyr_hdl=NULL;

//��ɫ ��
static void gui_event_on_click_red_down(void* hdl,uint8_t *pdata, uint16_t size)
{
    if(m_red_value)
    {
        m_red_value--;
        gui_layer_set_position(red_proc_lyr_hdl,RED_START_X+m_red_value,RED_START_Y);
        gui_window_show(NULL);

        //TODO:����ң������
        IR_SEND_RGB(RGB_VALUE_TO_RGB(m_red_value),RGB_VALUE_TO_RGB(m_green_value),RGB_VALUE_TO_RGB(m_blue_value));
    }
}
//��ɫ +
static void gui_event_on_click_red_up(void* hdl,uint8_t *pdata, uint16_t size)
{
    if(m_red_value<RGB_VALUE_MAX)
    {
        m_red_value++;
        gui_layer_set_position(red_proc_lyr_hdl,RED_START_X+m_red_value,RED_START_Y);
        gui_window_show(NULL);

        //TODO:����ң������
        IR_SEND_RGB(RGB_VALUE_TO_RGB(m_red_value),RGB_VALUE_TO_RGB(m_green_value),RGB_VALUE_TO_RGB(m_blue_value));
    }
}
//��ɫ   ֱ�ӵ������
static void gui_event_on_click_red(void* hdl,uint8_t *pdata, uint16_t size)
{
    gui_evt_tp_data_t *tp_data=(gui_evt_tp_data_t *)pdata;

    m_red_value=tp_data->x-RED_START_X;
    gui_layer_set_position(red_proc_lyr_hdl,RED_START_X+m_red_value,RED_START_Y);
    gui_window_show(NULL);

    //TODO:����ң������
    IR_SEND_RGB(RGB_VALUE_TO_RGB(m_red_value),RGB_VALUE_TO_RGB(m_green_value),RGB_VALUE_TO_RGB(m_blue_value));

}

//��ɫ  ��
static void gui_event_on_click_green_down(void* hdl,uint8_t *pdata, uint16_t size)
{
    if(m_green_value)
    {
        m_green_value--;
        gui_layer_set_position(green_proc_lyr_hdl,GREEN_START_X+m_green_value,GREEN_START_Y);
        gui_window_show(NULL);
        //TODO:����ң������
        IR_SEND_RGB(RGB_VALUE_TO_RGB(m_red_value),RGB_VALUE_TO_RGB(m_green_value),RGB_VALUE_TO_RGB(m_blue_value));
    }
}

//��ɫ +
static void gui_event_on_click_green_up(void* hdl,uint8_t *pdata, uint16_t size)
{
    if(m_green_value<RGB_VALUE_MAX)
    {
        m_green_value++;
        gui_layer_set_position(green_proc_lyr_hdl,GREEN_START_X+m_green_value,GREEN_START_Y);
        gui_window_show(NULL);
        //TODO:����ң������
        IR_SEND_RGB(RGB_VALUE_TO_RGB(m_red_value),RGB_VALUE_TO_RGB(m_green_value),RGB_VALUE_TO_RGB(m_blue_value));
    }
}

//��ɫ   ֱ�ӵ������
static void gui_event_on_click_green(void* hdl,uint8_t *pdata, uint16_t size)
{
    gui_evt_tp_data_t *tp_data=(gui_evt_tp_data_t *)pdata;

    m_green_value=tp_data->x-GREEN_START_X;
    gui_layer_set_position(green_proc_lyr_hdl,GREEN_START_X+m_green_value,GREEN_START_Y);
    gui_window_show(NULL);

    //TODO:����ң������
    IR_SEND_RGB(RGB_VALUE_TO_RGB(m_red_value),RGB_VALUE_TO_RGB(m_green_value),RGB_VALUE_TO_RGB(m_blue_value));

}

//��ɫ  ��
static void gui_event_on_click_blue_down(void* hdl,uint8_t *pdata, uint16_t size)
{
    if(m_blue_value)
    {
        m_blue_value--;
        gui_layer_set_position(blue_proc_lyr_hdl,BLUE_START_X+m_blue_value,BLUE_START_Y);
        gui_window_show(NULL);
        //TODO:����ң������
        IR_SEND_RGB(RGB_VALUE_TO_RGB(m_red_value),RGB_VALUE_TO_RGB(m_green_value),RGB_VALUE_TO_RGB(m_blue_value));
    }
}
//��ɫ +
static void gui_event_on_click_blue_up(void* hdl,uint8_t *pdata, uint16_t size)
{
    if(m_blue_value<RGB_VALUE_MAX)
    {
        m_blue_value++;
        gui_layer_set_position(blue_proc_lyr_hdl,BLUE_START_X+m_blue_value,BLUE_START_Y);
        gui_window_show(NULL);
        //TODO:����ң������
        IR_SEND_RGB(RGB_VALUE_TO_RGB(m_red_value),RGB_VALUE_TO_RGB(m_green_value),RGB_VALUE_TO_RGB(m_blue_value));
    }
}
//��ɫ   ֱ�ӵ������
static void gui_event_on_click_blue(void* hdl,uint8_t *pdata, uint16_t size)
{
    gui_evt_tp_data_t *tp_data=(gui_evt_tp_data_t *)pdata;

    m_blue_value=tp_data->x-BLUE_START_X;
    gui_layer_set_position(blue_proc_lyr_hdl,BLUE_START_X+m_blue_value,BLUE_START_Y);
    gui_window_show(NULL);

    //TODO:����ң������
    IR_SEND_RGB(RGB_VALUE_TO_RGB(m_red_value),RGB_VALUE_TO_RGB(m_green_value),RGB_VALUE_TO_RGB(m_blue_value));
}
/*---------------------------------------------------------------------------------------------------*/
typedef struct{
        const uint8_t *words_data;
        uint8_t words_len;
    }select_item_t;

static const select_item_t select_item[]={
    {(const uint8_t*)words00,4},
    {(const uint8_t*)words01,4},
    {(const uint8_t*)words02,5},
    {(const uint8_t*)words03,4},
    {(const uint8_t*)words04,5},
    {(const uint8_t*)words05,4},
    {(const uint8_t*)words06,4},
    {(const uint8_t*)words08,4},
    {(const uint8_t*)words09,4},
    {(const uint8_t*)words11,4},
    {(const uint8_t*)words12,4},
    {(const uint8_t*)words13,4},
    {(const uint8_t*)words14,3},
    {(const uint8_t*)words15,3},
    {(const uint8_t*)words16,2},
    {(const uint8_t*)words18,3},
    {(const uint8_t*)words20,2},
    {(const uint8_t*)words21,2},
    {(const uint8_t*)words22,2},
    {(const uint8_t*)words23,5},
    {(const uint8_t*)words25,4},
    {(const uint8_t*)words26,4},
    {(const uint8_t*)words27,4},
    {(const uint8_t*)words29,2},
    {(const uint8_t*)words30,2},
};
#define ITEM_NUM (sizeof(select_item)/(sizeof(select_item_t)))
#define FONT_W 24
#define FONT_H 24
/*
 * ϵͳӦ�ò˵������»���
 *
 *
 */
static void setting_window_init()
{
    APP_SETTING_LOG("create \r\n");

    gui_window_hdl_t hdl;
    gui_layer_hdl_t lyr_hdl;
    uint16_t num=0;
    uint16_t x,y,offset;


    m_win_x=0;
//    m_win_y=0;//���ﲻ�����Ϊ�˱����ϴ��˳�����ʾλ��
    m_last_x=0;
    m_last_y=0;
    offset=0;

    click_item_num=ITEM_NUM;

    //ע�⣺�˴�APP�˳�Ҫ�����ڴ�
    m_click_item=OS_MALLOC(sizeof(click_item_hdl_t)*click_item_num);

    if(!m_click_item)
    {
        return ;
    }

    //һҳ��ʾ10��ѡ�4��ͷβ�ع��ռ�
    win_height=WINDOW_ADDED+WINDOW_ALIGN_VAL*(click_item_num+4);

    //��ʾ����ͷ�հײ���
//    m_win_y=WINDOW_ADDED;

    /*
     * ���ڣ� ���»���
     */
    hdl=gui_window_create(0,m_win_y,GUI_RES_X,win_height);
    gui_window_set_scroll(hdl,GUI_ENABLE_SCROLL);

#define COLOR (RGB888_TO_RGB565(128<<16|128<<8|0<<0))
    gui_window_set_bg_color(hdl,((COLOR>>8)&0xff)|((COLOR&0xff)<<8) );



#if SUPPORT_MENU_ROLL
    setting_win_hdl=hdl;
#endif

    /* �����¼� */
    gui_window_subscribe_event(hdl,GUI_EVT_ON_DRAG,gui_event_on_grag);
    gui_window_subscribe_event(hdl,GUI_EVT_ON_PRESSED,gui_event_on_pressed);
    gui_window_subscribe_event(hdl,GUI_EVT_ON_RELEASED,gui_event_on_released);
    gui_window_subscribe_event(hdl,GUI_EVT_ON_SLIP_UP,gui_event_on_slip_up);
    gui_window_subscribe_event(hdl,GUI_EVT_ON_SLIP_DOWN,gui_event_on_slip_down);

    gui_window_subscribe_event(hdl,GUI_EVT_ON_CLICK,gui_event_on_click);

    //ѡ��
    lyr_hdl=gui_create_bmp_layerk(RES_BMP_49,0,0);
    gui_window_add_layer(hdl,lyr_hdl);
    select_lyr_hdl=lyr_hdl;

    /*
     * ������棺�ڶ�����RGB��
     */
    offset=WINDOW_ALIGN_VAL*4;


    //��
    lyr_hdl=gui_create_pixel_layer(4,RED_START_Y,FONT_W,FONT_H,GUI_PIX_TYPE_RGB1,((uint8_t*)wordsrgb));
    gui_window_add_layer(hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    lyr_hdl=gui_create_bmp_layerk(RES_BMP_48,4+FONT_W,RED_START_Y);
    gui_window_add_layer(hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_trans_color(lyr_hdl,WHITE);
    gui_layer_subscribe_event(lyr_hdl,GUI_EVT_ON_CLICK,gui_event_on_click_red_down);

    lyr_hdl=gui_create_bmp_layerk(RES_BMP_47,200,RED_START_Y);
    gui_window_add_layer(hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_trans_color(lyr_hdl,WHITE);
    gui_layer_subscribe_event(lyr_hdl,GUI_EVT_ON_CLICK,gui_event_on_click_red_up);

    lyr_hdl=gui_create_bmp_layerk(RES_BMP_54,RED_START_X,RED_START_Y+10);
    gui_window_add_layer(hdl,lyr_hdl);
    gui_layer_set_size(lyr_hdl,0,30);
    gui_layer_subscribe_event(lyr_hdl,GUI_EVT_ON_CLICK,gui_event_on_click_red);

    lyr_hdl=gui_create_bmp_layerk(RES_BMP_55,RED_START_X+m_red_value,RED_START_Y);
    gui_window_add_layer(hdl,lyr_hdl);
    red_proc_lyr_hdl=lyr_hdl;

    //��
    lyr_hdl=gui_create_pixel_layer(4,GREEN_START_Y,FONT_W,FONT_H,GUI_PIX_TYPE_RGB1,((uint8_t*)wordsrgb)+72);
    gui_window_add_layer(hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    lyr_hdl=gui_create_bmp_layerk(RES_BMP_48,4+FONT_W,GREEN_START_Y);
    gui_window_add_layer(hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_trans_color(lyr_hdl,WHITE);
    gui_layer_subscribe_event(lyr_hdl,GUI_EVT_ON_CLICK,gui_event_on_click_green_down);

    lyr_hdl=gui_create_bmp_layerk(RES_BMP_47,200,GREEN_START_Y);
    gui_window_add_layer(hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_trans_color(lyr_hdl,WHITE);
    gui_layer_subscribe_event(lyr_hdl,GUI_EVT_ON_CLICK,gui_event_on_click_green_up);

    lyr_hdl=gui_create_bmp_layerk(RES_BMP_54,GREEN_START_X,GREEN_START_Y+10);
    gui_window_add_layer(hdl,lyr_hdl);
    gui_layer_set_size(lyr_hdl,0,30);
    gui_layer_subscribe_event(lyr_hdl,GUI_EVT_ON_CLICK,gui_event_on_click_green);

    lyr_hdl=gui_create_bmp_layerk(RES_BMP_55,GREEN_START_X+m_green_value,GREEN_START_Y);
    gui_window_add_layer(hdl,lyr_hdl);
    green_proc_lyr_hdl=lyr_hdl;

    //��
    lyr_hdl=gui_create_pixel_layer(4,BLUE_START_Y,FONT_W,FONT_H,GUI_PIX_TYPE_RGB1,((uint8_t*)wordsrgb)+72*2);
    gui_window_add_layer(hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    lyr_hdl=gui_create_bmp_layerk(RES_BMP_48,4+FONT_W,BLUE_START_Y);
    gui_window_add_layer(hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_trans_color(lyr_hdl,WHITE);
    gui_layer_subscribe_event(lyr_hdl,GUI_EVT_ON_CLICK,gui_event_on_click_blue_down);

    lyr_hdl=gui_create_bmp_layerk(RES_BMP_47,200,BLUE_START_Y);
    gui_window_add_layer(hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_trans_color(lyr_hdl,WHITE);
    gui_layer_subscribe_event(lyr_hdl,GUI_EVT_ON_CLICK,gui_event_on_click_blue_up);

    lyr_hdl=gui_create_bmp_layerk(RES_BMP_54,BLUE_START_X,BLUE_START_Y+10);
    gui_window_add_layer(hdl,lyr_hdl);
    gui_layer_set_size(lyr_hdl,0,30);
    gui_layer_subscribe_event(lyr_hdl,GUI_EVT_ON_CLICK,gui_event_on_click_blue);

    lyr_hdl=gui_create_bmp_layerk(RES_BMP_55,BLUE_START_X+m_blue_value,BLUE_START_Y);
    gui_window_add_layer(hdl,lyr_hdl);
    blue_proc_lyr_hdl=lyr_hdl;

    /*
     * ѡ������
     */
    for(int i=0;i<click_item_num;i++)
    {
        x=(GUI_RES_X-(select_item[i].words_len*FONT_W))/2;
        y=(i)*WINDOW_ALIGN_VAL+offset;

        gui_geometry_line_t line;
        line.points[0].x=x;
        line.points[0].y=y-5;//y+FONT_H+1;
        line.points[1].x=x+FONT_W*select_item[i].words_len;
        line.points[1].y=y-5;//y+FONT_H+1;

        //�ָ���
        lyr_hdl=gui_create_geometry_layer(GUI_GEOMETRY_LINE,&line,GEOM_FILL_OUT_LINE);
        gui_layer_set_front_color(lyr_hdl,GRAY);
        gui_window_add_layer(hdl,lyr_hdl);

        //����
        for(int j=0;j<select_item[i].words_len;j++)
        {
            lyr_hdl=gui_create_pixel_layer(x+FONT_W*j,y,FONT_W,FONT_H,GUI_PIX_TYPE_RGB1,
                    (void*)(select_item[i].words_data+FONT_W*FONT_W*j/8));
            gui_window_add_layer(hdl,lyr_hdl);
            gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
            gui_layer_set_front_color(lyr_hdl,WHITE);

            //���õ���¼�
            if(j==0)
            {
                //gui_layer_set_size(lyr_hdl,FONT_W*select_item[i].words_len,FONT_H);//����ͼ�㲻�����ÿ��
                //gui_layer_subscribe_event(lyr_hdl,GUI_EVT_ON_CLICK,gui_event_on_click);
                m_click_item[i].x=x;
                m_click_item[i].y=y;
                m_click_item[i].w=FONT_W*select_item[i].words_len;
                m_click_item[i].h=FONT_H;
                m_click_item[i].hdl=lyr_hdl;
            }
        }

        num++;
    }

    gui_layer_set_position(select_lyr_hdl,0,m_click_item[selected_item].y);

    if(win_height>=GUI_RES_Y)
        win_height-=GUI_RES_Y;
    else
        win_height=0;

    /* ��ʾ���� */
    gui_window_show(hdl);
//    gui_window_show_pre(hdl);

}

static int _main(int argc, char *argv[])
{
    os_state_subscribe(SYS_STA_APP_ON_EXIT, sys_sta_event_cb);

    setting_window_init();

    APP_SETTING_LOG("start app=%d(setting)\r\n", SYS_APP_ID_MENU);
    return 0;
}

/*---------------------------------------------------------------------------------------------------*/

static const app_inst_info_t _app_info=
{
    .app_id=SYS_APP_ID_LIGHT,
    .file_id=APP_INST_PKG_FILE_ID,
    .type=APP_TYPE_TOOL,
    .reserved={0,0},

    .elf_inrom_addr=INVALID_ELF_INROM_ADDR,
    .elf_inrom_size=0,

    .elf_inram_addr=INVALID_ELF_INRAM_ADDR,
    .main=_main,
};

static os_app_node_t app_node =
{
    .list = LIST_INIT(app_node.list),
    .info=&_app_info,
    .priority = APP_PRIORITY_LOWEST,
};

void app_light_init(void)
{
    os_app_add(&app_node);
}
