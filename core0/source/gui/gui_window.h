/*
 * gui_window.h
 *
 *  Created on: 2019年7月11日
 *      Author: jace
 */

#ifndef WEAR_SRC_GUI_GUI_WINDOW_H_
#define WEAR_SRC_GUI_GUI_WINDOW_H_

#include "gui_layer.h"

/* 窗口 */
typedef enum{
    GUI_EFFECT_NONE,                //直接覆盖      gui_fill_fb_replace
    GUI_EFFECT_CONVERT_LB2RT,       //从左下->右上刷     gui_fill_fb_lbottom_to_rtop
    GUI_EFFECT_CONVERT_RB2LT,       //从右下角->左上角刷       gui_fill_fb_rbottom_to_ltop
    GUI_EFFECT_CONVERT_LT2RB,       //从左上角->右下角刷       gui_fill_fb_ltop_to_rbottom
    GUI_EFFECT_CONVERT_RT2LB,       //从右上->左下刷     gui_fill_fb_rtop_to_lbottom

    GUI_EFFECT_CONVERT_L2R,     //从左->右刷     gui_fill_fb_left_to_right
    GUI_EFFECT_CONVERT_R2L,     //从右->左刷     gui_fill_fb_right_to_left
    GUI_EFFECT_CONVERT_U2D,     //从上->下刷     gui_fill_fb_up_to_down
    GUI_EFFECT_CONVERT_D2U,      //从下->上刷      gui_fill_fb_down_to_up

    GUI_EFFECT_ROLL_L2R,     //从左->右 滚动 刷
    GUI_EFFECT_ROLL_R2L,     //从右->左 滚动刷

    GUI_EFFECT_CONVERT_U2D_REVERSE,     //从上->下刷    的倒放效果
    GUI_EFFECT_CONVERT_D2U_REVERSE,      //从下->上刷    的倒放效果
    GUI_EFFECT_CONVERT_R2L_REVERSE,     //从右->左刷    的倒放效果
    GUI_EFFECT_CONVERT_L2R_REVERSE,      //从左->右刷    的倒放效果

}gui_window_effect_t;

typedef enum{
    GUI_DISABLE_SCROLL,
//    GUI_ENABLE_HOR_SCROLL,
//    GUI_ENABLE_VER_SCROLL,
    GUI_ENABLE_SCROLL
}gui_window_scroll_t;

typedef struct {
    uint16_t app_id;        /* 窗口所属APP */

    uint16_t x, y;          /* 当前显示的起始位置：通过改变该值以改变显示内容，显示范围 (x,y)~(x+RESX-1,y+RESY-1) */
//    uint16_t x1, y1;        /* fb1缓存的窗口内的起始位置，范围为 (x1,y1)~(x1+RESX-1,y1+RESY-1) */
//    uint16_t x2, y2;        /* fb2缓存的窗口内的起始位置，范围为 (x2,y2)~(x2+RESX-1,y2+RESY-1) */
    uint16_t w, h;          /* 窗口大小：窗口可以比显示区域大 */

    uint16_t bg_color;      /* 背景颜色 */
    gui_bmp_layer_attr_t *bg_bmp;/* 背景图片资源 :窗口背景图片默认左上对齐，只显示一张*/

    uint32_t scroll:4;      /* 滚动功能：如果窗口比显示区域大，那么使能滚动可以让窗口拖动 */
    uint32_t effect:4;      /* 显示效果（待定） */
    uint32_t loaded:1;      /* 是否加载完成 */
    uint32_t reserved:32-9;

    uint8_t layer_cnt;      /* 窗口所添加图层数 */
    gui_layer_hdl_t *layers;/* 图层数数据 */

    uint8_t event_cnt;      /* 窗口事件数 */
    gui_event_hdl_t *events;/* 窗口事件数据 */

}gui_window_t;

#define GUI_WINDOW_INVALID_POS (0xffff)

typedef void * gui_window_hdl_t;

gui_window_hdl_t gui_window_create(uint16_t x,uint16_t y,uint16_t w,uint16_t h);
gui_window_hdl_t gui_window_create_maximum(uint16_t x,uint16_t y);

int gui_window_set_position(gui_window_hdl_t hdl,uint16_t x,uint16_t y);
uint16_t gui_window_get_y(gui_window_hdl_t hdl);
int gui_window_set_size(gui_window_hdl_t hdl,uint16_t w,uint16_t h);
int gui_window_set_scroll(gui_window_hdl_t hdl,gui_window_scroll_t scroll );
int gui_window_set_bg_bmp_inner(gui_window_hdl_t hdl,uint16_t  res_id,uint16_t app_id);
int gui_window_set_bg_bmp(gui_window_hdl_t hdl,uint16_t  res_id);
int gui_window_set_bg_color(gui_window_hdl_t hdl,uint16_t  bg_color);
int gui_window_set_effect(gui_window_hdl_t hdl,gui_window_effect_t  effect);
int gui_window_set_loaded(gui_window_hdl_t hdl);
int gui_window_get_loaded(gui_window_hdl_t hdl);

int gui_window_destroy_current();
int gui_window_destroy_last();

int gui_window_add_layer(gui_window_hdl_t hdl,gui_layer_hdl_t layer);
int gui_window_subscribe_event(gui_window_hdl_t hdl,
    gui_event_type_t evt_type,gui_event_call_back_t cb);
int gui_window_dispatch_event(gui_window_hdl_t hdl,
    gui_event_type_t evt_type,uint8_t *pdata, uint16_t size);

void gui_display_close();
void gui_display_open();


void gui_window_show(gui_window_hdl_t hdl);
void gui_window_show_pre(gui_window_hdl_t hdl);
void gui_window_show_mix(uint16_t top_pix,gui_window_effect_t effect);



#endif /* WEAR_SRC_GUI_GUI_WINDOW_H_ */
