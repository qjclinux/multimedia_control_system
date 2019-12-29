/*
 * font_mgr.h
 *
 *  Created on: 2019��8��12��
 *      Author: jace
 */

#ifndef WEAR_SRC_APP_MGR_FONT_MGR_H_
#define WEAR_SRC_APP_MGR_FONT_MGR_H_

#include "res_type.h"
#include "os_list.h"

/**-----------------------------------------------*/

//�ֿ� ��ϸ��Ϣ
typedef struct{
    os_list_t list;
    uint16_t app_id;
    uint16_t file_id;
    uint32_t offset;
    inst_pkg_res_font_t font_attr;
}font_info_t;

int os_font_add(font_info_t *font_info);
int  os_font_get(const char *name,font_info_t *node);
font_info_t* os_font_get_suitable_size(const char *name,uint8_t size);
int  os_font_exists(const char *name);
int  os_font_delete(const char *name);
int  os_font_delete_all(uint16_t app_id);

void os_font_print_all(void);
void os_font_self_test(void);

#endif /* WEAR_SRC_APP_MGR_FONT_MGR_H_ */
