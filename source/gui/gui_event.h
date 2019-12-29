/*
 * gui_event.h
 *
 *  Created on: 2019年7月11日
 *      Author: jace
 */

#ifndef WEAR_SRC_GUI_GUI_EVENT_H_
#define WEAR_SRC_GUI_GUI_EVENT_H_

typedef void (*gui_event_call_back_t)(void *hdl,uint8_t *pdata, uint16_t size);

/* 窗口事件 */
typedef enum{
    GUI_EVT_NONE,           /* 无事件 */
    GUI_EVT_ON_CLICK,       /* 窗口点击事件 */
    GUI_EVT_ON_DOUBLE_CLICK,/* 窗口双击事件 */
    GUI_EVT_ON_LONG_PRESS,  /* 窗口长按事件 */
    GUI_EVT_ON_PRESSED,     /* 窗口按下事件 */
    GUI_EVT_ON_RELEASED,    /* 窗口松开事件 */
    GUI_EVT_ON_DRAG,        /* 窗口拖动事件 */
    GUI_EVT_ON_SLIP_LEFT,   /* 窗口左滑事件 */
    GUI_EVT_ON_SLIP_RIGHT,  /* 窗口右滑事件 */
    GUI_EVT_ON_SLIP_UP,     /* 窗口上滑事件 */
    GUI_EVT_ON_SLIP_DOWN,   /* 窗口下滑事件 */
}gui_event_type_t;

typedef struct {
    uint16_t type;      //ref @gui_event_type_t
    uint8_t reserved[2];
    gui_event_call_back_t cb;
}gui_event_t;


typedef struct{
    uint16_t evt_id;//ref @gui_event_type_t
    uint16_t x;
    uint16_t y;
    int16_t peak_x;/* 当前x离TP按下点x0的差值 */
    int16_t peak_y;/* 当前x离TP按下点y0的差值 */
}gui_evt_tp_data_t;


typedef void * gui_event_hdl_t;

#endif /* WEAR_SRC_GUI_GUI_EVENT_H_ */
