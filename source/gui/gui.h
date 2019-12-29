/*
 * gui.h
 *
 *  Created on: 2019年7月9日
 *      Author: jace
 */

#ifndef _GUI_H_
#define _GUI_H_

#include "lcd_hw_drv.h"

#define GUI_RES_X LCD_RES_X
#define GUI_RES_Y LCD_RES_Y

#define GUI_BYTES_PER_PIX   (2)

/*
 * 刷屏 buf 分屏
 */
#if GUI_RES_Y>240
    #define GUI_BUF_DIV (1)//(4)//
#else
    #define GUI_BUF_DIV (1)
#endif
#define GUI_RES_Y_DIV (GUI_RES_Y/GUI_BUF_DIV)

#define GUI_BUF_NEED_DIV (GUI_BUF_DIV>1)

//RGB565颜色
//#define    WHITE   0xffff
//#define    RED     0xf800
//#define    GREEN   0x07e0
//#define    BLUE    0x001f
//#define    YELLOW  0xffe0
//#define    MAGENTA 0xf81f
//#define    CYAN    0x07ff
//#define    BLACK   0x0000
//#define    GREY    0X1041//0xf7de
//#define    GREYA   0X0082
//#define    GRAY   0X18C3 //

//高低字节相反
#define    WHITE   0xffff
#define    RED     0x00f8
#define    GREEN   0xe007
#define    BLUE    0x1f00
#define    YELLOW  0xe0ff
#define    MAGENTA 0x1ff8
#define    CYAN    0xff07
#define    BLACK   0x0000
#define    GREY    0X4110//0xf7de
#define    GREYA   0X8200
#define    GRAY   0XC318 //

//特殊颜色
#define TRANSP_COLOR    (BLACK)
#define FRONT_COLOR     (WHITE)
#define BACK_COLOR      (BLACK)
#define INVALID_COLOR   (BLACK+1)

uint16_t *gui_get_back_fb();
uint16_t *gui_get_front_fb();

void gui_flush(uint16_t *fb,uint16_t x,uint16_t y,uint16_t w,uint16_t h,bool wait_to_finish);
void gui_init(void);
void gui_reinit();
void gui_deinit();
void gui_self_test();

#endif /* _GUI_H_ */
