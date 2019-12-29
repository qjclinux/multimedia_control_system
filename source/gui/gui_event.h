/*
 * gui_event.h
 *
 *  Created on: 2019��7��11��
 *      Author: jace
 */

#ifndef WEAR_SRC_GUI_GUI_EVENT_H_
#define WEAR_SRC_GUI_GUI_EVENT_H_

typedef void (*gui_event_call_back_t)(void *hdl,uint8_t *pdata, uint16_t size);

/* �����¼� */
typedef enum{
    GUI_EVT_NONE,           /* ���¼� */
    GUI_EVT_ON_CLICK,       /* ���ڵ���¼� */
    GUI_EVT_ON_DOUBLE_CLICK,/* ����˫���¼� */
    GUI_EVT_ON_LONG_PRESS,  /* ���ڳ����¼� */
    GUI_EVT_ON_PRESSED,     /* ���ڰ����¼� */
    GUI_EVT_ON_RELEASED,    /* �����ɿ��¼� */
    GUI_EVT_ON_DRAG,        /* �����϶��¼� */
    GUI_EVT_ON_SLIP_LEFT,   /* �������¼� */
    GUI_EVT_ON_SLIP_RIGHT,  /* �����һ��¼� */
    GUI_EVT_ON_SLIP_UP,     /* �����ϻ��¼� */
    GUI_EVT_ON_SLIP_DOWN,   /* �����»��¼� */
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
    int16_t peak_x;/* ��ǰx��TP���µ�x0�Ĳ�ֵ */
    int16_t peak_y;/* ��ǰx��TP���µ�y0�Ĳ�ֵ */
}gui_evt_tp_data_t;


typedef void * gui_event_hdl_t;

#endif /* WEAR_SRC_GUI_GUI_EVENT_H_ */
