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
//    lcd_set_fb((uint32_t)fb,w,h);
    lcd_set_window(x, x+w-1, y, y+h-1);
    lcd_flush(wait_to_finish);
}


void gui_init()
{
    memset(gui_fb,0,sizeof(gui_fb));

    lcd_hw_init(GUI_RES_X,GUI_RES_Y);
    lcd_set_fb((uint32_t)gui_fb,GUI_RES_X,GUI_RES_Y);
    gui_flush(gui_fb,0,0,GUI_RES_X,GUI_RES_Y,true);

}

void gui_reinit()
{
    //TODO:打开 LCD控制器
    lcd_on();
}

void gui_deinit()
{
    //TODO:关闭 LCD控制器

    lcd_off();
}




void gui_self_test()
{

    for (;;)
    {
        //不分屏：刷全屏颜色测试
#if 0

        for(int i=0;i<GUI_RES_Y;i++)
        {
            for(int j=0;j<GUI_RES_X;j++)
            {
                gui_fb[i*GUI_RES_X+j]=((YELLOW>>8)&0xff)|((YELLOW&0xff)<<8);
            }
        }
        gui_flush(gui_fb,0,0,GUI_RES_X,GUI_RES_Y,true);
        delay_ms(500);

        for(int i=0;i<GUI_RES_Y;i++)
        {
            for(int j=0;j<GUI_RES_X;j++)
            {
                gui_fb[i*GUI_RES_X+j]=((BLUE>>8)&0xff)|((BLUE&0xff)<<8);
            }
        }
        gui_flush(gui_fb,0,0,GUI_RES_X,GUI_RES_Y,true);
        delay_ms(500);

        continue;
#endif



#if 1

        #define RES_NUM 3

        for(int i=0;i<RES_NUM;i++)
        {
            jacefs_read(0,1,
                    (uint8_t*)gui_fb,
                    GUI_RES_X*GUI_RES_Y*2,
                    PKG_RES_DESC_OFFSET+RES_NUM*12+i*(GUI_RES_X*GUI_RES_Y*2+4)+4
                    );

            gui_flush(gui_fb,0,0,GUI_RES_X,GUI_RES_Y,true);
            delay_ms(3000);
        }

#endif

    }

}

