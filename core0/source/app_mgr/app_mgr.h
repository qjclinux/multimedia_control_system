/*
 * app_mgr.h
 *
 *  Created on: 2019��6��17��
 *      Author: Administrator
 */

#ifndef WEAR_SRC_APP_MGR_APP_MGR_H_

#define WEAR_SRC_APP_MGR_APP_MGR_H_

#include "os_list.h"
#include "app_storage.h"

/**
 *  Ӧ�����ȼ�
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
 *  ϵͳӦ��
 *
 */
typedef struct {
        os_list_t list;
        const app_inst_info_t *info;
        uint8_t priority;   /* APP�������ȼ�����Ҫ�Ǹ�֪ͨ��ʹ�ã���������APPͳһҪ���ó� APP_PRIORITY_LOWEST */
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
 *  APP���Ի�ȡ
 *
 */
uint16_t os_app_get_type(uint16_t app_id);
uint16_t os_app_get_num(uint16_t type);
uint16_t os_app_get_first_id(uint16_t type);
/**
 *  ��ǰ���е�APP ID
 *
 */
void os_app_set_runing(uint16_t new_app_id);
uint16_t os_app_get_runing(void);
uint16_t os_app_get_id_next(uint16_t app_id);
uint16_t os_app_get_id_before(uint16_t app_id);

/**
 *  APP ����/�˳�
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

    0 ��ʾ��ЧID
    1 ��ʾOS��װ��APP ID������ͼƬ���ֿ⡢OS�̼���
    2 ��ʾbootloader��װ��APP ID
    3 ��ʾ��ʱ�ļ�APP ID
    4-255(0x60~0xff) 252��Ԥ��ϵͳ���̡�Ӧ��
    256-65534(0x100~0xfffe) �û�Ӧ��

 */
#define APP_ID_NONE (0x0000)

#define SYS_APP_ID_RES      (1)                 //ϵͳ��װ��������ͼƬ/�ֿ�/OS�����̼�
#define SYS_APP_ID_BL_FW    (2)                 //bootloader��װ��
#define SYS_APP_ID_OS_FW    (SYS_APP_ID_RES)    //OS������װ��
#define SYS_APP_ID_TMP      (3)                 //��ʱ�ļ�APP ID

#define USER_APP_ID_BASE (0x100)


#endif /* WEAR_SRC_APP_MGR_APP_MGR_H_ */
