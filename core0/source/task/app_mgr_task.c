
/*
 * wear_task.c
 *
 *  Created on: 2019��6��17��
 *      Author: jace
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <time.h>
#include "osal.h"
#include "console.h"
#include "system_state_service.h"
//#include "timer_service.h"
//#include "com_service.h"
#include "jacefs.h"
#include "jacefs_port.h"
#include "app_mgr.h"
#include "gui.h"
#include "gui_window.h"
#include "app_mgr_task.h"
#include "app_config.h"

#define DEBUG_APP_TASK 1
#if DEBUG_APP_TASK
    #define APP_TASK_LOG(fmt,args...) do{\
            /*printf("%s,%s(),%d:" ,__FILE__, __FUNCTION__,__LINE__);*/\
            os_printk("%s(),%d:" , __FUNCTION__,__LINE__);\
            os_printk(fmt,##args);\
        }while(0)
    #define APP_TASK_INFO(fmt,args...)        do{\
            os_printk(fmt,##args);\
        }while(0)
#else
    #define APP_TASK_LOG(fmt,args...)
    #define APP_TASK_INFO(fmt,args...)
#endif

QueueHandle_t m_sys_evt_queue;


static OS_TASK  app_mgr_task_hdl=NULL;


/*------------------------------------------------------------------------------------------------*/
//�ⲿ�����˳���ǰAPP����
void kill_current_app(void)
{
    sys_event_t step_event;
    step_event.event_type = SYS_EVENT_APP_EXIT;
    step_event.event_data=0;
    if ( xQueueSend( m_sys_evt_queue,( void * ) &step_event,0 ) != pdPASS)
    {
        ;//TODO:fix this err
    }
}

/*------------------------------------------------------------------------------------------------*/
//�ⲿ���������µ�APP
void switch_app(uint16_t app_id)
{
    sys_event_t app_event;
    app_event.event_type = SYS_EVENT_APP_START;
    app_event.event_value= app_id;
    if ( xQueueSend( m_sys_evt_queue,( void * ) &app_event,0 ) != pdPASS)
    {
        APP_TASK_LOG("send err!\r\n");
    }
}

//����������APP�ķ�ʽһ��ҪС��ʹ��
//�ڴ�������һ��Ҫ�����
void switch_app_with_arg(uint16_t app_id,bool force_start,int argc,uint32_t argv_addr)
{
    app_start_arg_t *start_arg=(app_start_arg_t *)OS_MALLOC(sizeof(app_start_arg_t));
    if(!start_arg)
    {
        return;
    }
    start_arg->app_id=app_id;
    start_arg->argc=argc;
    start_arg->argv_addr=argv_addr;
    start_arg->force_start=force_start;

    sys_event_t app_event;
    app_event.event_type = SYS_EVENT_APP_START_WITH_ARG;
    app_event.event_data= start_arg;

    if ( xQueueSend( m_sys_evt_queue,( void * ) &app_event,0 ) != pdPASS)
    {
        APP_TASK_LOG("send err!\r\n");
    }
}

/*------------------------------------------------------------------------------------------------*/
/*
 *  ϵͳ����ʱ��������һ��Ӧ��
 */
static void start_first_app()
{
    //TODO:������һ�ιػ�ʱ��APP

    sys_event_t step_event;
    step_event.event_type = SYS_EVENT_APP_START;
    step_event.event_value= DEFAULT_START_APP_ID;
    xQueueSend( m_sys_evt_queue,( void * ) &step_event,0 );
}

//----------------------------------------------------------------------------------------------
// ϵͳ��ʼ��
//----------------------------------------------------------------------------------------------
static void os_init_pre()
{
    jacefs_init();

    gui_init();
//    gui_self_test();

/*
 * �������ʼ�����������
 */
    //����APP��ʼ��
    internal_app_register();
    os_app_print_all();

    //�ⲿAPP��ʼ��
//    external_app_init();

    //APP���£��ֿ���ӵ�ϵͳ�б�
//    app_reload();

//    os_font_print_all();

}

static void os_init()
{
    m_sys_evt_queue = xQueueCreate(OS_EVENT_QUEUE_LEN, OS_EVENT_ITEM_SIZE);
    if (!m_sys_evt_queue)
    {
        //TODO:fix this error
        while(1);
    }

    os_init_pre();
    start_first_app();

    /*
                 * ��������
     */
    void time_counter_task_init();
    time_counter_task_init();

    void tp_task_init();
    tp_task_init();
}

/*------------------------------------------------------------------------------------------------*/
/*
 *  APP��������
 *
 */
static void app_mgr_task(void *params)
{
    sys_event_t event;

    os_init();

    //this task use for app management
    for (;;)
    {
        if (xQueueReceive(m_sys_evt_queue, &event, OS_MS_2_TICKS(1000)) == pdFALSE)
        {
#if configMEM_DEBUG
            void mem_debug_info_print(void);
            mem_debug_info_print();
#endif
            continue;
        }



        switch (event.event_type)
        {
            //ʱ��
            //event.event_data Ϊʱ��ṹ�壬sizeof(struct tm) �ֽ� struct tm ���ͣ�����Ҫ�ͷ��ڴ�
            case SYS_EVENT_TIME_CHG:
            {
                struct tm *now_time;
                now_time=(struct tm *)event.event_data;

//                APP_TASK_LOG("time= %d-%d-%d %d:%d:%d wday=%d\r\n",
//                    now_time->tm_year,now_time->tm_mon,now_time->tm_mday,
//                    now_time->tm_hour,now_time->tm_min,now_time->tm_sec,
//                    now_time->tm_wday);

                os_state_dispatch(os_app_get_runing(), SYS_STA_SECOND_CHANGE, now_time,sizeof(struct tm));

                break;
            }


            /*-----------------------------------------------------*/
            case SYS_EVENT_TIMER:
            {

            }
            case SYS_EVENT_FS_SYNC:
            {
                void (*fs_sync_func)()=event.event_data;
                fs_sync_func();
                break;
            }


            /*-----------------------------------------------------*/
            /*
             *  ������Ϣ�ɶ��Ƶ���Ŀ����
             */
            default:
            {
                void default_app_dispatch(void *pevt);
                default_app_dispatch(&event);
            }

        }

    }

}

// ϵͳ������
void main_task_init(void)
{
    //APP ����
    OS_TASK_CREATE("app_mgr_task", app_mgr_task, NULL, 1024*4, OS_TASK_PRIORITY_NORMAL,
        app_mgr_task_hdl);
    OS_ASSERT(app_mgr_task_hdl);
}

