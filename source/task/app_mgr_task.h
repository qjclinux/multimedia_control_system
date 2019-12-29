/*
 * app_mgr_task.h
 *
 *  Created on: 2019年6月26日
 *      Author: jace
 */

#ifndef WEAR_SRC_APP_MGR_TASK_H_
#define WEAR_SRC_APP_MGR_TASK_H_


/*
 *  系统事件队列
 *
 */
#define OS_EVENT_QUEUE_LEN (30)
#define OS_EVENT_ITEM_SIZE (sizeof(sys_event_t))
typedef struct
{
    uint32_t event_type;        //ref @sys_event_type_t
    union{
        void *event_data;
        uint32_t event_value;
    };
} sys_event_t;

extern QueueHandle_t m_sys_evt_queue;
extern QueueHandle_t m_gui_evt_queue;



typedef enum
{
    //通知类
    SYS_EVENT_MSG_NOTIF,
    SYS_EVENT_CHARGING_NOTIF,
    SYS_EVENT_LOWBAT_NOTIF,
    SYS_EVENT_ALARMC_NOTIF,
    SYS_EVENT_SEDENTARY_NOTIF,

    //数据有变化
    SYS_EVENT_TIME_CHG,
    SYS_EVENT_BAT_CHG,
    SYS_EVENT_TP_CHG,
    SYS_EVENT_STEP_CHG,
    SYS_EVENT_SLEEP_CHG,
    SYS_EVENT_BL_CHG,
    SYS_EVENT_HR_CHG,
    SYS_EVENT_KEY_CHG,

//    SYS_EVENT_APP_START_PRE,    //启动APP
//    SYS_EVENT_APP_START_NEXT,   //启动APP
    SYS_EVENT_APP_START,    //启动APP
    SYS_EVENT_APP_EXIT,     //退出APP
    SYS_EVENT_APP_INST,     //安装新的APP
    SYS_EVENT_APP_UNINST,   //卸载APP

    SYS_EVENT_APP_START_WITH_ARG,    //带参数的启动APP

    SYS_EVENT_TIMER,    //定时器服务回调改为在app_mgr中执行--防止定时器栈溢出
    SYS_EVENT_FS_SYNC,  //文件系统的同步定时器服务回调改为在app_mgr中执行--防止定时器栈溢出
} sys_event_type_t;



typedef struct{
    uint16_t app_id;
    bool force_start;
    int argc;
    uint32_t argv_addr;
}app_start_arg_t;
void switch_app_with_arg(uint16_t app_id,bool force_start,int argc,uint32_t argv_addr);

void switch_app(uint16_t app_id);


/*
 * SDK symbol
 */

//退出当前APP
void kill_current_app(void);



#endif /* WEAR_SRC_APP_MGR_TASK_H_ */
