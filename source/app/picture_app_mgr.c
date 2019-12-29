/*
 * demo_app_mgr.c
 *
 *  Created on: 2019年8月19日
 *      Author: jace
 *
 * 这里的测试代码是从hwgt项目中移过来的，注释部分没改。
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
 *  项目默认内置APP添加到运行列表
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
 * 项目定制的APP切换、安装、运行处理
 */
void default_app_dispatch(void *pevt)
{
    sys_event_t *evt=(sys_event_t *)pevt;

    switch (evt->event_type)
    {
        //启动APP
        //event.event_value 为app_id，不需要释放内存
        case SYS_EVENT_APP_START:
        {
            uint16_t app_id = evt->event_value;
            PICT_APP_LOG("start app_id=0x%04x(%d)\r\n", app_id,app_id);
            __start_app(app_id,true,true);
            break;
        }
        //退出APP
        //没有带参数，不需要释放内存
        case SYS_EVENT_APP_EXIT:
        {
            PICT_APP_LOG("app exit,start latest dial=0x%04x(%d)\r\n",latest_dial_app_id,latest_dial_app_id);
            __start_app(latest_dial_app_id,true,true);
            break;
        }
        case SYS_EVENT_APP_INST:     //安装新的APP
        case SYS_EVENT_APP_UNINST:   //卸载APP
        {
            break;
        }

        //带参数的启动APP
        //event.event_data 为app启动数据，app_start_arg_t*类型，需要释放内存
        case SYS_EVENT_APP_START_WITH_ARG:
        {
            app_start_arg_t *start_arg=(app_start_arg_t *)evt->event_data;
            if(start_arg)
            {
                PICT_APP_LOG("start app_id=0x%04x(%d),argc=%d,argv_addr=%08x\r\n",
                    start_arg->app_id,start_arg->app_id,start_arg->argc,start_arg->argv_addr);

                __start_app_with_arg(start_arg->app_id,true,true,
                    start_arg->force_start,start_arg->argc,(char**)start_arg->argv_addr);

                //只回收系统事件的空间，app参数argv不需要回收(由所发送的app处理)
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
 *  启动APP，在启动前先退出原来的APP，如果启动失败，则启动之前的APP
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

    //给APP发送退出事件
    os_state_dispatch(cur_app,SYS_STA_APP_ON_EXIT,0,0);

    last_app=cur_app;
    cur_app=app_id;

    app_exit(last_app,kill_last_window);
    if(app_start_with_argument(cur_app,argc,argv))
    {
        //启动失败，启动上一个正常的APP
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
 *  TP事件处理
 *
 *  这里的参数只适配了 360*360 的屏
 *
 */
#define SUPPORT_DRAG_UP_DOWN (0)
#define SUPPORT_RIGHT_SLIP_EXIT (1)

#define SRART_DIFF_X (20)      //启动表盘左右滑动切换的X最小值
#define DRAG_MIN_DIFF_X (10)   //TP变化大于该值才刷新窗口
#define ROLL_INV_X (80)        //自动滚动的窗口变化值

#define SRART_DIFF_Y (30)       //启动上下滑动切换的Y最小值
#define DRAG_MIN_DIFF_Y (10)    //TP变化大于该值才刷新窗口
#define ROLL_INV_Y (80)         //自动滚动的窗口变化值

#define __diff(x,y) ((x)>(y) ? (x)-(y):(y)-(x))

#define TP_VAL_TO_RES(val) ((val)*(1.7f))    //TP是 240*240，LCD是390*390，所以要转换一下坐标

typedef enum{
    APP_LOADING_NONE,
    APP_LOADING_NEXT,       /* 加载右边的表盘 */
    APP_LOADING_BEFORE,     /* 加载左边的表盘 */

#if SUPPORT_DRAG_UP_DOWN
    APP_LOADING_UP,         /* 加载上边的菜单--下拉菜单 */
    APP_LOADING_DOWN,       /* 加载下边的菜单--上拉菜单 */

    APP_LOADING_DOWN_DONE,  /* 加载下边的菜单--上拉菜单 完成，主要用于处理通知菜单加载时，无法进入release处理的问题 */
#endif

    APP_LOADING_EXITING,   /* 右滑退出APP */

}app_loading_t;

/*
 *  切换效果需要的变量
 */
static int16_t

#if SUPPORT_DRAG_UP_DOWN
        diff_y_last=0,      //上一次y滑动的距离
        diff_y_max=0,       //y滑动的最大距离
        diff_y_min=0,       //y滑动的最小距离
#endif
        diff_x_last=0,      //上一次x滑动的距离
        diff_x_max=0,       //x滑动的最大距离
        diff_x_min=0;       //x滑动的最小距离

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
 *  如果是表盘：
 *      左右滑动切换表盘
 *      上滑进入通知页面
 *      下滑进入状态栏
 */
static void __tp_evt_to_dial(gui_evt_tp_data_t *tp_data,uint16_t cur_app,uint16_t type)
{
    int16_t start_pix;

    switch(tp_data->evt_id)
    {
            /*
             *  TP 左滑：启动左边的APP
             *
             *  注意：如果TP先向右滑一小点，然后再向左滑动一大段，此时TP那边会上报左滑事件，
             *      但是对于表盘切换不允许左右两边同时切换的情况，所以如果先右滑再左滑的情况不属于左滑
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
//                    __start_app(next_dial(),false,false);//快速左滑
                    break;
                }

                //先向右滑了，加载了上一个表盘。再左滑，此时要加载下一个表盘
                if(app_loading==APP_LOADING_BEFORE)
                {
                    ROLL_TO_MIN(GUI_EFFECT_ROLL_R2L,ROLL_INV_X,0);
                    __start_app(next_dial(),true,true);
                }
                //正常左滑
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
//                    __start_app(before_dial(),false,false);//快速右滑
                    break;
                }

                //先向左滑了，加载了下一个表盘。再右滑，此时要加载上一个表盘
                if(app_loading==APP_LOADING_NEXT)
                {
                    ROLL_TO_MIN(GUI_EFFECT_ROLL_L2R,ROLL_INV_X,0);
                    __start_app(before_dial(),true,true);
                }
                //正常右滑
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
//                if(app_loading==APP_LOADING_NONE) //快速下滑
//                {
//                    __start_app(,false,false);
//                } else
                if((app_loading!=APP_LOADING_UP && app_loading!=APP_LOADING_DOWN)
                    /*|| IS_DIAL_TYPE(type)*/ )
                {
                    break;
                }

                start_pix=abs(diff_y_last);

                //先向下滑了，再上滑
                if(app_loading==APP_LOADING_UP)
                {
                    //下滑加载了下拉菜单。再上滑，此时要加载上一个表盘
                    if(cur_app==SYS_APP_ID_DEFAULT_STATUS)
                    {
                        ROLL_TO_MIN(GUI_EFFECT_CONVERT_U2D_REVERSE,ROLL_INV_Y,0);
                        __start_app(latest_dial_app_id,true,true);
                    }
                    //下滑加载表盘。再上滑，此时要显示消息
                    else
                    {
                        ROLL_TO_MIN(GUI_EFFECT_CONVERT_D2U,ROLL_INV_Y,0);
                        __start_app(SYS_APP_ID_DEFAULT_NOTIFY,true,true);
                    }
                }
                //正常上滑
                else
                {
                    //下拉菜单页上滑，显示表盘
                    if(cur_app==latest_dial_app_id)
                    {
                        ROLL_TO_MAX(GUI_EFFECT_CONVERT_U2D_REVERSE,ROLL_INV_Y,GUI_RES_Y);
                    }
                    else //表盘页上滑，显示消息
                    {
                        ROLL_TO_MAX(GUI_EFFECT_CONVERT_D2U,ROLL_INV_Y,GUI_RES_Y);
                    }
                    gui_window_set_loaded(NULL);
                    gui_window_show(NULL);
                }

                //修复如果是上滑进入通知页，给APP_LOADING_NONE将会不进入release处理，所以这里给APP_LOADING_DOWN_DONE
                //app_loading=APP_LOADING_NONE;
                app_loading=APP_LOADING_DOWN_DONE;
#endif

                break;
            }
            case GUI_EVT_ON_SLIP_DOWN:
            {
//                PICT_APP_LOG("slip down\r\n");

#if SUPPORT_DRAG_UP_DOWN
//                if(app_loading==APP_LOADING_NONE) //快速下滑
//                {
//                    __start_app(,false,false);
//                } else
                if((app_loading!=APP_LOADING_UP && app_loading!=APP_LOADING_DOWN)
                    /*|| IS_DIAL_TYPE(type)*/ )
                {
                    break;
                }

                start_pix=abs(diff_y_last);

                //先向上滑了，再下滑
                if(app_loading==APP_LOADING_DOWN)
                {
                    //上滑加载了消息。再下滑，此时要加载上一个表盘
                    if(cur_app==SYS_APP_ID_DEFAULT_NOTIFY)
                    {
                        ROLL_TO_MIN(GUI_EFFECT_CONVERT_D2U_REVERSE,ROLL_INV_Y,0);
                        __start_app(latest_dial_app_id,true,true);
                    }
                    //上滑加载表盘。再下滑，此时要显示下拉菜单
                    else
                    {
                        ROLL_TO_MIN(GUI_EFFECT_CONVERT_U2D,ROLL_INV_Y,0);
                        __start_app(SYS_APP_ID_DEFAULT_STATUS,true,true);
                    }
                }
                //正常下滑
                else
                {
                    //消息页下滑，显示表盘
                    if(cur_app==latest_dial_app_id)
                    {
                        ROLL_TO_MAX(GUI_EFFECT_CONVERT_D2U_REVERSE,ROLL_INV_Y,GUI_RES_Y);
                    }
                    else //表盘页下滑，显示下拉菜单
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
             *  TP 按下时记录当前坐标，用于比较拖动的距离
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
             * 对于左右滑：
             *  TP 松开：如果拖动启动了表盘，判断表盘哪个所占界面更多，多的那个为最终启动的表盘
             *
             *  diff_x_last < 0，说明左滑启动的下一个APP，如果 abs(diff_x_last)>=120则不需要切换APP，向左滚动显示完剩余界面即可；
             *                                    如果abs(diff_x_last)<120向右滚动界面（显示上一个APP界面），完成后启动上一个APP
             *
             *  diff_x_last > 0，说明右滑启动的上一个APP，如果 abs(diff_x_last)>=120则不需要切换APP，向右滚动显示完剩余界面即可；
             *                                    如果abs(diff_x_last)<120向左滚动界面（显示上一个APP界面），完成后启动下一个APP
             *
             *  diff_x_last == 0，如果是等于0，说明左滑右滑交替滑动之后，又滑回原来的表盘
             *
             * 对于上下滑：
             *  与左右滑思路一致。
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

                        //相当于上滑
                        if(app_loading==APP_LOADING_DOWN)
                        {
                            /*
                             *  如果是在状态栏上滑（此时运行表盘），使用倒放效果
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
                                 * 在表盘页上滑（此时运行通知），回到表盘
                                 * 在状态栏页上滑（此时运行表盘），回到状态栏页
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

                        //相当于下滑
                        else //if(app_loading==APP_LOADING_UP)
                        {
                            /*
                             *  如果是在通知栏下滑（此时运行表盘），使用倒放效果
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
                                 * 在表盘页下滑（此时运行状态栏页），回到表盘
                                 * 在通知页上滑（此时运行表盘），回到通知
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

                        //相当于左滑
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

                        //相当于右滑
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

                //正在运行表盘，只要操作松开，说明窗口加载完成，要清除上个窗口
                gui_window_destroy_last();

                break;
            }

            /*
             *  TP滑动：
             *  当TP是从左边向右边滑动（x增大10，y差值小于10），则启动上一个表盘
             *  当TP是从右边向左边滑动（x减小10，y差值小于10），则启动下一个表盘
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
                     *  下滑
                     *      1 在表盘页，下滑启动状态栏，
                     *      2 在状态栏，不支持下滑
                     *      3 在通知页，下滑回去表盘（改为：如果窗口在顶部了就回到表盘，如果不在顶部则把消息传给APP）
                     *  上滑
                     *      1 在表盘页，上滑启动通知栏
                     *      2 在通知栏，不支持上滑（改为：消息直接传给APP即可）
                     *      3 在状态栏，上滑回到表盘
                     *
                     */
                    //上下滑动
                    if(/*abs(diff_x)<SRART_DIFF_X &&*/ abs(diff_y)>=SRART_DIFF_Y)
                    {
                        if(diff_y>=SRART_DIFF_Y)//下滑
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
                        else //if(diff_y<=-SRART_DIFF_Y) //上滑
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

                    //通知和状态 不接受左右滑动
                    else if(cur_app==SYS_APP_ID_DEFAULT_STATUS || cur_app==SYS_APP_ID_DEFAULT_NOTIFY)
                    {
                        break;
                    }
                    else
#endif
                    //左右滑动
                    if(abs(diff_x)>=SRART_DIFF_X && abs(diff_y)<SRART_DIFF_Y)
                    {
                        if(diff_x>=SRART_DIFF_X)//右滑
                        {
                            __start_app(before_dial(),false,false);
                            app_loading=APP_LOADING_BEFORE;
                            diff_x_max=GUI_RES_X;
                            diff_x_min=0;
                        }
                        else //if(diff_x<=-SRART_DIFF_X)//左滑
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
                    //上面重新启动了APP，如果不重新获取显示将有问题
                    cur_app=os_app_get_runing();

                    //防止坐标越界
                    if(diff_y<diff_y_min)
                        diff_y=diff_y_min;
                    else if(diff_y>diff_y_max)
                        diff_y=diff_y_max;


                    if(__diff(diff_y_last,diff_y)>=DRAG_MIN_DIFF_Y)
                    {
                        if(app_loading==APP_LOADING_UP)//下滑
                        {
                            //如果是在通知下滑（此时运行表盘），使用倒放效果
                            if(cur_app==latest_dial_app_id)
                                gui_window_show_mix(diff_y,GUI_EFFECT_CONVERT_D2U_REVERSE);
                            //表盘页下滑，显示下拉菜单
                            else
                                gui_window_show_mix(diff_y,GUI_EFFECT_CONVERT_U2D);
                        }
                        else //if(app_loading==APP_LOADING_DOWN) //上滑
                        {
                            //如果是在下拉菜单栏上滑（此时运行表盘），使用倒放效果
                            if(cur_app==latest_dial_app_id)
                                gui_window_show_mix(-diff_y,GUI_EFFECT_CONVERT_U2D_REVERSE);
                            //表盘页上滑，显示消息
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
                    //防止坐标越界
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
             *   单击等事件
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
 *  系统APP    --APP_TYPE_TOOL
 *  子菜单                 --APP_TYPE_SUB
 *
 *      右滑回到表盘
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
//                    __start_app(next_dial(),false,false);//快速左滑
                    break;
                }

                //先向右滑了，加载了上一个表盘。再左滑，此时要加载下一个表盘
                if(app_loading==APP_LOADING_BEFORE)
                {
                    ROLL_TO_MIN(GUI_EFFECT_ROLL_R2L,ROLL_INV_X,0);
                    __start_app(next_dial(),true,true);
                }
                //正常左滑
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
                    //__start_app(latest_dial_app_id,false,false);//快速右滑 --TODO:如果开开启这个会出现BusFault_Handler ，原因不明

                    return;//这里选择return ，是为了不给APP发送TP事件
                }

                //正常右滑
                //if(app_loading==APP_LOADING_EXITING)
                {
                    ROLL_TO_MAX(GUI_EFFECT_CONVERT_R2L_REVERSE,ROLL_INV_X,GUI_RES_X);

                    gui_window_set_loaded(NULL);
                    gui_window_show(NULL);
                }
                app_loading=APP_LOADING_NONE;
#endif
                return;//这里选择return ，是为了不给APP发送TP事件
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
             *  TP 按下时记录当前坐标，用于比较拖动的距离
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

                    //相当于右滑
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

                    //正在运行表盘，只要操作松开，说明窗口加载完成，要清除上个窗口
                    gui_window_destroy_last();

                    //这里选择return ，是为了不给APP发送TP事件
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
                    //左右滑动
                    if(abs(diff_x)>=SRART_DIFF_X && abs(diff_y)<SRART_DIFF_Y)
                    {
                        if(diff_x>=SRART_DIFF_X)//右滑
                        {
                            last_exit_app=cur_app;
                            __start_app(latest_dial_app_id,false,false);
                            app_loading=APP_LOADING_EXITING;
                            diff_x_max=GUI_RES_X;
                            diff_x_min=0;
                        }
                        else //if(diff_x<=-SRART_DIFF_X)//左滑--不处理
                        {
                            //TODO:修复左滑，再右滑，导致切换的情况
                            break;
                        }
                    }
                    else
                    {
                        //TODO:修复上下滑，再右滑，导致切换的情况
                        break;
                    }
                }

                {
                    //防止坐标越界
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
                            //TODO：左右滑、上下滑应该进入这里，而不是切换
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
     * 把连续的TP拖动事件全部取出，防止拖动事件过多导致GUI显示延时
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
                //把事件从队列中移除
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
     * 如果是消息通知列表，
     * 上下滑动可以查看消息，滑到顶部后再下拉回到表盘；左右滑动删除消息（TODO：待实现）
     *
     */
#if SUPPORT_DRAG_UP_DOWN
    if(cur_app==SYS_APP_ID_DEFAULT_NOTIFY && app_loading==APP_LOADING_NONE)
    {
        gui_window_dispatch_event(NULL,
                tp_data->evt_id,(uint8_t*)tp_data,sizeof(gui_evt_tp_data_t));

        /*
         * 按下TP的时候，如果是在顶部，那么可以尝试回到表盘；其他情况不能回到表盘
         *
         * 注意：如果按下点在顶部，先向上滑动再向下滑动（y值比按下点大），那么也不能回到表盘
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
     * 如果是用户应用、表盘选择页，TP事件由APP自行处理，所以直接传递TP数据即可
     *
     * 新增：运动app
     *
     */
    if(IS_USER_TOOL_TYPE(type) )
    {
        gui_window_dispatch_event(NULL,
            tp_data->evt_id,(uint8_t*)tp_data,sizeof(gui_evt_tp_data_t));
    }

    /*
     * 如果是系统应用、二级以上的子菜单，这类应用右滑回到表盘，所以做特殊处理
     */
#if SUPPORT_RIGHT_SLIP_EXIT
    else if( IS_SYS_TOOL_TYPE(type) || IS_SUB_TYPE(type) || app_loading==APP_LOADING_EXITING)
    {
        __tp_evt_to_sys_app(tp_data,cur_app,type);
    }
#endif
    /*
     * 表盘类左右切换表盘，通知、状态栏上下滑进入和退出
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




