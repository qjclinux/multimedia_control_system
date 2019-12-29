/*
 * font_convert.h
 *
 *  Created on: 2019Äê8ÔÂ13ÈÕ
 *      Author: jace
 */

#ifndef WEAR_SRC_GUI_FONT_FONT_DATA_H_
#define WEAR_SRC_GUI_FONT_FONT_DATA_H_

#include "font_mgr.h"

#define FONT_ASCII_MIN (32)
#define FONT_ASCII_LIMIT (128)

#define IS_ASCII(x) ((x)<FONT_ASCII_LIMIT)

int font_gb2312_data(uint16_t gb2312,const font_info_t *font,uint8_t *data);
int font_ascii_data(uint16_t c,const font_info_t *font,uint8_t *data);

typedef int (*font_data_read_t)(uint16_t code,const font_info_t *font,uint8_t *data);

uint16_t font_gb2312_text_len(const char* text);
uint16_t font_gb2312_text_fill(const char* text,uint16_t *gb2312);


#endif /* WEAR_SRC_GUI_FONT_FONT_DATA_H_ */
