/*
 * gui_res.h
 *
 *  Created on: 2019Äê7ÔÂ11ÈÕ
 *      Author: jace
 */

#ifndef WEAR_SRC_GUI_GUI_RES_H_
#define WEAR_SRC_GUI_GUI_RES_H_

int gui_res_get_desc(uint16_t app_id,uint16_t file_id,uint16_t res_id,inst_pkg_res_desc_t *res_desc);
int gui_icon_get_desc(uint16_t app_id,uint16_t file_id,inst_pkg_res_desc_t *res_desc);
int gui_res_get_pict_wh(uint16_t app_id,uint16_t file_id,uint32_t offset,inst_pkg_res_picture_t *pict_wh);
int gui_res_get_data(gui_bmp_layer_attr_t *bmp,uint32_t res_offset,uint32_t read_size,uint8_t *rdata);
uint32_t gui_res_get_phy_addr(uint16_t app_id,uint16_t file_id,uint32_t offset);

#endif /* WEAR_SRC_GUI_GUI_RES_H_ */
