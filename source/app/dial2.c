
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
#include "app_mgr_task.h"

static const unsigned char cn_word1[][72]=
{
//U+706F(灯)
0x40,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x7C,0x80,0xF8,0x03,0x80,
0x00,0x02,0x80,0x04,0x02,0x84,0x04,0x02,0x88,0x02,0x02,0x50,0x01,0x02,0xF0,0x00,
0x02,0xC0,0x00,0x02,0x40,0x00,0x02,0x40,0x00,0x02,0xC0,0x00,0x02,0x40,0x01,0x02,
0x20,0x06,0x02,0x20,0x0C,0x02,0x10,0x08,0x02,0x10,0x00,0x02,0x08,0x40,0x02,0x04,
0x80,0x02,0x04,0x00,0x01,0x00,0x00,0x00,
//U+5149(光)
0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x10,0x00,0x00,0x10,0x04,0x40,0x10,0x04,0x80,
0x10,0x02,0x00,0x11,0x02,0x00,0x13,0x01,0x00,0x92,0x00,0x00,0x10,0x00,0x00,0x90,
0x3F,0xF8,0x7F,0x00,0x00,0x44,0x00,0x00,0x24,0x00,0x00,0x24,0x00,0x00,0x22,0x00,
0x00,0x22,0x00,0x00,0x21,0x20,0x00,0x21,0x20,0x80,0x20,0x20,0x40,0x40,0x20,0x30,
0xC0,0x7F,0x08,0x00,0x00,0x00,0x00,0x00,
//U+63A7(控)
0x00,0x40,0x00,0x40,0x80,0x00,0x80,0x80,0x01,0x40,0x00,0x01,0x40,0x08,0x01,0x40,
0x08,0x7E,0x40,0xF8,0x21,0x40,0x0B,0x10,0xF8,0x04,0x12,0x40,0x44,0x04,0x40,0x22,
0x18,0x40,0x11,0x30,0xC0,0x08,0x00,0x70,0x04,0x08,0x5C,0xC0,0x1F,0x40,0xB8,0x00,
0x40,0x80,0x00,0x40,0x80,0x00,0x40,0x80,0x00,0x40,0x80,0x00,0x50,0x80,0x78,0x60,
0xFE,0x07,0x40,0x00,0x00,0x00,0x00,0x00,
//U+5236(制)
0x00,0x00,0x00,0x00,0x02,0x10,0x40,0x02,0x30,0x40,0x02,0x10,0x40,0x02,0x10,0x20,
0x32,0x11,0xE0,0x0F,0x12,0x10,0x02,0x12,0x08,0x42,0x12,0x80,0x3F,0x12,0x7C,0x02,
0x12,0x00,0x02,0x12,0x10,0x7A,0x12,0xE0,0x27,0x12,0x20,0x22,0x13,0x20,0x22,0x12,
0x20,0x22,0x10,0x20,0x2A,0x10,0x30,0x12,0x10,0x20,0x02,0x10,0x00,0x02,0x16,0x00,
0x02,0x18,0x00,0x02,0x10,0x00,0x02,0x00,
};
static const unsigned char cn_word2[][72]=
{
//U+6E38(游)
0x00,0x00,0x01,0x00,0x00,0x01,0x00,0x02,0x01,0x10,0x04,0x01,0x60,0x84,0x00,0x40,
0x80,0x3C,0x00,0xB0,0x03,0x80,0x4F,0x00,0x00,0x24,0x08,0x8C,0xE4,0x0F,0x58,0x12,
0x04,0x50,0x9E,0x02,0x40,0x12,0x03,0x40,0x12,0x02,0x20,0x12,0x7E,0x20,0xF1,0x03,
0x20,0x11,0x02,0x20,0x09,0x02,0xAC,0x08,0x02,0x98,0x09,0x02,0x50,0x4E,0x02,0x20,
0x84,0x03,0x20,0x00,0x03,0x00,0x00,0x00,
//U+620F(戏)
0x00,0x00,0x00,0x00,0x60,0x00,0x00,0x40,0x00,0x00,0x40,0x03,0x00,0x44,0x06,0x80,
0x47,0x04,0x70,0x42,0x00,0x00,0x42,0x00,0x00,0x42,0x3F,0x20,0xFE,0x00,0x40,0x41,
0x00,0x40,0x41,0x08,0x80,0x80,0x04,0x80,0x81,0x02,0x40,0x83,0x01,0x60,0x82,0x00,
0x20,0x42,0x01,0x10,0x30,0x21,0x08,0x08,0x22,0x04,0x06,0x22,0x00,0x00,0x24,0x00,
0x00,0x38,0x00,0x00,0x30,0x00,0x00,0x00,
//U+4E2D(中)
0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x10,0x00,0x00,0x10,0x00,0x00,0x10,0x00,0x00,
0x10,0x00,0x20,0x10,0x1C,0xE0,0xFF,0x0B,0x20,0x10,0x08,0x20,0x10,0x08,0x20,0x10,
0x08,0x20,0x10,0x08,0x20,0xD0,0x0F,0xE0,0x3F,0x00,0x20,0x10,0x00,0x00,0x10,0x00,
0x00,0x10,0x00,0x00,0x10,0x00,0x00,0x10,0x00,0x00,0x10,0x00,0x00,0x10,0x00,0x00,
0x10,0x00,0x00,0x10,0x00,0x00,0x00,0x00,
//U+5FC3(心)
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x08,0x00,0x00,
0x10,0x00,0x00,0x30,0x00,0x00,0x20,0x04,0x00,0x20,0x08,0xA0,0x00,0x10,0x20,0x01,
0x60,0x10,0x01,0x40,0x10,0x01,0x00,0x10,0x02,0x00,0x10,0x02,0x00,0x08,0x04,0x02,
0x0C,0x04,0x04,0x00,0x08,0x04,0x00,0x10,0x04,0x00,0xE0,0x04,0x00,0x80,0x0F,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};
static const unsigned char cn_word3[][72]=
{
//U+97F3(音)
0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x10,0x00,0x00,0x10,0x00,0x00,0x00,0x06,0xC0,
0xFF,0x01,0x00,0x81,0x01,0x00,0x82,0x00,0x00,0x46,0x00,0x00,0x22,0x30,0x80,0xFF,
0x0F,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0xF9,0x03,0x00,0x07,0x01,0x00,0x01,0x01,
0x00,0x01,0x01,0x00,0x7F,0x01,0x00,0x01,0x01,0x00,0x01,0x01,0x00,0x01,0x01,0x00,
0xFF,0x01,0x00,0x01,0x01,0x00,0x00,0x00,
//U+4E50(乐)
0x00,0x00,0x00,0x00,0x00,0x03,0x00,0xC0,0x01,0x80,0x38,0x00,0x80,0x07,0x00,0x80,
0x10,0x00,0x80,0x30,0x00,0x40,0x10,0x00,0x40,0x10,0x00,0x40,0x10,0x08,0x40,0xF8,
0x0F,0xE0,0x17,0x00,0x00,0x10,0x00,0x00,0x11,0x00,0x00,0x11,0x01,0x80,0x11,0x06,
0xC0,0x10,0x08,0x20,0x10,0x30,0x10,0x10,0x20,0x08,0x10,0x00,0x04,0x16,0x00,0x00,
0x18,0x00,0x00,0x10,0x00,0x00,0x00,0x00,
//U+64AD(播)
0x00,0x00,0x00,0x40,0x00,0x04,0x40,0x80,0x03,0x40,0x70,0x02,0x40,0x4C,0x04,0x40,
0x48,0x02,0x40,0x50,0x02,0xE0,0xD3,0x3F,0x58,0x7E,0x01,0x40,0x60,0x02,0x40,0x52,
0x04,0xC0,0x49,0x08,0x60,0x4C,0x10,0x58,0x46,0x38,0x4C,0xF5,0x0F,0x40,0x4C,0x04,
0x40,0x44,0x04,0x40,0xC4,0x07,0x40,0x7C,0x04,0x40,0x44,0x04,0x50,0x44,0x04,0x60,
0xFC,0x07,0x40,0x00,0x04,0x00,0x00,0x00,
//U+653E(放)
0x00,0x00,0x00,0x00,0x80,0x00,0x80,0x80,0x01,0x00,0x81,0x00,0x00,0x83,0x00,0x00,
0x81,0x00,0x00,0x80,0x00,0x00,0x5E,0x30,0xFC,0xC1,0x1F,0x80,0x40,0x04,0x80,0x40,
0x04,0x40,0x28,0x02,0xC0,0x6F,0x02,0x40,0x54,0x02,0x40,0x94,0x01,0x40,0x04,0x01,
0x20,0x04,0x01,0x20,0x84,0x02,0x10,0x42,0x02,0x50,0x22,0x04,0x88,0x12,0x04,0x08,
0x09,0x08,0x04,0x07,0x78,0x00,0x00,0x00,
};
static const unsigned char cn_word4[][72]=
{
//U+7CFB(系)
0x00,0x00,0x00,0x00,0x00,0x07,0x00,0xE0,0x00,0x00,0x1E,0x00,0xE0,0x19,0x00,0x00,
0x0C,0x01,0x00,0x84,0x01,0x00,0xC2,0x00,0x00,0x3F,0x00,0x80,0x11,0x00,0x00,0x08,
0x01,0x00,0x04,0x02,0x00,0xE3,0x0D,0xC0,0x1F,0x08,0x00,0x10,0x00,0x00,0x10,0x00,
0x00,0x93,0x01,0x80,0x11,0x06,0xC0,0x10,0x0C,0x20,0x10,0x10,0x10,0x12,0x00,0x08,
0x1C,0x00,0x00,0x10,0x00,0x00,0x00,0x00,
//U+7EDF(统)
0x00,0x00,0x00,0x00,0x40,0x00,0x80,0x80,0x00,0x40,0x80,0x00,0x40,0x80,0x00,0x20,
0x80,0x3F,0x20,0x7C,0x00,0x10,0x41,0x00,0x10,0x21,0x02,0xF8,0x10,0x04,0x88,0x10,
0x08,0x40,0xEC,0x1F,0x20,0x38,0x01,0x20,0x20,0x01,0xD0,0x21,0x01,0x38,0x20,0x01,
0x00,0x10,0x01,0x00,0x13,0x01,0xC0,0x08,0x21,0x38,0x08,0x21,0x08,0x04,0x21,0x00,
0x02,0x61,0x80,0x01,0x3E,0x00,0x00,0x00,
//U+8BBE(设)
0x00,0x00,0x00,0x20,0x10,0x00,0x40,0x10,0x03,0xC0,0xF0,0x01,0x80,0x10,0x01,0x00,
0x10,0x01,0x00,0x10,0x01,0x00,0x10,0x01,0xC0,0x08,0x02,0x7C,0x08,0x3E,0x40,0x04,
0x02,0x40,0x02,0x06,0x40,0xF8,0x03,0x40,0x00,0x02,0x40,0x18,0x01,0x40,0x10,0x01,
0x40,0xA2,0x00,0x40,0x41,0x00,0xC0,0xA0,0x00,0x60,0x10,0x01,0x00,0x08,0x02,0x00,
0x04,0x0C,0x00,0x03,0x38,0x80,0x00,0x00,
//U+7F6E(置)
0x00,0x00,0x00,0x20,0x00,0x08,0xE0,0xFF,0x0F,0x20,0x44,0x08,0x20,0x44,0x08,0x20,
0xF4,0x0F,0xE0,0x0F,0x00,0x00,0x10,0x00,0x00,0x90,0x1F,0xF0,0x7F,0x00,0x00,0x10,
0x00,0x80,0xF0,0x03,0x80,0x0F,0x01,0x80,0x00,0x01,0x80,0xFF,0x01,0x80,0x00,0x01,
0x80,0x78,0x01,0x00,0x87,0x01,0x00,0x41,0x01,0x00,0xBF,0x01,0x00,0x01,0x01,0x00,
0xFF,0x7F,0xFC,0x00,0x00,0x00,0x00,0x00,
};
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

static void light_click_cb(void *hdl,uint8_t *pdata, uint16_t size)
{
    switch_app(SYS_APP_ID_LIGHT);

}
static void setting_click_cb(void *hdl,uint8_t *pdata, uint16_t size)
{
    switch_app(SYS_APP_ID_SETTING);
}
static void music_click_cb(void *hdl,uint8_t *pdata, uint16_t size)
{
    switch_app(SYS_APP_ID_MUSIC);
}
static void game_click_cb(void *hdl,uint8_t *pdata, uint16_t size)
{
    switch_app(SYS_APP_ID_GAME);
}
static void _window_init()
{
#define PINK (RGB888_TO_RGB565(255<<16|143<<8|139<<0))

    gui_window_hdl_t win_hdl;

//    struct tm now_time=os_get_now_time();

    /* 窗口 */
    win_hdl=gui_window_create_maximum(0,0);
    gui_window_set_bg_color(win_hdl,((PINK>>8)&0xff)|((PINK&0xff)<<8) );

//    gui_window_subscribe_event(win_hdl,GUI_EVT_ON_CLICK,on_click_call_back);
//    gui_window_subscribe_event(win_hdl,GUI_EVT_ON_SLIP_LEFT,on_click_call_back);
//    gui_window_subscribe_event(win_hdl,GUI_EVT_ON_SLIP_RIGHT,on_click_call_back);

    /*
         *  添加图层
     */
    gui_layer_hdl_t lyr_hdl;


    //灯光控制
    lyr_hdl=gui_create_bmp_layerk(RES_BMP_52,10,14);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_trans_color(lyr_hdl,WHITE);
    gui_layer_subscribe_event(lyr_hdl,GUI_EVT_ON_CLICK,light_click_cb);

    uint16_t x,y;
    x=10;
    y=120;
    lyr_hdl=gui_create_pixel_layer(x,y,24,24,GUI_PIX_TYPE_RGB1,cn_word1);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    lyr_hdl=gui_create_pixel_layer(x+24*1,y,24,24,GUI_PIX_TYPE_RGB1,&cn_word1[1]);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    lyr_hdl=gui_create_pixel_layer(x+24*2,y,24,24,GUI_PIX_TYPE_RGB1,&cn_word1[2]);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    lyr_hdl=gui_create_pixel_layer(x+24*3,y,24,24,GUI_PIX_TYPE_RGB1,&cn_word1[3]);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    //设置
    lyr_hdl=gui_create_bmp_layerk(RES_BMP_53,120,14);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_trans_color(lyr_hdl,WHITE);
    gui_layer_subscribe_event(lyr_hdl,GUI_EVT_ON_CLICK,setting_click_cb);

    x=130;
    y=120;
    lyr_hdl=gui_create_pixel_layer(x,y,24,24,GUI_PIX_TYPE_RGB1,cn_word4);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    lyr_hdl=gui_create_pixel_layer(x+24*1,y,24,24,GUI_PIX_TYPE_RGB1,&cn_word4[1]);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    lyr_hdl=gui_create_pixel_layer(x+24*2,y,24,24,GUI_PIX_TYPE_RGB1,&cn_word4[2]);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    lyr_hdl=gui_create_pixel_layer(x+24*3,y,24,24,GUI_PIX_TYPE_RGB1,&cn_word4[3]);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    //音乐
    lyr_hdl=gui_create_bmp_layerk(RES_BMP_50,10,178);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_trans_color(lyr_hdl,WHITE);
    gui_layer_subscribe_event(lyr_hdl,GUI_EVT_ON_CLICK,music_click_cb);

    x=10;
    y=280;
    lyr_hdl=gui_create_pixel_layer(x,y,24,24,GUI_PIX_TYPE_RGB1,cn_word3);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    lyr_hdl=gui_create_pixel_layer(x+24*1,y,24,24,GUI_PIX_TYPE_RGB1,&cn_word3[1]);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    lyr_hdl=gui_create_pixel_layer(x+24*2,y,24,24,GUI_PIX_TYPE_RGB1,&cn_word3[2]);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    lyr_hdl=gui_create_pixel_layer(x+24*3,y,24,24,GUI_PIX_TYPE_RGB1,&cn_word3[3]);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    //游戏
    lyr_hdl=gui_create_bmp_layerk(RES_BMP_51,130,178);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_trans_color(lyr_hdl,WHITE);
    gui_layer_subscribe_event(lyr_hdl,GUI_EVT_ON_CLICK,game_click_cb);

    x=130;
    y=280;
    lyr_hdl=gui_create_pixel_layer(x,y,24,24,GUI_PIX_TYPE_RGB1,cn_word2);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    lyr_hdl=gui_create_pixel_layer(x+24*1,y,24,24,GUI_PIX_TYPE_RGB1,&cn_word2[1]);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    lyr_hdl=gui_create_pixel_layer(x+24*2,y,24,24,GUI_PIX_TYPE_RGB1,&cn_word2[2]);
    gui_window_add_layer(win_hdl,lyr_hdl);
    gui_layer_set_transparent(lyr_hdl,LAYER_ENABLE_TRANSP);
    gui_layer_set_front_color(lyr_hdl,WHITE);

    lyr_hdl=gui_create_pixel_layer(x+24*3,y,24,24,GUI_PIX_TYPE_RGB1,&cn_word2[3]);
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
    .app_id=SYS_APP_ID_DEFAULT_DIAL2,
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

void dial2_init(void)
{
    os_app_add(&app_node);
}

