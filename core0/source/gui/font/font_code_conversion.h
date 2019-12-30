/*
 * font_code_conversion.h
 *
 *  Created on: 2019Äê11ÔÂ5ÈÕ
 *      Author: jace
 */

#ifndef WEAR_SRC_GUI_FONT_FONT_CODE_CONVERSION_H_
#define WEAR_SRC_GUI_FONT_FONT_CODE_CONVERSION_H_


uint16_t *utf8_to_unicode(const uint8_t *utf8,uint16_t utf8_bytes,uint16_t *unicode_len);
uint16_t *uicode_to_gb2312(const uint16_t *unicode,uint16_t unicode_len,uint16_t *gb2312_len);


#endif /* WEAR_SRC_GUI_FONT_FONT_CODE_CONVERSION_H_ */
