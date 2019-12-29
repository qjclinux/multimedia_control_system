/*
 * system_state_service.h
 *
 *  Created on: 2019年6月17日
 *      Author: Administrator
 */

#ifndef WEAR_SRC_SERVICE_SYSTEM_STATUS_SERVICE_H_
#define WEAR_SRC_SERVICE_SYSTEM_STATUS_SERVICE_H_

#include "os_list.h"
#include <stdint.h>

typedef struct {
    os_list_t list;
    uint32_t app_id;
    uint32_t event;
    void (*callback)(uint32_t event,const void *pdata,uint16_t size);
} os_state_node_t;

typedef enum{
    SYS_STA_SECOND_CHANGE   =0x1<<0,
//    SYS_STA_MIN_CHANGE      =0x1<<1,
//    SYS_STA_HOUR_CHANGE     =0x1<<2,
//    SYS_STA_DAY_CHANGE      =0x1<<3,

    SYS_STA_STEP_CHANGE     =0x1<<4,
    SYS_STA_SLEEP_CHANGE    =0x1<<5,
    SYS_STA_BL_CHANGE       =0x1<<6,
    SYS_STA_CHARGE_CHANGE   =0x1<<7,
    SYS_STA_BAT_CHANGE      =0x1<<8,

    SYS_STA_APP_ON_EXIT     =0x1<<9,

    SYS_STA_HR_CHANGE       =0x1<<10,

}sys_sta_item_t;

/*
 *  注意！！！ 这里的操作没有进行多线程、中断使用的保护，所以所以的调用必须在同一线程中！
 */

typedef struct tm os_sta_data_time_t ;
typedef uint32_t os_sta_data_step_t ;
typedef void* os_sta_data_sleep_t ;
typedef bool os_sta_data_bl_t ;
typedef bool os_sta_data_charge_t ;
typedef int8_t os_sta_data_battery_t ;
typedef uint8_t os_sta_data_heart_t ;

int os_state_subscribe(uint32_t event,
    void (*callback)(uint32_t event,const void *pdata,uint16_t size) );
void os_state_unsubscribe(uint32_t app_id);
os_state_node_t *os_state_node_get(uint32_t app_id);
void os_state_dispatch(uint32_t app_id,uint32_t event,const void *pdata,uint16_t size);


#endif /* WEAR_SRC_SERVICE_SYSTEM_STATUS_SERVICE_H_ */
