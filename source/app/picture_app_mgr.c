/*
 * demo_app_mgr.c
 *
 *  Created on: 2019��8��19��
 *      Author: jace
 *
 * ����Ĳ��Դ����Ǵ�hwgt��Ŀ���ƹ����ģ�ע�Ͳ���û�ġ�
 *
 */


#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <time.h>
#include "osal.h"
#include "console.h"
#include "app_mgr.h"
#include "gui.h"
#include "system_state_service.h"
#include "gui_window.h"
#include "font_mgr.h"
#include "app_mgr_task.h"
#include "app_config.h"

#define DEBUG_PICT_APP 1
#if DEBUG_PICT_APP
    #define PICT_APP_LOG(fmt,args...) do{\
            /*printf("%s,%s(),%d:" ,__FILE__, __FUNCTION__,__LINE__);*/\
            os_printk("%s(),%d:" , __FUNCTION__,__LINE__);\
            os_printk(fmt,##args);\
        }while(0)
    #define PICT_APP_INFO(fmt,args...)        do{\
            os_printk(fmt,##args);\
        }while(0)
#else
    #define PICT_APP_LOG(fmt,args...)
    #define PICT_APP_INFO(fmt,args...)
#endif

static uint16_t latest_dial_app_id =DEFAULT_START_APP_ID;

static void __start_app(uint16_t app_id,bool kill_last_window,bool laod_immediately);
static void __start_app_with_arg(
    uint16_t app_id,bool kill_last_window,bool laod_immediately,bool force_start,
    int argc, char *argv[]);

static void __tp_evt_dispatch(void *pdata);

/*
 *  ��ĿĬ������APP��ӵ������б�
 */
void internal_app_register()
{
    dial1_init();
    dial2_init();

    app_light_init();
//    app_setting_init();
//    app_game_init();
//    app_music_init();
}

/*
 * ��Ŀ���Ƶ�APP�л�����װ�����д���
 */
void default_app_dispatch(void *pevt)
{
    sys_event_t *evt=(sys_event_t *)pevt;

    switch (evt->event_type)
    {
        //����APP
        //event.event_value Ϊapp_id������Ҫ�ͷ��ڴ�
        case SYS_EVENT_APP_START:
        {
            uint16_t app_id = evt->event_value;
            PICT_APP_LOG("start app_id=0x%04x(%d)\r\n", app_id,app_id);
            __start_app(app_id,true,true);
            break;
        }
        //�˳�APP
        //û�д�����������Ҫ�ͷ��ڴ�
        case SYS_EVENT_APP_EXIT:
        {
            PICT_APP_LOG("app exit,start latest dial=0x%04x(%d)\r\n",latest_dial_app_id,latest_dial_app_id);
            __start_app(latest_dial_app_id,true,true);
            break;
        }
        case SYS_EVENT_APP_INST:     //��װ�µ�APP
        case SYS_EVENT_APP_UNINST:   //ж��APP
        {
            break;
        }

        //������������APP
        //event.event_data Ϊapp�������ݣ�app_start_arg_t*���ͣ���Ҫ�ͷ��ڴ�
        case SYS_EVENT_APP_START_WITH_ARG:
        {
            app_start_arg_t *start_arg=(app_start_arg_t *)evt->event_data;
            if(start_arg)
            {
                PICT_APP_LOG("start app_id=0x%04x(%d),argc=%d,argv_addr=%08x\r\n",
                    start_arg->app_id,start_arg->app_id,start_arg->argc,start_arg->argv_addr);

                __start_app_with_arg(start_arg->app_id,true,true,
                    start_arg->force_start,start_arg->argc,(char**)start_arg->argv_addr);

                //ֻ����ϵͳ�¼��Ŀռ䣬app����argv����Ҫ����(�������͵�app����)
//                if(start_arg->argv_addr)
//                {
//                    OS_FREE((void*)start_arg->argv_addr);
//                }
                OS_FREE(start_arg);
            }

            break;
        }

        case SYS_EVENT_TP_CHG:
        {
            __tp_evt_dispatch(evt->event_data);
            break;
        }
    }

}

/*------------------------------------------------------------------------------------------------*/
/*
 *  ����APP��������ǰ���˳�ԭ����APP���������ʧ�ܣ�������֮ǰ��APP
 *
 */

static void __start_app_with_arg(
    uint16_t app_id,
    bool kill_last_window,
    bool laod_immediately,
    bool force_start,
    int argc,
    char *argv[])
{
    uint16_t last_app,cur_app=os_app_get_runing();

    if(!app_exists(app_id))
    {
        PICT_APP_LOG("app=%x not exists!\r\n",app_id);
        return;
    }

    if(!force_start && app_id==cur_app)
    {
        return ;
    }

    //��APP�����˳��¼�
    os_state_dispatch(cur_app,SYS_STA_APP_ON_EXIT,0,0);

    last_app=cur_app;
    cur_app=app_id;

    app_exit(last_app,kill_last_window);
    if(app_start_with_argument(cur_app,argc,argv))
    {
        //����ʧ�ܣ�������һ��������APP
        cur_app=last_app;
        app_start_with_argument(cur_app,argc,argv);
    }

    if( IS_DIAL_TYPE(os_app_get_type(app_id)) )
    {
        latest_dial_app_id=app_id;
    }

    if(laod_immediately && gui_window_get_loaded(NULL)==false)
    {
        gui_window_set_loaded(NULL);
        gui_window_show(NULL);
    }
}

static void __start_app(uint16_t app_id,bool kill_last_window,bool laod_immediately)
{
    __start_app_with_arg(app_id,kill_last_window,laod_immediately,false,0,0);
}


/*------------------------------------------------------------------------------------------------*/
/*
 *  TP�¼�����
 *
 *  ����Ĳ���ֻ������ 360*360 ����
 *
 */
#define SUPPORT_DRAG_UP_DOWN (0)
#define SUPPORT_RIGHT_SLIP_EXIT (1)

#define SRART_DIFF_X (20)      //�����������һ����л���X��Сֵ
#define DRAG_MIN_DIFF_X (10)   //TP�仯���ڸ�ֵ��ˢ�´���
#define ROLL_INV_X (80)        //�Զ������Ĵ��ڱ仯ֵ

#define SRART_DIFF_Y (30)       //�������»����л���Y��Сֵ
#define DRAG_MIN_DIFF_Y (10)    //TP�仯���ڸ�ֵ��ˢ�´���
#define ROLL_INV_Y (80)         //�Զ������Ĵ��ڱ仯ֵ

#define __diff(x,y) ((x)>(y) ? (x)-(y):(y)-(x))

#define TP_VAL_TO_RES(val) ((val)*(1.7f))    //TP�� 240*240��LCD��390*390������Ҫת��һ������

typedef enum{
    APP_LOADING_NONE,
    APP_LOADING_NEXT,       /* �����ұߵı��� */
    APP_LOADING_BEFORE,     /* ������ߵı��� */

#if SUPPORT_DRAG_UP_DOWN
    APP_LOADING_UP,         /* �����ϱߵĲ˵�--�����˵� */
    APP_LOADING_DOWN,       /* �����±ߵĲ˵�--�����˵� */

    APP_LOADING_DOWN_DONE,  /* �����±ߵĲ˵�--�����˵� ��ɣ���Ҫ���ڴ���֪ͨ�˵�����ʱ���޷�����release��������� */
#endif

    APP_LOADING_EXITING,   /* �һ��˳�APP */

}app_loading_t;

/*
 *  �л�Ч����Ҫ�ı���
 */
static int16_t

#if SUPPORT_DRAG_UP_DOWN
        diff_y_last=0,      //��һ��y�����ľ���
        diff_y_max=0,       //y������������
        diff_y_min=0,       //y��������С����
#endif
        diff_x_last=0,      //��һ��x�����ľ���
        diff_x_max=0,       //x������������
        diff_x_min=0;       //x��������С����

static app_loading_t app_loading=APP_LOADING_NONE;

#define ROLL_TO_MIN(effect,inv,dest) \
    for(;start_pix>0;start_pix-=inv)\
    {\
        if(start_pix<dest)\
        {\
            start_pix=dest;\
            break;\
        }\
        gui_window_show_mix(start_pix,effect);\
    }\
    gui_window_show_mix(dest,effect)

#define ROLL_TO_MAX(effect,inv,dest) \
    for(;start_pix<GUI_RES_X;start_pix+=inv)\
    {\
        if(start_pix>dest)\
        {\
            start_pix=dest;\
            break;\
        }\
        gui_window_show_mix(start_pix,effect);\
    }\
    gui_window_show_mix(dest,effect)


#define next_dial() os_app_get_id_next(cur_app)
#define before_dial() os_app_get_id_before(cur_app)

/*
 *  ����Ǳ��̣�
 *      ���һ����л�����
 *      �ϻ�����֪ͨҳ��
 *      �»�����״̬��
 */
static void __tp_evt_to_dial(gui_evt_tp_data_t *tp_data,uint16_t cur_app,uint16_t type)
{
    int16_t start_pix;

    switch(tp_data->evt_id)
    {
            /*
             *  TP �󻬣�������ߵ�APP
             *
             *  ע�⣺���TP�����һ�һС�㣬Ȼ�������󻬶�һ��Σ���ʱTP�Ǳ߻��ϱ����¼���
             *      ���Ƕ��ڱ����л���������������ͬʱ�л������������������һ����󻬵������������
             */
            case GUI_EVT_ON_SLIP_LEFT:
            {
//                PICT_APP_LOG("slip left\r\n");
                start_pix=0;

#if SUPPORT_DRAG_UP_DOWN
                if(app_loading==APP_LOADING_UP || app_loading==APP_LOADING_DOWN
                    || !IS_DIAL_TYPE(type) )
                {
                    break;
                }
#endif
                if(app_loading!=APP_LOADING_NONE)
                {
                    start_pix=abs(diff_x_last);
                }
                else
                {
//                    __start_app(next_dial(),false,false);//������
                    break;
                }

                //�����һ��ˣ���������һ�����̡����󻬣���ʱҪ������һ������
                if(app_loading==APP_LOADING_BEFORE)
                {
                    ROLL_TO_MIN(GUI_EFFECT_ROLL_R2L,ROLL_INV_X,0);
                    __start_app(next_dial(),true,true);
                }
                //������
                else
                {
                    ROLL_TO_MAX(GUI_EFFECT_ROLL_R2L,ROLL_INV_X,GUI_RES_X);
                    gui_window_set_loaded(NULL);
                    gui_window_show(NULL);
                }

                app_loading=APP_LOADING_NONE;
                break;
            }
            case GUI_EVT_ON_SLIP_RIGHT:
            {
//                PICT_APP_LOG("slip right\r\n");
                start_pix=0;

#if SUPPORT_DRAG_UP_DOWN
                if(app_loading==APP_LOADING_UP || app_loading==APP_LOADING_DOWN
                    || !IS_DIAL_TYPE(type) )
                {
                    break;
                }
#endif
                if(app_loading!=APP_LOADING_NONE)
                {
                    start_pix=abs(diff_x_last);
                }
                else
                {
//                    __start_app(before_dial(),false,false);//�����һ�
                    break;
                }

                //�������ˣ���������һ�����̡����һ�����ʱҪ������һ������
                if(app_loading==APP_LOADING_NEXT)
                {
                    ROLL_TO_MIN(GUI_EFFECT_ROLL_L2R,ROLL_INV_X,0);
                    __start_app(before_dial(),true,true);
                }
                //�����һ�
                else
                {
                    ROLL_TO_MAX(GUI_EFFECT_ROLL_L2R,ROLL_INV_X,GUI_RES_X);

                    gui_window_set_loaded(NULL);
                    gui_window_show(NULL);
                }
                app_loading=APP_LOADING_NONE;
                break;
            }
            case GUI_EVT_ON_SLIP_UP:
            {
//                PICT_APP_LOG("slip up\r\n");

#if SUPPORT_DRAG_UP_DOWN
//                if(app_loading==APP_LOADING_NONE) //�����»�
//                {
//                    __start_app(,false,false);
//                } else
                if((app_loading!=APP_LOADING_UP && app_loading!=APP_LOADING_DOWN)
                    /*|| IS_DIAL_TYPE(type)*/ )
                {
                    break;
                }

                start_pix=abs(diff_y_last);

                //�����»��ˣ����ϻ�
                if(app_loading==APP_LOADING_UP)
                {
                    //�»������������˵������ϻ�����ʱҪ������һ������
                    if(cur_app==SYS_APP_ID_DEFAULT_STATUS)
                    {
                        ROLL_TO_MIN(GUI_EFFECT_CONVERT_U2D_REVERSE,ROLL_INV_Y,0);
                        __start_app(latest_dial_app_id,true,true);
                    }
                    //�»����ر��̡����ϻ�����ʱҪ��ʾ��Ϣ
                    else
                    {
                        ROLL_TO_MIN(GUI_EFFECT_CONVERT_D2U,ROLL_INV_Y,0);
                        __start_app(SYS_APP_ID_DEFAULT_NOTIFY,true,true);
                    }
                }
                //�����ϻ�
                else
                {
                    //�����˵�ҳ�ϻ�����ʾ����
                    if(cur_app==latest_dial_app_id)
                    {
                        ROLL_TO_MAX(GUI_EFFECT_CONVERT_U2D_REVERSE,ROLL_INV_Y,GUI_RES_Y);
                    }
                    else //����ҳ�ϻ�����ʾ��Ϣ
                    {
                        ROLL_TO_MAX(GUI_EFFECT_CONVERT_D2U,ROLL_INV_Y,GUI_RES_Y);
                    }
                    gui_window_set_loaded(NULL);
                    gui_window_show(NULL);
                }

                //�޸�������ϻ�����֪ͨҳ����APP_LOADING_NONE���᲻����release�������������APP_LOADING_DOWN_DONE
                //app_loading=APP_LOADING_NONE;
                app_loading=APP_LOADING_DOWN_DONE;
#endif

                break;
            }
            case GUI_EVT_ON_SLIP_DOWN:
            {
//                PICT_APP_LOG("slip down\r\n");

#if SUPPORT_DRAG_UP_DOWN
//                if(app_loading==APP_LOADING_NONE) //�����»�
//                {
//                    __start_app(,false,false);
//                } else
                if((app_loading!=APP_LOADING_UP && app_loading!=APP_LOADING_DOWN)
                    /*|| IS_DIAL_TYPE(type)*/ )
                {
                    break;
                }

                start_pix=abs(diff_y_last);

                //�����ϻ��ˣ����»�
                if(app_loading==APP_LOADING_DOWN)
                {
                    //�ϻ���������Ϣ�����»�����ʱҪ������һ������
                    if(cur_app==SYS_APP_ID_DEFAULT_NOTIFY)
                    {
                        ROLL_TO_MIN(GUI_EFFECT_CONVERT_D2U_REVERSE,ROLL_INV_Y,0);
                        __start_app(latest_dial_app_id,true,true);
                    }
                    //�ϻ����ر��̡����»�����ʱҪ��ʾ�����˵�
                    else
                    {
                        ROLL_TO_MIN(GUI_EFFECT_CONVERT_U2D,ROLL_INV_Y,0);
                        __start_app(SYS_APP_ID_DEFAULT_STATUS,true,true);
                    }
                }
                //�����»�
                else
                {
                    //��Ϣҳ�»�����ʾ����
                    if(cur_app==latest_dial_app_id)
                    {
                        ROLL_TO_MAX(GUI_EFFECT_CONVERT_D2U_REVERSE,ROLL_INV_Y,GUI_RES_Y);
                    }
                    else //����ҳ�»�����ʾ�����˵�
                    {
                        ROLL_TO_MAX(GUI_EFFECT_CONVERT_U2D,ROLL_INV_Y,GUI_RES_Y);
                    }
                    gui_window_set_loaded(NULL);
                    gui_window_show(NULL);
                }
                app_loading=APP_LOADING_NONE;
#endif
                break;
            }
//            case GUI_EVT_ON_CLICK:
//            {
//                break;
//            }

            case GUI_EVT_ON_LONG_PRESS:
            {
#if 0
                if(app_loading==APP_LOADING_NONE && IS_DIAL_TYPE(type))
                {
                    __start_app(SYS_APP_ID_SELECT_DIAL,true,true);
                }
#endif
                break;
            }

            /*
             *  TP ����ʱ��¼��ǰ���꣬���ڱȽ��϶��ľ���
             */
            case GUI_EVT_ON_PRESSED:
            {
//                PICT_APP_LOG("press\r\n");

                app_loading=APP_LOADING_NONE;
                diff_x_last=0;
#if SUPPORT_DRAG_UP_DOWN
                diff_y_last=0;
#endif
                break;
            }

            /*
             * �������һ���
             *  TP �ɿ�������϶������˱��̣��жϱ����ĸ���ռ������࣬����Ǹ�Ϊ���������ı���
             *
             *  diff_x_last < 0��˵������������һ��APP����� abs(diff_x_last)>=120����Ҫ�л�APP�����������ʾ��ʣ����漴�ɣ�
             *                                    ���abs(diff_x_last)<120���ҹ������棨��ʾ��һ��APP���棩����ɺ�������һ��APP
             *
             *  diff_x_last > 0��˵���һ���������һ��APP����� abs(diff_x_last)>=120����Ҫ�л�APP�����ҹ�����ʾ��ʣ����漴�ɣ�
             *                                    ���abs(diff_x_last)<120����������棨��ʾ��һ��APP���棩����ɺ�������һ��APP
             *
             *  diff_x_last == 0������ǵ���0��˵�����һ����滬��֮���ֻ���ԭ���ı���
             *
             * �������»���
             *  �����һ�˼·һ�¡�
             */
            case GUI_EVT_ON_RELEASED:
            {
//                PICT_APP_LOG("release\r\n");

                if(app_loading!=APP_LOADING_NONE)
                {
#if SUPPORT_DRAG_UP_DOWN
                    if(app_loading==APP_LOADING_DOWN_DONE)
                    {

                    }
                    else if(app_loading==APP_LOADING_UP || app_loading==APP_LOADING_DOWN)
                    {
                        start_pix=abs(diff_y_last);
                        gui_window_effect_t effect;

                        //�൱���ϻ�
                        if(app_loading==APP_LOADING_DOWN)
                        {
                            /*
                             *  �������״̬���ϻ�����ʱ���б��̣���ʹ�õ���Ч��
                             */
                            if(cur_app==latest_dial_app_id)
                                effect=GUI_EFFECT_CONVERT_U2D_REVERSE;
                            else
                                effect=GUI_EFFECT_CONVERT_D2U;

                            if(start_pix>=GUI_RES_Y/2)
                            {
                                ROLL_TO_MAX(effect,ROLL_INV_Y,GUI_RES_Y);
                                gui_window_set_loaded(NULL);
                                gui_window_show(NULL);
                            }
                            else
                            {
                                ROLL_TO_MIN(effect,ROLL_INV_Y,0);

                                /*
                                 * �ڱ���ҳ�ϻ�����ʱ����֪ͨ�����ص�����
                                 * ��״̬��ҳ�ϻ�����ʱ���б��̣����ص�״̬��ҳ
                                 */
                                if(cur_app==SYS_APP_ID_DEFAULT_NOTIFY)
                                {
                                    __start_app(latest_dial_app_id,true,true);
                                }
                                else
                                {
                                    __start_app(SYS_APP_ID_DEFAULT_STATUS,true,true);
                                }
                            }

                        }

                        //�൱���»�
                        else //if(app_loading==APP_LOADING_UP)
                        {
                            /*
                             *  �������֪ͨ���»�����ʱ���б��̣���ʹ�õ���Ч��
                             */
                            if(cur_app==latest_dial_app_id)
                                effect=GUI_EFFECT_CONVERT_D2U_REVERSE;
                            else
                                effect=GUI_EFFECT_CONVERT_U2D;

                            if(start_pix>=GUI_RES_Y/2)
                            {
                                ROLL_TO_MAX(effect,ROLL_INV_Y,GUI_RES_Y);
                                gui_window_set_loaded(NULL);
                                gui_window_show(NULL);
                            }
                            else
                            {
                                ROLL_TO_MIN(effect,ROLL_INV_Y,0);

                                /*
                                 * �ڱ���ҳ�»�����ʱ����״̬��ҳ�����ص�����
                                 * ��֪ͨҳ�ϻ�����ʱ���б��̣����ص�֪ͨ
                                 */
                                if(cur_app==SYS_APP_ID_DEFAULT_STATUS)
                                {
                                    __start_app(latest_dial_app_id,true,true);
                                }
                                else
                                {
                                    __start_app(SYS_APP_ID_DEFAULT_NOTIFY,true,true);
                                }
                            }
                        }

                    }
                    else
#endif
                    {
                        start_pix=abs(diff_x_last);

                        //�൱����
                        if(app_loading==APP_LOADING_NEXT)
                        {
                            if(start_pix>=GUI_RES_X/2)
                            {
                                ROLL_TO_MAX(GUI_EFFECT_ROLL_R2L,ROLL_INV_X,GUI_RES_X);
                                gui_window_set_loaded(NULL);
                                gui_window_show(NULL);
                            }
                            else
                            {
                                ROLL_TO_MIN(GUI_EFFECT_ROLL_R2L,ROLL_INV_X,0);
                                __start_app(before_dial(),true,true);
                            }

                        }

                        //�൱���һ�
                        else //if(app_loading==APP_LOADING_BEFORE)
                        {
                            if(start_pix>=GUI_RES_X/2)
                            {
                                ROLL_TO_MAX(GUI_EFFECT_ROLL_L2R,ROLL_INV_X,GUI_RES_X);
                                gui_window_set_loaded(NULL);
                                gui_window_show(NULL);
                            }
                            else
                            {
                                ROLL_TO_MIN(GUI_EFFECT_ROLL_L2R,ROLL_INV_X,0);
                                __start_app(next_dial(),true,true);
                            }

                        }
                    }

                    app_loading=APP_LOADING_NONE;
                }

                //�������б��̣�ֻҪ�����ɿ���˵�����ڼ�����ɣ�Ҫ����ϸ�����
                gui_window_destroy_last();

                break;
            }

            /*
             *  TP������
             *  ��TP�Ǵ�������ұ߻�����x����10��y��ֵС��10������������һ������
             *  ��TP�Ǵ��ұ�����߻�����x��С10��y��ֵС��10������������һ������
             *
             */
            case GUI_EVT_ON_DRAG:
            {
                int16_t diff_x,diff_y;

                diff_x=TP_VAL_TO_RES(tp_data->peak_x);
                diff_y=TP_VAL_TO_RES(tp_data->peak_y);

//                PICT_APP_LOG("drag x=%d dx=%d\r\n",tp_data->x,diff_x);
                if(diff_x==0 && diff_y==0)
                {
                    break;
                }

                if(app_loading==APP_LOADING_NONE)
                {

#if SUPPORT_DRAG_UP_DOWN
                    /*
                     *  �»�
                     *      1 �ڱ���ҳ���»�����״̬����
                     *      2 ��״̬������֧���»�
                     *      3 ��֪ͨҳ���»���ȥ���̣���Ϊ����������ڶ����˾ͻص����̣�������ڶ��������Ϣ����APP��
                     *  �ϻ�
                     *      1 �ڱ���ҳ���ϻ�����֪ͨ��
                     *      2 ��֪ͨ������֧���ϻ�����Ϊ����Ϣֱ�Ӵ���APP���ɣ�
                     *      3 ��״̬�����ϻ��ص�����
                     *
                     */
                    //���»���
                    if(/*abs(diff_x)<SRART_DIFF_X &&*/ abs(diff_y)>=SRART_DIFF_Y)
                    {
                        if(diff_y>=SRART_DIFF_Y)//�»�
                        {
                            if(cur_app==SYS_APP_ID_DEFAULT_STATUS)
                            {
                                break;
                            }
                            if(cur_app==SYS_APP_ID_DEFAULT_NOTIFY)
                            {
                                __start_app(latest_dial_app_id,false,false);
                            }
                            else
                            {
                                __start_app(SYS_APP_ID_DEFAULT_STATUS,false,false);
                            }
                            app_loading=APP_LOADING_UP;
                            diff_y_max=GUI_RES_Y;
                            diff_y_min=0;
                        }
                        else //if(diff_y<=-SRART_DIFF_Y) //�ϻ�
                        {
                            if(cur_app==SYS_APP_ID_DEFAULT_NOTIFY)
                            {
                                break;
                            }
                            if(cur_app==SYS_APP_ID_DEFAULT_STATUS)
                            {
                                __start_app(latest_dial_app_id,false,false);
                            }
                            else
                            {
                                __start_app(SYS_APP_ID_DEFAULT_NOTIFY,false,false);
                            }
                            app_loading=APP_LOADING_DOWN;
                            diff_y_max=0;
                            diff_y_min=-GUI_RES_Y;
                        }
                    }

                    //֪ͨ��״̬ ���������һ���
                    else if(cur_app==SYS_APP_ID_DEFAULT_STATUS || cur_app==SYS_APP_ID_DEFAULT_NOTIFY)
                    {
                        break;
                    }
                    else
#endif
                    //���һ���
                    if(abs(diff_x)>=SRART_DIFF_X && abs(diff_y)<SRART_DIFF_Y)
                    {
                        if(diff_x>=SRART_DIFF_X)//�һ�
                        {
                            __start_app(before_dial(),false,false);
                            app_loading=APP_LOADING_BEFORE;
                            diff_x_max=GUI_RES_X;
                            diff_x_min=0;
                        }
                        else //if(diff_x<=-SRART_DIFF_X)//��
                        {
                            __start_app(next_dial(),false,false);
                            app_loading=APP_LOADING_NEXT;
                            diff_x_max=0;
                            diff_x_min=-GUI_RES_X;
                        }
                    }
                    else
                    {
                        break;
                    }
                }

#if SUPPORT_DRAG_UP_DOWN
                if(app_loading==APP_LOADING_UP || app_loading==APP_LOADING_DOWN)
                {
                    //��������������APP����������»�ȡ��ʾ��������
                    cur_app=os_app_get_runing();

                    //��ֹ����Խ��
                    if(diff_y<diff_y_min)
                        diff_y=diff_y_min;
                    else if(diff_y>diff_y_max)
                        diff_y=diff_y_max;


                    if(__diff(diff_y_last,diff_y)>=DRAG_MIN_DIFF_Y)
                    {
                        if(app_loading==APP_LOADING_UP)//�»�
                        {
                            //�������֪ͨ�»�����ʱ���б��̣���ʹ�õ���Ч��
                            if(cur_app==latest_dial_app_id)
                                gui_window_show_mix(diff_y,GUI_EFFECT_CONVERT_D2U_REVERSE);
                            //����ҳ�»�����ʾ�����˵�
                            else
                                gui_window_show_mix(diff_y,GUI_EFFECT_CONVERT_U2D);
                        }
                        else //if(app_loading==APP_LOADING_DOWN) //�ϻ�
                        {
                            //������������˵����ϻ�����ʱ���б��̣���ʹ�õ���Ч��
                            if(cur_app==latest_dial_app_id)
                                gui_window_show_mix(-diff_y,GUI_EFFECT_CONVERT_U2D_REVERSE);
                            //����ҳ�ϻ�����ʾ��Ϣ
                            else
                                gui_window_show_mix(-diff_y,GUI_EFFECT_CONVERT_D2U);
                        }
                        diff_y_last=diff_y;
                    }
                    break;
                }
                else
#endif
                {
                    //��ֹ����Խ��
                    if(diff_x<diff_x_min)
                        diff_x=diff_x_min;
                    else if(diff_x>diff_x_max)
                        diff_x=diff_x_max;


                    if(__diff(diff_x_last,diff_x)>=DRAG_MIN_DIFF_X)
                    {
                        if(app_loading==APP_LOADING_BEFORE)
                            gui_window_show_mix(diff_x,GUI_EFFECT_ROLL_L2R);
                        else //if(app_loading==APP_LOADING_NEXT)
                            gui_window_show_mix(-diff_x,GUI_EFFECT_ROLL_R2L);
                        diff_x_last=diff_x;
                    }
                }

                break;
            }

            /*
             *   �������¼�
             */
            default:
            {
                gui_window_dispatch_event(NULL,
                    tp_data->evt_id,(uint8_t*)tp_data,sizeof(gui_evt_tp_data_t));
                break;
            }
    }

}

/*
 *  ϵͳAPP    --APP_TYPE_TOOL
 *  �Ӳ˵�                 --APP_TYPE_SUB
 *
 *      �һ��ص�����
 *
 */
#if SUPPORT_RIGHT_SLIP_EXIT
static void __tp_evt_to_sys_app(gui_evt_tp_data_t *tp_data,uint16_t cur_app,uint16_t type)
{
    static uint16_t last_exit_app=APP_ID_NONE;
    int16_t start_pix;

    switch(tp_data->evt_id)
    {
            case GUI_EVT_ON_SLIP_LEFT:
            {
#if 0
//                PICT_APP_LOG("slip left\r\n");
                start_pix=0;

                if(app_loading!=APP_LOADING_NONE)
                {
                    start_pix=abs(diff_x_last);
                }
                else
                {
//                    __start_app(next_dial(),false,false);//������
                    break;
                }

                //�����һ��ˣ���������һ�����̡����󻬣���ʱҪ������һ������
                if(app_loading==APP_LOADING_BEFORE)
                {
                    ROLL_TO_MIN(GUI_EFFECT_ROLL_R2L,ROLL_INV_X,0);
                    __start_app(next_dial(),true,true);
                }
                //������
                else
                {
                    ROLL_TO_MAX(GUI_EFFECT_ROLL_R2L,ROLL_INV_X,GUI_RES_X);
                    gui_window_set_loaded(NULL);
                    gui_window_show(NULL);
                }

                app_loading=APP_LOADING_NONE;
#endif
                break;
            }

            case GUI_EVT_ON_SLIP_RIGHT:
            {
#if 1
//                PICT_APP_LOG("slip right\r\n");
                start_pix=0;

                if(app_loading!=APP_LOADING_NONE)
                {
                    start_pix=abs(diff_x_last);
                }
                else
                {
                    //__start_app(latest_dial_app_id,false,false);//�����һ� --TODO:�����������������BusFault_Handler ��ԭ����

                    return;//����ѡ��return ����Ϊ�˲���APP����TP�¼�
                }

                //�����һ�
                //if(app_loading==APP_LOADING_EXITING)
                {
                    ROLL_TO_MAX(GUI_EFFECT_CONVERT_R2L_REVERSE,ROLL_INV_X,GUI_RES_X);

                    gui_window_set_loaded(NULL);
                    gui_window_show(NULL);
                }
                app_loading=APP_LOADING_NONE;
#endif
                return;//����ѡ��return ����Ϊ�˲���APP����TP�¼�
                //break;
            }
            case GUI_EVT_ON_SLIP_UP:
            {

                break;
            }
            case GUI_EVT_ON_SLIP_DOWN:
            {
                break;
            }

            case GUI_EVT_ON_LONG_PRESS:
            {
                break;
            }

            /*
             *  TP ����ʱ��¼��ǰ���꣬���ڱȽ��϶��ľ���
             */
            case GUI_EVT_ON_PRESSED:
            {
//                PICT_APP_LOG("press\r\n");
                app_loading=APP_LOADING_NONE;
                diff_x_last=0;
                break;
            }

            case GUI_EVT_ON_RELEASED:
            {
//                PICT_APP_LOG("release\r\n");
#if 1
                if(app_loading!=APP_LOADING_NONE)
                {
                    start_pix=abs(diff_x_last);

                    //�൱���һ�
                    if(app_loading==APP_LOADING_EXITING)
                    {
                        if(start_pix>=GUI_RES_X/2)
                        {
                            ROLL_TO_MAX(GUI_EFFECT_CONVERT_R2L_REVERSE,ROLL_INV_X,GUI_RES_X);
                            gui_window_set_loaded(NULL);
                            gui_window_show(NULL);
                        }
                        else
                        {
                            ROLL_TO_MIN(GUI_EFFECT_CONVERT_R2L_REVERSE,ROLL_INV_X,0);
                            __start_app(last_exit_app,true,true);
                        }

                    }
                    app_loading=APP_LOADING_NONE;

                    //�������б��̣�ֻҪ�����ɿ���˵�����ڼ�����ɣ�Ҫ����ϸ�����
                    gui_window_destroy_last();

                    //����ѡ��return ����Ϊ�˲���APP����TP�¼�
                    return;
                }
#endif
                break;
            }

            case GUI_EVT_ON_DRAG:
            {
                int16_t diff_x,diff_y;

                diff_x=TP_VAL_TO_RES(tp_data->peak_x);
                diff_y=TP_VAL_TO_RES(tp_data->peak_y);

//                PICT_APP_LOG("drag x=%d dx=%d\r\n",tp_data->x,diff_x);
                if(diff_x==0 && diff_y==0)
                {
                    break;
                }

                if(app_loading==APP_LOADING_NONE)
                {
                    //���һ���
                    if(abs(diff_x)>=SRART_DIFF_X && abs(diff_y)<SRART_DIFF_Y)
                    {
                        if(diff_x>=SRART_DIFF_X)//�һ�
                        {
                            last_exit_app=cur_app;
                            __start_app(latest_dial_app_id,false,false);
                            app_loading=APP_LOADING_EXITING;
                            diff_x_max=GUI_RES_X;
                            diff_x_min=0;
                        }
                        else //if(diff_x<=-SRART_DIFF_X)//��--������
                        {
                            //TODO:�޸��󻬣����һ��������л������
                            break;
                        }
                    }
                    else
                    {
                        //TODO:�޸����»������һ��������л������
                        break;
                    }
                }

                {
                    //��ֹ����Խ��
                    if(diff_x<diff_x_min)
                        diff_x=diff_x_min;
                    else if(diff_x>diff_x_max)
                        diff_x=diff_x_max;


                    if(__diff(diff_x_last,diff_x)>=DRAG_MIN_DIFF_X)
                    {
                        if(app_loading==APP_LOADING_EXITING)
                            gui_window_show_mix(diff_x,GUI_EFFECT_CONVERT_R2L_REVERSE);
                        else
                        {
                            //TODO�����һ������»�Ӧ�ý�������������л�
                        }
                        diff_x_last=diff_x;
                    }
                }

                break;
            }
    }


    if(app_loading!=APP_LOADING_EXITING)
    {
        gui_window_dispatch_event(NULL,
            tp_data->evt_id,(uint8_t*)tp_data,sizeof(gui_evt_tp_data_t));
    }



}
#endif

static void __tp_evt_dispatch(void *pdata)
{

    /*
     * ��������TP�϶��¼�ȫ��ȡ������ֹ�϶��¼����ർ��GUI��ʾ��ʱ
     *
     */
#if 1
    sys_event_t peek_event;
    gui_evt_tp_data_t *peak_tp_data;

    do{
        if(xQueuePeek(m_sys_evt_queue, &peek_event, 0) == pdFALSE)
        {
            break;
        }

        if(peek_event.event_type==SYS_EVENT_TP_CHG)
        {
            peak_tp_data=(gui_evt_tp_data_t*)peek_event.event_data;
            if(peak_tp_data->evt_id==GUI_EVT_ON_DRAG)
            {
                //���¼��Ӷ������Ƴ�
                if (xQueueReceive(m_sys_evt_queue, &peek_event, 0) == pdFALSE)
                {
                    break;
                }

//                PICT_APP_INFO("^");
                if(pdata)
                {
                    OS_FREE(pdata);
                }
                pdata=peak_tp_data;
                continue;
            }
        }
        break;

    }while(1);
#endif

    gui_evt_tp_data_t *tp_data=(gui_evt_tp_data_t*)pdata;
    if(!tp_data)
    {
        return;
    }


#if 0
    const char tp_evt_chr[][15]={
        "NONE",
        "CLICK",
        "DOUBLE_CLICK",
        "LONG_PRESS",
        "PRESSED",
        "RELEASED",
        "DRAG",
        "SLIP_LEFT",
        "SLIP_RIGHT",
        "SLIP_UP",
        "SLIP_DOWN",
    };

    PICT_APP_LOG(" %s xy={%d,%d} peak={%d,%d}\r\n",
        tp_evt_chr[tp_data->evt_id],tp_data->x,tp_data->y,
        tp_data->peak_x,tp_data->peak_y
        );
#endif

    uint16_t cur_app =os_app_get_runing();
    uint16_t type=os_app_get_type(cur_app);

    /*
     * �������Ϣ֪ͨ�б�
     * ���»������Բ鿴��Ϣ�������������������ص����̣����һ���ɾ����Ϣ��TODO����ʵ�֣�
     *
     */
#if SUPPORT_DRAG_UP_DOWN
    if(cur_app==SYS_APP_ID_DEFAULT_NOTIFY && app_loading==APP_LOADING_NONE)
    {
        gui_window_dispatch_event(NULL,
                tp_data->evt_id,(uint8_t*)tp_data,sizeof(gui_evt_tp_data_t));

        /*
         * ����TP��ʱ��������ڶ�������ô���Գ��Իص����̣�����������ܻص�����
         *
         * ע�⣺������µ��ڶ����������ϻ��������»�����yֵ�Ȱ��µ�󣩣���ôҲ���ܻص�����
         */
        static bool can_back_to_dial=false;
        static int16_t org_y=0;

        if(tp_data->evt_id==GUI_EVT_ON_PRESSED && gui_window_get_y(NULL)==0)
        {
            can_back_to_dial=true;
            org_y=tp_data->y;
        }
        else if(tp_data->evt_id==GUI_EVT_ON_RELEASED || tp_data->y<org_y)
        {
            can_back_to_dial=false;
        }

        if(can_back_to_dial)
        {
            __tp_evt_to_dial(tp_data,cur_app,type);
        }

    }else
#endif

    /*
     * ������û�Ӧ�á�����ѡ��ҳ��TP�¼���APP���д�������ֱ�Ӵ���TP���ݼ���
     *
     * �������˶�app
     *
     */
    if(IS_USER_TOOL_TYPE(type) )
    {
        gui_window_dispatch_event(NULL,
            tp_data->evt_id,(uint8_t*)tp_data,sizeof(gui_evt_tp_data_t));
    }

    /*
     * �����ϵͳӦ�á��������ϵ��Ӳ˵�������Ӧ���һ��ص����̣����������⴦��
     */
#if SUPPORT_RIGHT_SLIP_EXIT
    else if( IS_SYS_TOOL_TYPE(type) || IS_SUB_TYPE(type) || app_loading==APP_LOADING_EXITING)
    {
        __tp_evt_to_sys_app(tp_data,cur_app,type);
    }
#endif
    /*
     * �����������л����̣�֪ͨ��״̬�����»�������˳�
     */
    else
    {
        __tp_evt_to_dial(tp_data,cur_app,type);
    }

    if(tp_data)
    {
        OS_FREE(tp_data);
    }

}




