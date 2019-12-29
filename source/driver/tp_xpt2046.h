/*
 * tp_xpt2046.h
 *
 *  Created on: 2019Äê12ÔÂ23ÈÕ
 *      Author: jace
 */

#ifndef DRIVER_TP_XPT2046_H_
#define DRIVER_TP_XPT2046_H_

bool tp_int_get();
void tp_init();
uint8_t tp_read_xy(uint16_t *x,uint16_t *y);

#endif /* DRIVER_TP_XPT2046_H_ */
