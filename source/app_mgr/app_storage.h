
#ifndef __APP_STORAGE_H__ 
#define __APP_STORAGE_H__ 

#include <stdint.h>
#include <stdbool.h>
#include "res_type.h"


//APP��ϸ��Ϣ
typedef struct{
    uint16_t app_id;
    uint16_t file_id;
    uint16_t type;
    uint8_t reserved[2];

    uint32_t elf_inrom_addr;    /* .tetx .rodata .data .bss ��flash�������ĵ�ַ--�Ƿ����ڵĵ�ַ���������������flash�ĵ�ַ */
    uint32_t elf_inrom_size;

    uint32_t elf_inram_addr;    /* .data .bss ���ڴ�ĵ�ַ */

    int (*main)(int , char **);//app entry
}app_inst_info_t;


#define INVALID_ELF_INROM_ADDR (0xffffffff)
#define INVALID_ELF_INRAM_ADDR (0xffffffff)

//app ��װ����jacefs��ָ�����ļ�id
#define APP_INST_PKG_FILE_ID (0)



#endif

