/*
 * app_mgr.h
 *
 *  Created on: 2019年6月17日
 *      Author: Administrator
 */

#ifndef WEAR_SRC_APP_MGR_APP_MGR_H_

#define WEAR_SRC_APP_MGR_APP_MGR_H_

#include "os_list.h"
#include "app_storage.h"

/**
 *  应用优先级
 *
 */

typedef enum{
    APP_PRIORITY_HIGHEST,
    APP_PRIORITY_HIGH,
    APP_PRIORITY_NOLMAL,
    APP_PRIORITY_LOW,
    APP_PRIORITY_LOWEST,
}app_priority_t;

/**
 *  系统应用
 *
 */
typedef struct {
        os_list_t list;
        const app_inst_info_t *info;
        uint8_t priority;   /* APP运行优先级，主要是给通知类使用，其他类型APP统一要设置成 APP_PRIORITY_LOWEST */
} os_app_node_t;

#define IS_DIAL_TYPE(x) ((x)==APP_TYPE_DIAL||(x)==APP_TYPE_USER_DIAL)
#define IS_TOOL_TYPE(x) ((x)==APP_TYPE_TOOL||(x)==APP_TYPE_USER_TOOL)

#define IS_SYS_DIAL_TYPE(x) ((x)==APP_TYPE_DIAL)
#define IS_SYS_TOOL_TYPE(x) ((x)==APP_TYPE_TOOL)
#define IS_USER_DIAL_TYPE(x) ((x)==APP_TYPE_USER_DIAL)
#define IS_USER_TOOL_TYPE(x) ((x)==APP_TYPE_USER_TOOL)

#define IS_MISC_TYPE(x) ((x)==APP_TYPE_MISC)
#define IS_SUB_TYPE(x) ((x)==APP_TYPE_SUB)

#define IS_SYS_APP(x) ((x)==APP_TYPE_DIAL||(x)==APP_TYPE_TOOL)
#define IS_USER_APP(x) ((x)==APP_TYPE_USER_DIAL||(x)==APP_TYPE_USER_TOOL)

int os_app_add(os_app_node_t *node);
int os_app_copy_and_add(const app_inst_info_t *inst_info);
void os_app_remove(os_app_node_t *node);
void os_app_free(os_app_node_t *node);
os_app_node_t *os_app_get_node(uint16_t app_id);
os_app_node_t *os_app_get_node_next(uint16_t app_id);
os_app_node_t *os_app_get_node_before(uint16_t app_id);
void os_app_print_all(void);

/**
 *  APP属性获取
 *
 */
uint16_t os_app_get_type(uint16_t app_id);
uint16_t os_app_get_num(uint16_t type);
uint16_t os_app_get_first_id(uint16_t type);
/**
 *  当前运行的APP ID
 *
 */
void os_app_set_runing(uint16_t new_app_id);
uint16_t os_app_get_runing(void);
uint16_t os_app_get_id_next(uint16_t app_id);
uint16_t os_app_get_id_before(uint16_t app_id);

/**
 *  APP 运行/退出
 *
 */
int app_exit(uint16_t app_id,bool kill_window);
int app_start(uint16_t app_id);
int app_start_with_argument(uint16_t app_id,int argc, char *argv[]);
int app_start_without_rel(uint16_t app_id);
int app_exists(uint16_t app_id);

/**
 *
system app id

    0 表示无效ID
    1 表示OS安装包APP ID（包括图片、字库、OS固件）
    2 表示bootloader安装包APP ID
    3 表示临时文件APP ID
    4-255(0x60~0xff) 252个预留系统表盘、应用
    256-65534(0x100~0xfffe) 用户应用

 */
#define APP_ID_NONE (0x0000)

#define SYS_APP_ID_RES      (1)                 //系统安装包，包括图片/字库/OS升级固件
#define SYS_APP_ID_BL_FW    (2)                 //bootloader安装包
#define SYS_APP_ID_OS_FW    (SYS_APP_ID_RES)    //OS升级安装包
#define SYS_APP_ID_TMP      (3)                 //临时文件APP ID

#define USER_APP_ID_BASE (0x100)


#endif /* WEAR_SRC_APP_MGR_APP_MGR_H_ */
