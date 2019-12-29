#ifndef RES_TYPE_H
#define RES_TYPE_H

#include <stdint.h>

//文件头
typedef struct{
    uint8_t magic[10];
    uint16_t crc16_total;
}inst_pkg_header_t;
//APP信息
typedef struct{
    uint16_t id;
    uint16_t version;
    uint16_t sdk_version;
    uint8_t name[30];
    uint16_t type;
}inst_pkg_app_info_t;
#define APP_NAME_LEN (30)
//资源信息
typedef struct{
    uint32_t elf_offset;
    uint32_t elf_size;
    uint32_t icon_offset;
    uint32_t icon_size;
    uint32_t res_num;
}inst_pkg_res_info_t;
//资源描述
typedef struct{
    uint16_t type;
    uint16_t id;
    uint32_t size;
    uint32_t offset;
}inst_pkg_res_desc_t;


//资源-RGB565
typedef struct {
    uint16_t width;
    uint16_t height;
//    uint8_t *data;
}inst_pkg_res_picture_t;

//资源-font
#define FONT_NAME_LENGHT (10)
#define FONT_DATA_HDR_SIZE (FONT_NAME_LENGHT+4)
typedef struct {
    char name[FONT_NAME_LENGHT];
    uint8_t size;
    uint8_t width;
    uint8_t height;
    uint8_t bytes_per_word;
//    uint8_t *data;
}inst_pkg_res_font_t;

/**-----------------------------------------------*/
//for res pack

#define PKG_MAGIC "InstatllPkg"
//#define RES_TYPE_RGB565 "rgb565"
//#define RES_TYPE_RGB888 "rgb888"
//#define RES_TYPE_RGB "rgb"
//#define RES_TYPE_RGB3 "rgb3"
//#define RES_TYPE_RGB1 "rgb1"
//#define RES_TYPE_FONT "font"

typedef enum{
    RES_TYPE_UNKNOW =0,
    RES_TYPE_RGB565 =1,
    RES_TYPE_RGB888 =2,
    RES_TYPE_RGB    =3,
    RES_TYPE_RGB3   =4,
    RES_TYPE_RGB1   =5,
    RES_TYPE_FONT   =0x11,
}res_type_t;

typedef enum{
    APP_TYPE_UNKNOW=0,
    APP_TYPE_OS_FW=1,
    APP_TYPE_BOOTLOADER=2,
    APP_TYPE_DIAL=3,        /* 系统内置：恢复出厂设置不删除 */
    APP_TYPE_TOOL=4,
    APP_TYPE_USER_DIAL=5,   /* 用户安装：恢复出厂设置删除 */
    APP_TYPE_USER_TOOL=6,
    APP_TYPE_FONT=7,        /* 字库 */

    APP_TYPE_MISC=8,        /* 系统内置：菜单等 */
    APP_TYPE_SUB=9,         /* 系统内置：第二级菜单以上的应用全部放在这里 */
}app_type_t;

#define PKG_HEADER_OFFSET (0)
#define PKG_APP_INFO_OFFSET (sizeof(inst_pkg_header_t))
#define PKG_RES_INFO_OFFSET (sizeof(inst_pkg_header_t)+sizeof(inst_pkg_app_info_t))
#define PKG_RES_DESC_OFFSET (sizeof(inst_pkg_header_t)+sizeof(inst_pkg_app_info_t)+sizeof(inst_pkg_res_info_t))

//#pragma pack(1)
typedef struct{
    inst_pkg_header_t header;
    inst_pkg_app_info_t app_info;
    inst_pkg_res_info_t res_info;
}inst_pkg_t;
//#pragma pack()


#define INVALID_APP_ID 0xffff
#define INVALID_APP_VERSION 0xffff
#define INVALID_SDK_VERSION 0xffff
#define INVALID_APP_TYPE 0

//自动生成资源ID的初始值
#define BASE_RES_ID 10001

//app icon 默认资源id（icon在安装包内无id，而其他资源id>0，故当本系统中资源id索引为0时默认为icon）
#define ICON_RES_ID (0)

#endif // RES_TYPE_H
