
/*
 * tp_task.c
 *
 *  Created on: 2019��6��26��
 *      Author: jace
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <time.h>
#include "osal.h"
#include "app_mgr.h"
#include "console.h"
#include "tp_xpt2046.h"
#include "app_mgr_task.h"
#include "tp_xpt2046.h"
#include "remote.h"
#include "gui.h"
#include "gui_window.h"

#if 1
//#ifdef SUPPORT_TP

#define DEBUG_TP_TASK 0
#if DEBUG_TP_TASK
    #define TP_TASK_LOG(fmt,args...) do{\
            /*printf("%s,%s(),%d:" ,__FILE__, __FUNCTION__,__LINE__);*/\
            os_printk("%s(),%d:" , __FUNCTION__,__LINE__);\
            os_printk(fmt,##args);\
        }while(0)
    #define TP_TASK_INFO(fmt,args...)        do{\
            os_printk(fmt,##args);\
        }while(0)
#else
    #define TP_TASK_LOG(fmt,args...)
    #define TP_TASK_INFO(fmt,args...)
#endif

static OS_TASK  tp_task_handle =NULL;
#define TP_EVENT_BIT (0x1<<3)

//static void tp_int_cb(void)
//{
//    if(tp_task_handle)
//    {
//        OS_TASK_NOTIFY_GIVE_FROM_ISR(tp_task_handle);
//    }
//}

//tp���͵�ϵͳ�¼�����
static void tp_evt_send(uint16_t evt,uint16_t x,uint16_t y,int16_t peak_x,int16_t peak_y)
{
    gui_evt_tp_data_t *tp_evt_data;
    sys_event_t tp_event;
    tp_event.event_type = SYS_EVENT_TP_CHG;

    tp_evt_data=(gui_evt_tp_data_t *)OS_MALLOC(sizeof(gui_evt_tp_data_t));
    if(tp_evt_data)
    {
        tp_evt_data->evt_id=evt;
        tp_evt_data->x=x;
        tp_evt_data->y=y;
        tp_evt_data->peak_x=peak_x;
        tp_evt_data->peak_y=peak_y;

        tp_event.event_data = (void*)tp_evt_data;
        if ( xQueueSend( m_sys_evt_queue,( void * ) &tp_event,0 ) != pdPASS) //����APP TASK
        {
            //TODO:fix this error.
        }
    }
}

void key1_handler()
{
    static bool last_is_press=false;
    if(!tp_int_get())
    {
        if(!last_is_press)
        {
            last_is_press=true;
            TP_TASK_LOG("press\r\n");

            //���ⷢ������
            IR_SEND_KEY(0x3);
        }
    }
    else
    {
        if(last_is_press)
        {
            TP_TASK_LOG("release\r\n");
            IR_SEND_KEY(0x4);
        }

        last_is_press=false;
    }
}


typedef struct {
    uint8_t is_press;
    uint16_t x;
    uint16_t y;
}tp_raw_data_t;

bool tp_read_data(tp_raw_data_t *tp_data)
{
#define TP_CALIB_START_X  300
#define TP_CALIB_END_X  3750
#define TP_CALIB_START_Y  440
#define TP_CALIB_END_Y  3800

    static uint16_t x,y;
    static uint16_t adc_x,adc_y;
    static bool last_is_press=false;
    bool now_is_press=false;

    //for(;;)
    {
        //OS_DELAY_MS(20);

        //key1_handler();

        now_is_press=tp_int_get();

        if(!now_is_press && !last_is_press)
            //continue;
            return 0;

        if(!last_is_press)
        {
//            PRINTF("tp press\r\n");
        }

        if(now_is_press)
        {
            tp_read_xy(&adc_x,&adc_y);
            //PRINTF("tp adc={%d,%d}\r\n",adc_x,adc_y);
        }
        else if(last_is_press)
        {
//            PRINTF("tp release\r\n");
        }
        last_is_press=now_is_press;

        /*
         * TP ADCתΪ��Ļ����
         */
        if(adc_x<TP_CALIB_START_X) adc_x=TP_CALIB_START_X;
        if(adc_x>TP_CALIB_END_X) adc_x=TP_CALIB_END_X;
        if(adc_y<TP_CALIB_START_Y) adc_y=TP_CALIB_START_Y;
        if(adc_y>TP_CALIB_END_Y) adc_y=TP_CALIB_END_Y;

        x=(adc_x-TP_CALIB_START_X)*GUI_RES_X/(TP_CALIB_END_X-TP_CALIB_START_X);
        y=(adc_y-TP_CALIB_START_Y)*GUI_RES_Y/(TP_CALIB_END_Y-TP_CALIB_START_Y);
//        PRINTF("tp xy={%d,%d}\r\n",x,y);
    }

    tp_data->is_press=now_is_press;
    tp_data->x=x;
    tp_data->y=y;

    return 1;
}

/*
 *  TP ����
 *
 */
void tp_task(void *params)
{
    tp_init();

#if 1
//    exti_regitser(TP_INT_PORT,TP_INT_PIN,HW_WKUP_PIN_STATE_LOW,tp_int_cb);

    tp_raw_data_t tp_data,*tp_raw_data;

    //tp���ݷ���
    int16_t last_x=0,last_y=0,
        diff_x=0,diff_y=0;
    bool last_is_release=true;
    bool wait_to_release=false;

    int16_t diff_total_x=0,     /* ���һ������ܲ�ֵ */
        diff_total_y=0,         /* ���»������ܲ�ֵ */
        diff_total_peak=0;      /* ��������ȡ���л���������ֵ */

    //����ʱ��
    uint32_t t1=0,t2=0,t3=0,t4=0;

    for (;;)
    {

        OS_DELAY_MS(20);
//        OS_TASK_NOTIFY_TAKE(pdTRUE,OS_TASK_NOTIFY_FOREVER);

        if(!tp_read_data(&tp_data))
        {
            continue;
        }

        tp_raw_data=&tp_data;


        //�ɿ��ֺ��ϱ��˶��״̬��������
        if(!tp_raw_data->is_press && last_is_release)
        {
            last_is_release=true;

//            TP_TASK_LOG("tp release +1\r\n");
            continue;
        }

        //��һ���ǳ����¼�����ôҪ�ȴ�TP�ɿ���Ŵ����µ�TP�¼�
        if(last_is_release)
        {
            wait_to_release=false;
        }
        else
        {
            if(wait_to_release)
            {
                if(!tp_raw_data->is_press)
                {
                    last_is_release=true;

                    //TODO:�������Ҫ�ϱ�TP�ɿ��¼�
                }
                TP_TASK_LOG("wait to release\r\n");
                continue;
            }
        }


        //ת���ɺ���Ļ����һ�µķ���
        tp_raw_data->y=GUI_RES_Y-tp_raw_data->y;

        //TP���£�ȡ��һ������ֵΪ��ʼֵ
        if(last_is_release)
        {
            t1=OS_GET_TICK_COUNT();

            last_x=tp_raw_data->x;
            last_y=tp_raw_data->y;
            last_is_release =false;
            diff_total_x=diff_total_y=diff_total_peak=0;

//            serial_digital_out(tp_raw_data->x,tp_raw_data->y,diff_x,diff_y);
            TP_TASK_LOG("tp press xy={%d,%d} diff={%d,%d} t={%d,%d} \r\n",
                    tp_raw_data->x,tp_raw_data->y,
                    diff_x,diff_y,
                    diff_total_x,diff_total_y);

           /*
            *  TP�����¼�
            */
            tp_evt_send(GUI_EVT_ON_PRESSED,tp_raw_data->x,tp_raw_data->y,diff_total_x,diff_total_y);

            t3=OS_GET_TICK_COUNT();
            continue;
        }

        //TP�ɿ����ж�TP����
        if(!tp_raw_data->is_press)
        {
            t2=OS_GET_TICK_COUNT();

            last_is_release=true;

//            serial_digital_out(tp_raw_data->x,tp_raw_data->y,diff_x,diff_y);
            TP_TASK_LOG("tp release xy={%d,%d} diff={%d,%d} t={%d,%d} peak=%d t=%d\r\n",
                    tp_raw_data->x,tp_raw_data->y,
                    diff_x,diff_y,
                    diff_total_x,diff_total_y,
                    diff_total_peak,
                    OS_TICKS_2_MS(t2-t1));

            /*
             *  TP�����¼�
             */

#define CLICK_DIFF_TIME (300)
#define CLICK_DIFF_XY (5)


            //��ֵС�� 5 ��ʱ��300ms֮�ڣ���Ϊ�ǵ���
            if(diff_total_peak<=CLICK_DIFF_XY && OS_TICKS_2_MS(t2-t1)<=CLICK_DIFF_TIME)
            {
                TP_TASK_LOG("tp click\r\n");
                tp_evt_send(GUI_EVT_ON_CLICK,tp_raw_data->x,tp_raw_data->y,diff_total_x,diff_total_y);
            }

           /*
            *  TP�����¼�
            */

#define SLIP_DIFF_XY (10)
            else if(diff_total_peak>SLIP_DIFF_XY)
            {
                if(diff_total_peak==abs(diff_total_x))
                {
                    if(diff_total_x<0)
                    {
                        TP_TASK_LOG("tp slip left\r\n");
                        tp_evt_send(GUI_EVT_ON_SLIP_LEFT,tp_raw_data->x,tp_raw_data->y,diff_total_x,diff_total_y);
                    }
                    else
                    {
                        TP_TASK_LOG("tp slip right\r\n");
                        tp_evt_send(GUI_EVT_ON_SLIP_RIGHT,tp_raw_data->x,tp_raw_data->y,diff_total_x,diff_total_y);
                    }
                }
                else if(diff_total_peak==abs(diff_total_y))
                {
                    if(diff_total_y<0)
                    {
                        TP_TASK_LOG("tp slip up\r\n");
                        tp_evt_send(GUI_EVT_ON_SLIP_UP,tp_raw_data->x,tp_raw_data->y,diff_total_x,diff_total_y);
                    }
                    else
                    {
                        TP_TASK_LOG("tp slip down\r\n");
                        tp_evt_send(GUI_EVT_ON_SLIP_DOWN,tp_raw_data->x,tp_raw_data->y,diff_total_x,diff_total_y);
                    }
                }
            }

            TP_TASK_LOG("\r\n");

           /*
            *  TP�ɿ��¼�
            */
            tp_evt_send(GUI_EVT_ON_RELEASED,tp_raw_data->x,tp_raw_data->y,diff_total_x,diff_total_y);

            continue;
        }

        //����������x,yͻ�䣬Ҫɸѡ����
        if(tp_raw_data->x>GUI_RES_X || tp_raw_data->y>GUI_RES_Y )
        {
            TP_TASK_LOG("tp data err!\r\n");
            continue;
        }

        diff_x=tp_raw_data->x-last_x;
        diff_y=tp_raw_data->y-last_y;

        diff_total_x+=diff_x;
        diff_total_y+=diff_y;

        //ȡ������������Զ��ֵ�����ڴ�����һȦ�ص�ԭ��������
        if(abs(diff_total_x)>diff_total_peak)
            diff_total_peak=abs(diff_total_x);
        if(abs(diff_total_y)>diff_total_peak)
            diff_total_peak=abs(diff_total_y);

        last_x=tp_raw_data->x;
        last_y=tp_raw_data->y;


#if DEBUG_TP_TASK
        if(diff_x!=0 || diff_y!=0)
        {
//            serial_digital_out(tp_raw_data->x,tp_raw_data->y,diff_x,diff_y);
            TP_TASK_LOG("tp drag xy={%d,%d} diff={%d,%d} t={%d,%d}\r\n",
                            tp_raw_data->x,tp_raw_data->y,
                            diff_x,diff_y,
                            diff_total_x,diff_total_y);
        }
#endif

        t4=OS_GET_TICK_COUNT();

        /*
         * TP�����¼�
         */
#define LP_DIFF_TIME (1200)
#define LP_DIFF_XY (10)

        //��ֵС�� 10 ��ʱ��LP_DIFF_TIME ms���ϣ���Ϊ�ǳ���
        if(diff_total_peak<=LP_DIFF_XY && OS_TICKS_2_MS(t4-t1)>=LP_DIFF_TIME)
        {
            TP_TASK_LOG("tp long press\r\n");
            tp_evt_send(GUI_EVT_ON_LONG_PRESS,tp_raw_data->x,tp_raw_data->y,diff_total_x,diff_total_y);
            wait_to_release=true;
            continue;
        }


        /*
         *  TP�϶��¼�
         *
         */

        //�ϱ��¼�ʱ��������̫��
        if(OS_TICKS_2_MS(t4-t3)>=10)
        {
            t3=t4;
            tp_evt_send(GUI_EVT_ON_DRAG,tp_raw_data->x,tp_raw_data->y,diff_total_x,diff_total_y);
        }

    }
#endif



}

void tp_task_init()
{
    OS_TASK_CREATE("tp_task", tp_task, NULL, 1024, OS_TASK_PRIORITY_NORMAL,
        tp_task_handle);

    OS_ASSERT(tp_task_handle);
}

#endif
