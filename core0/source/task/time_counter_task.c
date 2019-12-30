
/*
 * time_counter_task.c
 *
 *  Created on: 2019年6月26日
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
 * utc 秒数从1970年开始
 * 默认时间 ： 北京时间  2019/12/28 14:12:6
 * TODO：这个变量要设置成关机开机后数据不变
 */
static time_t m_utc_sec=1577513526+8*60*60;

//localtime 转换出来的时间从 1900年开始
static struct tm now_time ;


//app 设置时间
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


//获取UTC秒数，该秒数为从1970年到现在为止的秒数
time_t os_get_utc_second()
{
    time_t _utc;
    OS_ENTER_CRITICAL_SECTION();
    _utc=m_utc_sec;
    OS_LEAVE_CRITICAL_SECTION();
    return  _utc;

}


//获取时间结构体（其中 tm->tm_year 已经是加上1900的，所以直接用该时间即可，不要再处理）
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
 *  计时  服务
 *
 */
static void time_counter_task(void *params)
{
    sys_event_t time_evt;
    time_evt.event_type = SYS_EVENT_TIME_CHG;


    for (;;)
    {
        //TODO：从RTC获取时间
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

