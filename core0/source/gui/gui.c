/*
 * gui.c
 *
 *  Created on: 2019年7月9日
 *      Author: jace
 */

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <gui.h>
#include <jacefs.h>
#include <res_type.h>
#include "sw_delay.h"
#include "project_cfg.h"


static uint16_t gui_fb[GUI_RES_Y*GUI_RES_X];

uint16_t *gui_get_back_fb()
{
    return gui_fb;
}

uint16_t *gui_get_front_fb()
{
    return gui_fb;
}



void gui_flush(uint16_t *fb,uint16_t x,uint16_t y,uint16_t w,uint16_t h,bool wait_to_finish)
{
#if !USE_CORE1_FLUSH_SCREEN
//    lcd_set_fb((uint32_t)fb,w,h);
    lcd_set_window(x, x+w-1, y, y+h-1);
    lcd_flush(wait_to_finish);
#else

    void notif_cor1_flush_screen();
    notif_cor1_flush_screen();
#endif
}


void gui_init()
{
    memset(gui_fb,0,sizeof(gui_fb));

#if !USE_CORE1_FLUSH_SCREEN
    lcd_hw_init(GUI_RES_X,GUI_RES_Y);
    lcd_set_fb((uint32_t)gui_fb,GUI_RES_X,GUI_RES_Y);
    gui_flush(gui_fb,0,0,GUI_RES_X,GUI_RES_Y,true);
#else
    void notif_cor1_flush_screen();
    notif_cor1_flush_screen();
#endif

}

void gui_reinit()
{
    //TODO:打开 LCD控制器
#if !USE_CORE1_FLUSH_SCREEN
    lcd_on();
#endif
}

void gui_deinit()
{
    //TODO:关闭 LCD控制器
#if !USE_CORE1_FLUSH_SCREEN
    lcd_off();
#endif
}

