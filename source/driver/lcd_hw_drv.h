/*
 * lcd_hw_drv.h
 *
 *  Created on: 2019Äê6ÔÂ25ÈÕ
 *      Author: jace
 */

#ifndef LCD_HW_INIT_H_
#define LCD_HW_INIT_H_

#define LCD_RES_X 240
#define LCD_RES_Y 320


void lcd_off();
void lcd_on();

void lcd_set_window(uint16_t start_x,uint16_t end_x,uint16_t start_y,uint16_t end_y);
void lcd_set_fb(uint32_t fb_addr,uint16_t resx,uint16_t resy);
void lcd_flush(bool);
void lcd_hw_init(uint16_t resx,uint16_t resy);


#endif /* LCD_HW_INIT_H_ */
