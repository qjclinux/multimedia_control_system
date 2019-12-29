/*
 * app_mgr_task.h
 *
 *  Created on: 2019��6��26��
 *      Author: jace
 */

#ifndef WEAR_SRC_APP_MGR_TASK_H_
#define WEAR_SRC_APP_MGR_TASK_H_


/*
 *  ϵͳ�¼�����
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
    //֪ͨ��
    SYS_EVENT_MSG_NOTIF,
    SYS_EVENT_CHARGING_NOTIF,
    SYS_EVENT_LOWBAT_NOTIF,
    SYS_EVENT_ALARMC_NOTIF,
    SYS_EVENT_SEDENTARY_NOTIF,

    //�����б仯
    SYS_EVENT_TIME_CHG,
    SYS_EVENT_BAT_CHG,
    SYS_EVENT_TP_CHG,
    SYS_EVENT_STEP_CHG,
    SYS_EVENT_SLEEP_CHG,
    SYS_EVENT_BL_CHG,
    SYS_EVENT_HR_CHG,
    SYS_EVENT_KEY_CHG,

//    SYS_EVENT_APP_START_PRE,    //����APP
//    SYS_EVENT_APP_START_NEXT,   //����APP
    SYS_EVENT_APP_START,    //����APP
    SYS_EVENT_APP_EXIT,     //�˳�APP
    SYS_EVENT_APP_INST,     //��װ�µ�APP
    SYS_EVENT_APP_UNINST,   //ж��APP

    SYS_EVENT_APP_START_WITH_ARG,    //������������APP

    SYS_EVENT_TIMER,    //��ʱ������ص���Ϊ��app_mgr��ִ��--��ֹ��ʱ��ջ���
    SYS_EVENT_FS_SYNC,  //�ļ�ϵͳ��ͬ����ʱ������ص���Ϊ��app_mgr��ִ��--��ֹ��ʱ��ջ���
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

//�˳���ǰAPP
void kill_current_app(void);



#endif /* WEAR_SRC_APP_MGR_TASK_H_ */
