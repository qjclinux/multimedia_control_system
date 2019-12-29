
/*
 * app_default_dial1.c
 *
 *  Created on: 2019年12月28日
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

static gui_layer_hdl_t sec_lyr_hdl=NULL,min_lyr_hdl=NULL,hour_lyr_hdl=NULL;
static gui_layer_hdl_t date_hdl[2]={NULL,NULL},
                        date_hdl1[2]={NULL,NULL};
static gui_layer_hdl_t week_hdl=NULL;


#define PT_OFFSET_Y (20)
typedef struct{
    uint16_t center_x,center_y; /* 旋转中心，相对图层起始点的值 */
    uint16_t res_id;
}dail_pointer_t;

#if 1
static const dail_pointer_t dail_pointer[3][15]=
{
    {
        {4,107,RES_BMP_31},
        {4,107,RES_BMP_38},
        {6,105,RES_BMP_39},
        {8,103,RES_BMP_40},
        {10,98,RES_BMP_41},
        {12,93,RES_BMP_42},
        {13,87,RES_BMP_43},
        {15,80,RES_BMP_44},
        {16,72,RES_BMP_45},
        {17,64,RES_BMP_32},
        {18,54,RES_BMP_33},
        {19,44,RES_BMP_34},
        {20,34,RES_BMP_35},
        {20,23,RES_BMP_36},
        {20,12,RES_BMP_37},
    },
    {
//        {5,100,RES_BMP_186},
//        {7,100,RES_BMP_193},
//        {10,97,RES_BMP_194},
//        {11,95,RES_BMP_195},
//        {13,92,RES_BMP_196},
//        {14,87,RES_BMP_197},
//        {17,82,RES_BMP_198},
//        {18,76,RES_BMP_199},
//        {19,69,RES_BMP_200},
//        {20,62,RES_BMP_187},
//        {20,53,RES_BMP_188},
//        {21,44,RES_BMP_189},

            {21,35,RES_BMP_4},
            {20,26,RES_BMP_5},
            {20,16,RES_BMP_6},

            {21,35,RES_BMP_4},
            {20,26,RES_BMP_5},
            {20,16,RES_BMP_6},

            {21,35,RES_BMP_4},
            {20,26,RES_BMP_5},
            {20,16,RES_BMP_6},

            {21,35,RES_BMP_4},
            {20,26,RES_BMP_5},
            {20,16,RES_BMP_6},

            {21,35,RES_BMP_4},//12  13   14
            {20,26,RES_BMP_5},
            {20,16,RES_BMP_6},
    },
    {
        {5,71,RES_BMP_7},//0  1   2
        {7,72,RES_BMP_8},
        {9,71,RES_BMP_9},

        {5,71,RES_BMP_7},
        {7,72,RES_BMP_8},
        {9,71,RES_BMP_9},

        {5,71,RES_BMP_7},
        {7,72,RES_BMP_8},
        {9,71,RES_BMP_9},

        {5,71,RES_BMP_7},
        {7,72,RES_BMP_8},
        {9,71,RES_BMP_9},

        {5,71,RES_BMP_7},
        {7,72,RES_BMP_8},
        {9,71,RES_BMP_9},

//        {10,70,RES_BMP_240},
//        {13,67,RES_BMP_241},
//        {14,64,RES_BMP_242},
//        {16,60,RES_BMP_243},
//        {17,56,RES_BMP_244},
//        {18,51,RES_BMP_245},
//        {19,45,RES_BMP_232},
//        {19,40,RES_BMP_233},
//        {20,34,RES_BMP_234},
//        {19,27,RES_BMP_235},
//        {19,20,RES_BMP_236},
//        {19,13,RES_BMP_237},
    }
};

static const uint16_t date_num[]=
{
    RES_BMP_17  ,
    RES_BMP_18  ,
    RES_BMP_19  ,
    RES_BMP_20  ,
    RES_BMP_21  ,
    RES_BMP_22  ,
    RES_BMP_23  ,
    RES_BMP_24  ,
    RES_BMP_25  ,
    RES_BMP_26  ,
};

static const uint16_t week_num[]=
{
    RES_BMP_16,
    RES_BMP_10,
    RES_BMP_12,
    RES_BMP_11,
    RES_BMP_15,
    RES_BMP_13,
    RES_BMP_14,
};

#endif

static inline void __draw(gui_layer_hdl_t lyr,int8_t val,const dail_pointer_t *pt)
{
    uint16_t pos_x,pos_y;
    int16_t angle;

    int8_t idx=val%15;
    angle=val/15*90;

    pos_x=120-pt[idx].center_x;
    pos_y=120+PT_OFFSET_Y-pt[idx].center_y;

    gui_layer_set_bmp(lyr,pt[idx].res_id,true);
    gui_layer_set_position(lyr,pos_x,pos_y);
    gui_layer_set_angle(lyr,angle,pt[idx].center_x,pt[idx].center_y);
}

static inline void pointer_draw(const struct tm *now_time)
{
//    __draw(sec_lyr_hdl,now_time->tm_sec,dail_pointer[0]);
//    __draw(min_lyr_hdl,now_time->tm_sec,dail_pointer[1]);
//    __draw(hour_lyr_hdl,now_time->tm_sec,dail_pointer[2]);

    if(sec_lyr_hdl)
    {
        __draw(sec_lyr_hdl,now_time->tm_sec,dail_pointer[0]);
    }

    if(min_lyr_hdl)
    {
        __draw(min_lyr_hdl,now_time->tm_min,dail_pointer[1]);
    }

    if(hour_lyr_hdl)
    {
        __draw(hour_lyr_hdl,(now_time->tm_hour%12)*5+now_time->tm_min/12,dail_pointer[2]);
    }

    if(sec_lyr_hdl || min_lyr_hdl || hour_lyr_hdl)
        gui_window_show(NULL);

}

static void on_click_call_back(void *hdl,uint8_t *pdata, uint16_t size)
{
    //TODO:进入灯光控制系统

}

static void sys_evt_call_back(uint32_t event,const void *pdata,uint16_t size)
{
    if (SYS_STA_SECOND_CHANGE & event)
    {
        struct tm *now_time=(struct tm *)pdata;

        pointer_draw(now_time);

        gui_layer_set_bmp(date_hdl[0],date_num[(now_time->tm_mon+1)/10%10],true);
        gui_layer_set_bmp(date_hdl[1],date_num[(now_time->tm_mon+1)%10],true);
        gui_layer_set_bmp(date_hdl1[0],date_num[now_time->tm_mday/10%10],true);
        gui_layer_set_bmp(date_hdl1[1],date_num[now_time->tm_mday%10],true);

        gui_layer_set_bmp(week_hdl,week_num[now_time->tm_wday],true);
    }

    //1S 刷新一次即可
    if(event&(SYS_STA_SECOND_CHANGE))
    {
        gui_window_show(NULL);
    }
}

static void _window_init()
{
    gui_window_hdl_t win_hdl;

    struct tm now_time=os_get_now_time();

    /* 窗口 */
    win_hdl=gui_window_create_maximum(0,0);

    gui_window_subscribe_event(win_hdl,GUI_EVT_ON_CLICK,on_click_call_back);
//    gui_window_subscribe_event(win_hdl,GUI_EVT_ON_SLIP_LEFT,on_click_call_back);
//    gui_window_subscribe_event(win_hdl,GUI_EVT_ON_SLIP_RIGHT,on_click_call_back);

    /*
         *  添加图层
     */
    gui_layer_hdl_t lyr_hdl;

    lyr_hdl=gui_create_bmp_layerk(RES_BMP_46,0,0);
    gui_window_add_layer(win_hdl,lyr_hdl);

    lyr_hdl=gui_create_bmp_layerk(RES_BMP_0,0,40+PT_OFFSET_Y);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_trans_color(lyr_hdl,BLACK);

    lyr_hdl=gui_create_bmp_layerk(RES_BMP_1,40,0+PT_OFFSET_Y);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_trans_color(lyr_hdl,BLACK);

    lyr_hdl=gui_create_bmp_layerk(RES_BMP_2,200,40+PT_OFFSET_Y);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_trans_color(lyr_hdl,BLACK);

    lyr_hdl=gui_create_bmp_layerk(RES_BMP_3,40,200+PT_OFFSET_Y);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_trans_color(lyr_hdl,BLACK);

    //日期+星期
#define DATE_X 80
#define DATE_Y 266
    lyr_hdl=gui_create_bmp_layerk(date_num[(now_time.tm_mon+1)/10%10],DATE_X,DATE_Y);
    gui_window_add_layer(win_hdl,lyr_hdl);
    date_hdl[0]=lyr_hdl;
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_trans_color(lyr_hdl,BLACK);
    lyr_hdl=gui_create_bmp_layerk(date_num[(now_time.tm_mon+1)%10],DATE_X+9*1,DATE_Y);
    gui_window_add_layer(win_hdl,lyr_hdl);
    date_hdl[1]=lyr_hdl;
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_trans_color(lyr_hdl,BLACK);
    lyr_hdl=gui_create_bmp_layerk(RES_BMP_28,DATE_X+9*2,DATE_Y);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_trans_color(lyr_hdl,BLACK);
    lyr_hdl=gui_create_bmp_layerk(date_num[now_time.tm_mday/10%10],DATE_X+9*3,DATE_Y);
    gui_window_add_layer(win_hdl,lyr_hdl);
    date_hdl1[0]=lyr_hdl;
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_trans_color(lyr_hdl,BLACK);
    lyr_hdl=gui_create_bmp_layerk(date_num[now_time.tm_mday%10],DATE_X+9*4,DATE_Y);
    gui_window_add_layer(win_hdl,lyr_hdl);
    date_hdl1[1]=lyr_hdl;
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_trans_color(lyr_hdl,BLACK);

    lyr_hdl=gui_create_bmp_layerk(week_num[now_time.tm_wday],DATE_X+9*4+20,DATE_Y+2);
    gui_window_add_layer(win_hdl,lyr_hdl);
    week_hdl=lyr_hdl;
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_trans_color(lyr_hdl,BLACK);

    /*
         * 设：x0 y0 为旋转中心，x y 为图层位置 ，w h 为图像宽高
         * 则指针位置（图层位置）为：x=120-x0,y=120-y0
         * 即指针位置（图层位置）只与旋转中心有关
     *
     */
    //时间指针
    lyr_hdl=gui_create_bmp_layerk(dail_pointer[2][0].res_id,115,0+PT_OFFSET_Y);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_trans_color(lyr_hdl,0x0000);
    hour_lyr_hdl=lyr_hdl;

    lyr_hdl=gui_create_bmp_layerk(dail_pointer[1][0].res_id,115,0+PT_OFFSET_Y);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_trans_color(lyr_hdl,0x0000);
    min_lyr_hdl=lyr_hdl;

    lyr_hdl=gui_create_bmp_layerk(dail_pointer[0][0].res_id,116,0+PT_OFFSET_Y);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_trans_color(lyr_hdl,0x0000);
    sec_lyr_hdl=lyr_hdl;


    /* 显示窗口 */
//    gui_window_show(win_hdl);

    pointer_draw(&now_time);
}

static int dia1_main(int argc, char *argv[])
{

    _window_init();
    os_state_subscribe(SYS_STA_SECOND_CHANGE,sys_evt_call_back);
    return 0;
}

/*------------------------------------------------------------------------------*/

static const app_inst_info_t _app_info=
{
    .app_id=SYS_APP_ID_DEFAULT_DIAL,
    .file_id=APP_INST_PKG_FILE_ID,
    .type=APP_TYPE_DIAL,
    .reserved={0,0},

    .elf_inrom_addr=INVALID_ELF_INROM_ADDR,
    .elf_inrom_size=0,

    .elf_inram_addr=INVALID_ELF_INRAM_ADDR,
    .main=dia1_main,
};

static os_app_node_t app_node =
{
    .list = LIST_INIT(app_node.list),
    .info=&_app_info,
    .priority = APP_PRIORITY_LOWEST,
};

void dial1_init(void)
{
    os_app_add(&app_node);
}

