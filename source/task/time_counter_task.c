
/*
 * time_counter_task.c
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
#include "system_state_service.h"
#include "app_mgr_task.h"
#include "time.h"
#include "time_counter_task.h"

/*
 * utc ������1970�꿪ʼ
 * Ĭ��ʱ�� �� ����ʱ��  2019/12/28 14:12:6
 * TODO���������Ҫ���óɹػ����������ݲ���
 */
static time_t m_utc_sec=1577513526+8*60*60;

//localtime ת��������ʱ��� 1900�꿪ʼ
static struct tm now_time ;


//app ����ʱ��
void os_set_time(const uint8_t* data)
{
    if(data==NULL)
        return;
    struct tm t;
    time_t sec;

    t.tm_year=(data[0]<<8|data[1])-1900;
    t.tm_mon=data[2]-1;
    t.tm_mday=data[3];
    t.tm_hour=data[4];
    t.tm_min=data[5];
    t.tm_sec=data[6];
    t.tm_isdst = 0;
    sec=mktime(&t);

    OS_LOG("set time ,sec=%d ,%d-%d-%d %d:%d:%d\r\n",
        (uint32_t)sec ,
        t.tm_year+1900,t.tm_mon+1,t.tm_mday,t.tm_hour,t.tm_min,t.tm_sec
        );

    OS_ENTER_CRITICAL_SECTION();
    m_utc_sec=sec;
    OS_LEAVE_CRITICAL_SECTION();
}


//��ȡUTC������������Ϊ��1970�굽����Ϊֹ������
time_t os_get_utc_second()
{
    time_t _utc;
    OS_ENTER_CRITICAL_SECTION();
    _utc=m_utc_sec;
    OS_LEAVE_CRITICAL_SECTION();
    return  _utc;

}


//��ȡʱ��ṹ�壨���� tm->tm_year �Ѿ��Ǽ���1900�ģ�����ֱ���ø�ʱ�伴�ɣ���Ҫ�ٴ���
struct tm os_get_now_time()
{
    struct tm _tm;
    OS_ENTER_CRITICAL_SECTION();
    _tm=now_time;
    OS_LEAVE_CRITICAL_SECTION();
    return  _tm;
}

#if 1

static OS_TASK  time_counter_task_handle ;

/*
 *  ��ʱ  ����
 *
 */
static void time_counter_task(void *params)
{
    sys_event_t time_evt;
    time_evt.event_type = SYS_EVENT_TIME_CHG;


    for (;;)
    {
        //TODO����RTC��ȡʱ��
        m_utc_sec++;

        now_time = *localtime( &m_utc_sec);
        now_time.tm_year+=1900;

//        OS_LOG("time= %d-%d-%d %d:%d:%d wday=%d\r\n",
//            now_time.tm_year,now_time.tm_mon,now_time.tm_mday,
//            now_time.tm_hour,now_time.tm_min,now_time.tm_sec,now_time.tm_wday);

        time_evt.event_data=&now_time;

        if ( xQueueSend( m_sys_evt_queue,( void * ) &time_evt,0 ) != pdPASS)
        {

            //TODO:fix this error.
        }

        OS_DELAY_MS(1000);
    }
}

void time_counter_task_init()
{
//    return;
    OS_TASK_CREATE("time_counter", time_counter_task, NULL, 1024, OS_TASK_PRIORITY_NORMAL,
        time_counter_task_handle);
    OS_ASSERT(time_counter_task_handle);
}
#endif

