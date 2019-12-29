
#ifndef __APP_STORAGE_H__ 
#define __APP_STORAGE_H__ 

#include <stdint.h>
#include <stdbool.h>
#include "res_type.h"


//APP详细信息
typedef struct{
    uint16_t app_id;
    uint16_t file_id;
    uint16_t type;
    uint8_t reserved[2];

    uint32_t elf_inrom_addr;    /* .tetx .rodata .data .bss 在flash代码区的地址--是分区内的地址，不是相对于整个flash的地址 */
    uint32_t elf_inrom_size;

    uint32_t elf_inram_addr;    /* .data .bss 在内存的地址 */

    int (*main)(int , char **);//app entry
}app_inst_info_t;


#define INVALID_ELF_INROM_ADDR (0xffffffff)
#define INVALID_ELF_INRAM_ADDR (0xffffffff)

//app 安装包在jacefs内指定的文件id
#define APP_INST_PKG_FILE_ID (0)



#endif

