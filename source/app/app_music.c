
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

#if 0
/*
 * 24*24 宋体，中文点阵
 * 一个文字
 */
extern const unsigned char cn_words[][72];

static void sys_evt_call_back(uint32_t event,const void *pdata,uint16_t size)
{
    if (SYS_STA_SECOND_CHANGE & event)
    {
//        struct tm *now_time=(struct tm *)pdata;

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

//    struct tm now_time=os_get_now_time();

    /* 窗口 */
    win_hdl=gui_window_create_maximum(0,0);
    gui_window_set_bg_color(win_hdl,BLUE );

//    gui_window_subscribe_event(win_hdl,GUI_EVT_ON_CLICK,on_click_call_back);
//    gui_window_subscribe_event(win_hdl,GUI_EVT_ON_SLIP_LEFT,on_click_call_back);
//    gui_window_subscribe_event(win_hdl,GUI_EVT_ON_SLIP_RIGHT,on_click_call_back);

    /*
         *  添加图层
     */
    gui_layer_hdl_t lyr_hdl;


    lyr_hdl=gui_create_bmp_layerk(RES_BMP_50,70,50);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_trans_color(lyr_hdl,WHITE);

#define WORD_X 20
#define WORD_Y  160
    lyr_hdl=gui_create_pixel_layer(WORD_X,WORD_Y,24,24,GUI_PIX_TYPE_RGB1,cn_words);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    lyr_hdl=gui_create_pixel_layer(WORD_X+24*1,WORD_Y,24,24,GUI_PIX_TYPE_RGB1,&cn_words[1]);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    lyr_hdl=gui_create_pixel_layer(WORD_X+24*2,WORD_Y,24,24,GUI_PIX_TYPE_RGB1,&cn_words[2]);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    lyr_hdl=gui_create_pixel_layer(WORD_X+24*3,WORD_Y,24,24,GUI_PIX_TYPE_RGB1,&cn_words[3]);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    lyr_hdl=gui_create_pixel_layer(WORD_X+24*4,WORD_Y,24,24,GUI_PIX_TYPE_RGB1,&cn_words[4]);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    lyr_hdl=gui_create_pixel_layer(WORD_X+24*5,WORD_Y,24,24,GUI_PIX_TYPE_RGB1,&cn_words[5]);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    lyr_hdl=gui_create_pixel_layer(WORD_X+24*6,WORD_Y,24,24,GUI_PIX_TYPE_RGB1,&cn_words[6]);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    lyr_hdl=gui_create_pixel_layer(WORD_X+24*7,WORD_Y,24,24,GUI_PIX_TYPE_RGB1,&cn_words[7]);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    /* 显示窗口 */
    gui_window_show(win_hdl);
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
    .app_id=SYS_APP_ID_MUSIC,
    .file_id=APP_INST_PKG_FILE_ID,
    .type=APP_TYPE_TOOL,
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

void app_music_init(void)
{
    os_app_add(&app_node);
}

#endif
