/*
 * gui_res.c
 *
 *  Created on: 2019年7月11日
 *      Author: jace
 */

#include <string.h>
#include <stdbool.h>
#include "osal.h"
#include "gui.h"
#include "console.h"
#include "gui_window.h"
#include "gui_res.h"
#include "gui_lock.h"
#include "font_data.h"
#include "app_mgr.h"
#include "jacefs.h"

#define DEBUG_GUI_RES 0

#if DEBUG_GUI_RES
#define GUI_RES_LOG(fmt,args...)    do{\
                                /*os_printk("%s,%s(),%d:" ,__FILE__, __FUNCTION__,__LINE__);*/\
                                os_printk("%s(),%d:" , __FUNCTION__,__LINE__);\
                                os_printk(fmt,##args);\
                            }while(0)
#define GUI_RES_INFO(fmt,args...)   do{\
                                os_printk(fmt,##args);\
                            }while(0)
#else
#define GUI_RES_LOG(fmt,args...)
#define GUI_RES_INFO(fmt,args...)
#endif


//获取资源描述
int gui_res_get_desc(uint16_t app_id,uint16_t file_id,uint16_t res_id,inst_pkg_res_desc_t *res_desc)
{
    uint32_t res_num;
    if(!res_desc)
        return -1;

    if(jacefs_read(file_id,app_id,(uint8_t*)&res_num,sizeof(uint32_t),PKG_RES_DESC_OFFSET-sizeof(uint32_t))
        !=sizeof(uint32_t))
    {
        GUI_RES_LOG("res info read err!\r\n");
        return -1;
    }

    if(res_num==0)
    {
        GUI_RES_LOG("res num==0 err!\r\n");
        return -1;
    }

    //由于资源现在用的ID是顺序增加的，所以直接取查找的资源ID与第一个ID之差，快速定位资源位置
    if(res_id>=BASE_RES_ID)
    {
        uint32_t find_idx=res_id-BASE_RES_ID;

        if(jacefs_read(file_id,app_id,(uint8_t*)res_desc,sizeof(inst_pkg_res_desc_t),
            PKG_RES_DESC_OFFSET+sizeof(inst_pkg_res_desc_t)*find_idx)
            !=sizeof(inst_pkg_res_desc_t))
        {
            GUI_RES_LOG("res desc read err!\r\n");
            return -1;
        }
        GUI_RES_LOG("desc[%d]={res_id=%d,type=%d,offset=%d,size=%d,}\r\n",
            find_idx,res_desc->id,res_desc->type,res_desc->offset,res_desc->size);

        if(res_desc->id==res_id)
        {
            GUI_RES_LOG("res desc found!\r\n");
            return 0;
        }
    }


    //遍历方法查找资源(速度慢)
    for(int i=0;i<res_num;i++)
    {
        if(jacefs_read(file_id,app_id,(uint8_t*)res_desc,sizeof(inst_pkg_res_desc_t),PKG_RES_DESC_OFFSET+sizeof(inst_pkg_res_desc_t)*i)
                !=sizeof(inst_pkg_res_desc_t))
            {
                GUI_RES_LOG("res desc read err!\r\n");
                return -1;
            }

        GUI_RES_LOG("desc[%d]={res_id=%d,type=%d,offset=%d,size=%d,}\r\n",
            i,res_desc->id,res_desc->type,res_desc->offset,res_desc->size);

        if(res_desc->id==res_id)
        {
            GUI_RES_LOG("res desc found!\r\n");
            return 0;
        }
    }

    GUI_RES_LOG("res desc not found!\r\n");
    return -1;
}

//获取icon资源描述
int gui_icon_get_desc(uint16_t app_id,uint16_t file_id,inst_pkg_res_desc_t *res_desc)
{

    if(!res_desc)
        return -1;

    inst_pkg_res_info_t res_info;

    if(jacefs_read(file_id,app_id,(uint8_t*)&res_info,sizeof(inst_pkg_res_info_t),PKG_RES_INFO_OFFSET)
        !=sizeof(inst_pkg_res_info_t))
    {
        GUI_RES_LOG("res info read err!\r\n");
        return -1;
    }

    res_desc->id=ICON_RES_ID;
    res_desc->type=RES_TYPE_RGB565;
    res_desc->size=res_info.icon_size;
    res_desc->offset=res_info.icon_offset;

    GUI_RES_LOG("desc={res_id=%d,type=%d,offset=%d,size=%d,}\r\n",
        res_desc->id,res_desc->type,res_desc->offset,res_desc->size);

    return 0;
}

//获取图片资源属性--宽、高
int gui_res_get_pict_wh(uint16_t app_id,uint16_t file_id,uint32_t offset,inst_pkg_res_picture_t *pict_wh)
{
    if(!pict_wh)
        return -1;

    if(jacefs_read(file_id,app_id,(uint8_t*)pict_wh,sizeof(inst_pkg_res_picture_t),offset)
        !=sizeof(inst_pkg_res_picture_t))
    {
        GUI_RES_LOG("pict wh read err!\r\n");
        return -1;
    }

    GUI_RES_LOG("pict w=%d,h=%d\r\n",pict_wh->width,pict_wh->height);
    return 0;
}

//资源数据获取
//返回读取到的字节数
int gui_res_get_data(gui_bmp_layer_attr_t *bmp,uint32_t res_offset,uint32_t read_size,uint8_t *rdata)
{
    if(!rdata)
        return 0;

    int len;

    if(bmp->phy_addr!=0xffffffff)
    {
//        len=ext_flash_read(bmp->phy_addr+res_offset,rdata,read_size);
        memcpy(rdata,(uint8_t*)(bmp->phy_addr+res_offset),read_size);

    }
    else
    {
        //fs读取
        len=jacefs_read(bmp->file_id,bmp->app_id,(uint8_t*)rdata,read_size,bmp->offset+res_offset);
    }

//    GUI_RES_LOG("read app_id=%d,file_id=%d,offset=%d,size=%d,ret len=%d \r\n",
//        app_id,file_id,offset,size,len);

    return len;
}

//获取资源的真实物理地址
uint32_t gui_res_get_phy_addr(uint16_t app_id,uint16_t file_id,uint32_t offset)
{
    uint32_t phy_addr;

    if(jacefs_file_get_phy_addr(file_id,app_id,offset,&phy_addr)!=FS_RET_SUCCESS)
    {
        GUI_RES_LOG("res phy_addr invalid!\r\n");
    }

    return phy_addr;
}
