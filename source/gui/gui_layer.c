/*
 * gui_layer.c
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
#include "font_code_conversion.h"

#define DEBUG_GUI_LAYER 1

#if DEBUG_GUI_LAYER
#define GUI_LAYER_LOG(fmt,args...)    do{\
                                /*os_printk("%s,%s(),%d:" ,__FILE__, __FUNCTION__,__LINE__);*/\
                                os_printk("%s(),%d:" , __FUNCTION__,__LINE__);\
                                os_printk(fmt,##args);\
                            }while(0)
#define GUI_LAYER_INFO(fmt,args...)   do{\
                                os_printk(fmt,##args);\
                            }while(0)
#else
#define GUI_LAYER_LOG(fmt,args...)
#define GUI_LAYER_INFO(fmt,args...)
#endif



/**
@brief : 获取bmp资源的属性(从安装包获取)

@param :
-res_id 资源ID
-app_id 资源所属APP
-file_id 安装包所在jacefs的文件ID

@retval:
-   bmp 资源属性数据，0 失败
*/
gui_bmp_layer_attr_t *gui_layer_get_bmp_attr(uint16_t res_id,uint16_t app_id,uint16_t file_id)
{
    gui_bmp_layer_attr_t *bmp;

    inst_pkg_res_desc_t res_desc;
    inst_pkg_res_picture_t pict_wh;

    if(res_id==ICON_RES_ID)
    {
        if(gui_icon_get_desc(app_id,file_id,&res_desc))
        {
            GUI_LAYER_LOG("get icon=%d desc err!\r\n",res_id);
            return 0;
        }
    }
    else
    {
        if(gui_res_get_desc(app_id,file_id,res_id,&res_desc))
        {
            GUI_LAYER_LOG("get res=%d desc err!\r\n",res_id);
            return 0;
        }
    }

    //判断资源是否为图像，是则转换为GUI的对应类型
    switch(res_desc.type)
    {
        case RES_TYPE_RGB565:   res_desc.type=GUI_PIX_TYPE_RGB565;  break;
        case RES_TYPE_RGB888:   res_desc.type=GUI_PIX_TYPE_RGB888;  break;
        case RES_TYPE_RGB:      res_desc.type=GUI_PIX_TYPE_RGB;     break;
        case RES_TYPE_RGB3:     res_desc.type=GUI_PIX_TYPE_RGB3;    break;
        case RES_TYPE_RGB1:     res_desc.type=GUI_PIX_TYPE_RGB1;    break;
        default:
            return 0;
    }

    if(gui_res_get_pict_wh(app_id,file_id,res_desc.offset,&pict_wh))
    {
        GUI_LAYER_LOG("get res=%d w&h err!\r\n",res_id);
        return 0;
    }

    bmp=(gui_bmp_layer_attr_t *)OS_MALLOC(sizeof(gui_bmp_layer_attr_t));
    if(!bmp)
    {
        return 0;
    }

    //图片属性
    bmp->w=pict_wh.width;
    bmp->h=pict_wh.height;
    bmp->offset=res_desc.offset+sizeof(inst_pkg_res_picture_t);
    bmp->res_id=res_desc.id;
    bmp->type=res_desc.type;
    bmp->size=res_desc.size-sizeof(inst_pkg_res_picture_t);
    bmp->app_id=app_id;
    bmp->file_id=file_id;

    //获取物理地址，以便加速资源数据读取
    bmp->phy_addr=gui_res_get_phy_addr(bmp->app_id,bmp->file_id,bmp->offset);

    //TODO:宽高对不上，需要从安装包修复
    bmp->w=bmp->size/bmp->h/GUI_BYTES_PER_PIX;

    return bmp;
}



/**
@brief : 创建图片图层，layer宽度默认和图像宽高一致
                        注：
                            该函数只允许系统应用使用，外部安装的不可使用
@param :
-res_id 资源ID
-x      图层所在窗口内的x偏移
-y      图层所在窗口内的y偏移
-app_id 资源所属APP
-file_id 安装包所在jacefs的文件ID

@retval:
-   hdl 成功，0 失败
*/

gui_layer_hdl_t gui_create_bmp_layer_inner(uint16_t res_id,uint16_t x,uint16_t y,uint16_t app_id,uint16_t file_id)
{
    gui_layer_t *glayer;
    gui_bmp_layer_attr_t *bmp;

    bmp=gui_layer_get_bmp_attr(res_id,app_id,file_id);
    if(!bmp)
    {
        return 0;
    }

    glayer=(gui_layer_t *)OS_MALLOC(sizeof(gui_layer_t));
    if(!glayer)
    {
        OS_FREE(bmp);
        return 0;
    }

    //图层属性
    glayer->x=x;
    glayer->y=y;
    glayer->w=bmp->w;
    glayer->h=bmp->h;

    glayer->type=LAYER_TYPE_BITMAP;
    glayer->alignment=LAYER_ALIGN_TOP_LEFT;
    glayer->fixed=LAYER_DISABLE_FIXED;
    glayer->transparent=LAYER_DISABLE_TRANSP;
    glayer->visible=LAYER_VISIBLE;
    glayer->event_cnt=0;

    glayer->front_color=FRONT_COLOR;
    glayer->back_color=BACK_COLOR;
    glayer->trans_color=TRANSP_COLOR;

    glayer->angle=0;
    glayer->rotate_center_x=0;
    glayer->rotate_center_y=0;

    glayer->attr=bmp;

    return (gui_layer_hdl_t*)glayer;
}

/**
@brief : 创建图片图层，layer宽度默认和图像宽高一致
        注意：该函数bmp使用的是图层所属的APP的安装包内的资源

@param :
-res_id 资源ID
-x      图层所在窗口内的x偏移
-y      图层所在窗口内的y偏移

@retval:
-   hdl 成功，0 失败
*/
gui_layer_hdl_t gui_create_bmp_layer(uint16_t res_id,uint16_t x,uint16_t y)
{
    uint16_t app_id;
    app_id=os_app_get_runing();
    return gui_create_bmp_layer_inner(res_id,x,y,app_id,APP_INST_PKG_FILE_ID);
}


/**
@brief : 重新设置图片图层的图像
        注意：该函数bmp使用的是图层所属的APP的安装包内的资源

@param :
-hdl    图层句柄
-res_id 资源ID
-resize ==true 则重新根据图像大小修改图层大小，==false 则维持原大小

@retval:
-0 成功，-1 失败
*/
int gui_layer_set_bmp(gui_layer_hdl_t hdl,uint16_t res_id,bool resize)
{
    if(!hdl)
        return -1;
    gui_layer_t *layer=(gui_layer_t *)hdl;

    if(layer->type!=LAYER_TYPE_BITMAP)
        return -1;

    gui_lock();

    gui_bmp_layer_attr_t *new_bmp;
    gui_bmp_layer_attr_t *old_bmp;

    old_bmp=(gui_bmp_layer_attr_t *)layer->attr;

    //如果两次资源相同，那么不再重新设置
    if(old_bmp->res_id==res_id)
    {
        gui_unlock();
        return 0;
    }

    new_bmp=gui_layer_get_bmp_attr(res_id,old_bmp->app_id,old_bmp->file_id);

    if(!new_bmp)
    {
        gui_unlock();
        return -1;
    }

    layer->attr=new_bmp;
    OS_FREE(old_bmp);

    //修改图层大小和BMP一致
    if(resize)
    {
        layer->w=new_bmp->w;
        layer->h=new_bmp->h;
    }

    gui_unlock();

    return 0;
}

/*----------------------------------------------------------------------------------------------*/
/**
@brief : 创建像素图层

@param :
-x      图层所在窗口内的x偏移
-y      图层所在窗口内的y偏移
-w      图层宽，也是像素图像的宽
-h      图层高，也是像素图像的高
-angle  图层旋转角度，也是像素图像旋转角度
-pix_data 图像像素数据

@retval:
-   hdl 成功，0 失败
*/
gui_layer_hdl_t gui_create_pixel_layer(uint16_t x,uint16_t y,uint16_t w,uint16_t h,
                                    gui_pixel_type_t pix_type,const void *pix_data)
{
    gui_layer_t *glayer;
    gui_pixel_layer_attr_t *pix_attr;

    if(!pix_data)
        return 0;

    pix_attr=(gui_pixel_layer_attr_t *)OS_MALLOC(sizeof(gui_pixel_layer_attr_t));
    if(!pix_attr)
    {
        return 0;
    }

    glayer=(gui_layer_t *)OS_MALLOC(sizeof(gui_layer_t));
    if(!glayer)
    {
        OS_FREE(pix_attr);
        return 0;
    }

    //像素属性
    pix_attr->type=pix_type;
    pix_attr->data=pix_data;


    //图层属性
    glayer->x=x;
    glayer->y=y;
    glayer->w=w;
    glayer->h=h;

    glayer->type=LAYER_TYPE_PIXEL;
    glayer->alignment=LAYER_ALIGN_TOP_LEFT;
    glayer->fixed=LAYER_DISABLE_FIXED;
    glayer->transparent=LAYER_DISABLE_TRANSP;
    glayer->visible=LAYER_VISIBLE;
    glayer->event_cnt=0;

    glayer->front_color=FRONT_COLOR;
    glayer->back_color=BACK_COLOR;
    glayer->trans_color=TRANSP_COLOR;

    glayer->angle=0;
    glayer->rotate_center_x=0;
    glayer->rotate_center_y=0;

    glayer->attr=pix_attr;

    return (gui_layer_hdl_t*)glayer;
}

/*----------------------------------------------------------------------------------------------*/
/**
@brief : 创建文本图层
            注：text编码类型为GB2312/GBK

@param :
-x      图层所在窗口内的x偏移
-y      图层所在窗口内的y偏移
-w      图层宽，也是像素图像的宽
-h      图层高，也是像素图像的高
-size   文本字号
-text   要显示的文本

@retval:
-   hdl 成功，0 失败
*/
gui_layer_hdl_t gui_create_text_layer(uint16_t x,uint16_t y,uint16_t w,uint16_t h,
                                    uint8_t size,const char *text,text_encoding_t text_encoding)
{
    gui_layer_t *glayer;
    gui_text_layer_attr_t *text_attr;

    if(!text)
        return 0;

    text_attr=(gui_text_layer_attr_t *)OS_MALLOC(sizeof(gui_text_layer_attr_t));
    if(!text_attr)
    {
        return 0;
    }

    glayer=(gui_layer_t *)OS_MALLOC(sizeof(gui_layer_t));
    if(!glayer)
    {
        OS_FREE(text_attr);
        return 0;
    }

    //文本属性
    text_attr->size=size;

    //加载字库
    const char* font_name_str[]={
        "ascii",
        "gb2312",
        "unicode",
        "utf8",
    };
    for(int i=0;i<CHARACTER_SET_MAX;i++)
    {
        text_attr->fonts[i]=os_font_get_suitable_size(font_name_str[i],size);
    }

    text_attr->text_encoding=text_encoding;
    text_attr->text_len=0;
    text_attr->text_code=NULL;

    switch(text_attr->text_encoding)
    {
        case TEXT_ENCODING_ASCII:
        case TEXT_ENCODING_GB2312:
        {
            text_attr->text_len = font_gb2312_text_len(text);

            if (text_attr->text_len > 0)
            {
                text_attr->text_code = (uint16_t *)OS_MALLOC(sizeof(uint16_t) * text_attr->text_len);
            }
            if(text_attr->text_code==NULL)
            {
                OS_FREE(glayer);
                OS_FREE(text_attr);
                return NULL;
            }

            font_gb2312_text_fill(text,text_attr->text_code);

            break;
        }

        case TEXT_ENCODING_GB2312_2BYTES:
        {
            //这里是两个字节一个文字，所以长度直接计算
            text_attr->text_len=0;
            const uint16_t *gb2312=(const uint16_t *)text;
            while(gb2312[text_attr->text_len])
            {
                text_attr->text_len++;
            }

            if (text_attr->text_len > 0)
            {
                text_attr->text_code = (uint16_t *)OS_MALLOC(sizeof(uint16_t) * text_attr->text_len);
            }
            if(text_attr->text_code==NULL)
            {
                OS_FREE(glayer);
                OS_FREE(text_attr);
                return NULL;
            }
            memcpy(text_attr->text_code,text,text_attr->text_len*sizeof(uint16_t));
            break;
        }

        case TEXT_ENCODING_UNICODE:
        {
            //这里的 unicode 是两个字节一个文字，所以长度直接计算
            uint16_t text_len=0;
            const uint16_t *unicode=(const uint16_t *)text;
            while(unicode[text_len])
            {
                text_len++;
            }

            //现在只有中文字库，所以unicode 统一转为gb2312
            text_attr->text_encoding=TEXT_ENCODING_GB2312;

            text_attr->text_code=uicode_to_gb2312((const uint16_t *)text,text_len,&text_attr->text_len);
            if(text_attr->text_code==NULL)
            {
                OS_FREE(glayer);
                OS_FREE(text_attr);
                return NULL;
            }
            break;
        }
        case TEXT_ENCODING_UTF8:
            break;
        case TEXT_ENCODING_NONE:
        default:
        {
            break;
        }
    }

    //测试字符 编码
//    OS_LOG("text len=%d\r\n",text_attr->text_len);
//    for(int i=0;i<text_attr->text_len;i++)
//    {
//        OS_INFO(" %x",text_attr->text_code[i]);
//    }
//    OS_INFO("\r\n");


    //图层属性
    glayer->x=x;
    glayer->y=y;
    glayer->w=w;
    glayer->h=h;

    glayer->type=LAYER_TYPE_TEXT;
    glayer->alignment=LAYER_ALIGN_TOP_LEFT;
    glayer->fixed=LAYER_DISABLE_FIXED;
    glayer->transparent=LAYER_DISABLE_TRANSP;
    glayer->visible=LAYER_VISIBLE;
    glayer->event_cnt=0;

    glayer->front_color=FRONT_COLOR;
    glayer->back_color=BACK_COLOR;
    glayer->trans_color=TRANSP_COLOR;

    glayer->angle=0;
    glayer->rotate_center_x=0;
    glayer->rotate_center_y=0;

    glayer->attr=text_attr;

    return (gui_layer_hdl_t*)glayer;
}

/*----------------------------------------------------------------------------------------------*/
/**
@brief : 创建几何图层

@param :
-type  几何类型，ref @gui_geometry_t
-element 不同的type有不同的类型，对应关系为：
            GUI_GEOMETRY_POINT      point_t,
            GUI_GEOMETRY_LINE       gui_geometry_line_t,
            GUI_GEOMETRY_POLYGON    gui_geometry_polygon_t,
            GUI_GEOMETRY_CIRCLE     gui_geometry_circle_t

                            其他暂时不支持。

-fill   填充内容，ref @gui_geometry_fill_t

@retval:
-   hdl 成功，0 失败
*/
gui_layer_hdl_t gui_create_geometry_layer(gui_geometry_t type,
                                const void *element,gui_geometry_fill_t fill)
{
    gui_layer_t *glayer;
    gui_geometry_layer_attr_t *geom_attr;
    void *cp_element;

    if(!element)
    {
        return NULL;
    }
    cp_element=NULL;

    switch(type)
    {
        case GUI_GEOMETRY_POINT:
        {
            point_t *p=(point_t *)OS_MALLOC(sizeof(point_t));
            if(!p)
            {
                return NULL;
            }
            *p=*(point_t *)element;
            cp_element=p;
            break;
        }
        case GUI_GEOMETRY_LINE:
        {
            gui_geometry_line_t *line=(gui_geometry_line_t *)OS_MALLOC(sizeof(gui_geometry_line_t));
            if(!line)
            {
                return NULL;
            }
            *line=*(gui_geometry_line_t *)element;
            cp_element=line;
            break;
        }
        case GUI_GEOMETRY_POLYGON:
        {
            gui_geometry_polygon_t *src_polygon=(gui_geometry_polygon_t *)element;
            gui_geometry_polygon_t *polygon;
            if(src_polygon->point_cnt==0)
            {
                return NULL;
            }
            polygon=(gui_geometry_polygon_t *)OS_MALLOC(sizeof(gui_geometry_polygon_t));
            if(!polygon)
            {
                return NULL;
            }
            polygon->points=(point_t *)OS_MALLOC(sizeof(point_t)*src_polygon->point_cnt);
            if(!polygon->points)
            {
                OS_FREE(polygon);
                return NULL;
            }
            polygon->point_cnt=src_polygon->point_cnt;
            memcpy(polygon->points,src_polygon->points,sizeof(point_t)*src_polygon->point_cnt);

            cp_element=polygon;
            break;
        }
        case GUI_GEOMETRY_CIRCLE:
        {
            gui_geometry_circle_t *circle=(gui_geometry_circle_t *)OS_MALLOC(sizeof(gui_geometry_circle_t));
            if(!circle)
            {
                return NULL;
            }
            *circle=*(gui_geometry_circle_t *)element;
            cp_element=circle;
            break;
        }

        case GUI_GEOMETRY_ARC:  /* 不支持的类型 */
        default:
        {
            return NULL;
        }
    }

    geom_attr=(gui_geometry_layer_attr_t *)OS_MALLOC(sizeof(gui_geometry_layer_attr_t));
    if(!geom_attr)
    {
        goto geom_err;
    }

    glayer=(gui_layer_t *)OS_MALLOC(sizeof(gui_layer_t));
    if(!glayer)
    {
        OS_FREE(geom_attr);
        goto geom_err;
    }

    //像素属性
    geom_attr->fill=fill;
    geom_attr->type=type;
    geom_attr->element=cp_element;

    //图层属性
    switch(geom_attr->type)
    {
        case GUI_GEOMETRY_POINT:
        {
            point_t *p=(point_t *)cp_element;
            glayer->x=p->x;
            glayer->y=p->y;
            glayer->w=1;
            glayer->h=1;
            break;
        }
        case GUI_GEOMETRY_LINE:
        {
            gui_geometry_line_t *line=(gui_geometry_line_t *)cp_element;
            glayer->x=line->points[0].x>line->points[1].x?line->points[1].x:line->points[0].x;
            glayer->y=line->points[0].y>line->points[1].y?line->points[1].y:line->points[0].y;
            glayer->w=line->points[0].x>line->points[1].x?
                        (line->points[0].x-line->points[1].x+1):(line->points[1].x-line->points[0].x+1);
            glayer->h=line->points[0].y>line->points[1].y?
                        (line->points[0].y-line->points[1].y+1):(line->points[1].y-line->points[0].y+1);
            break;
        }
        case GUI_GEOMETRY_POLYGON:
        {
            gui_geometry_polygon_t *polygon=(gui_geometry_polygon_t *)cp_element;

            uint16_t max_x,max_y,min_x,min_y;
            max_x=min_x=polygon->points[0].x;
            max_y=min_y=polygon->points[0].y;
            for(int i=1;i<polygon->point_cnt;i++)
            {
                if(min_x>polygon->points[i].x)
                    min_x=polygon->points[i].x;
                if(min_y>polygon->points[i].y)
                    min_y=polygon->points[i].y;

                if(max_x<polygon->points[i].x)
                    max_x=polygon->points[i].x;
                if(max_y<polygon->points[i].y)
                    max_y=polygon->points[i].y;
            }
            glayer->x=min_x;
            glayer->y=min_y;
            glayer->w=max_x-min_x+1;
            glayer->h=max_y-min_y+1;
            break;
        }
        case GUI_GEOMETRY_CIRCLE:
        {
            gui_geometry_circle_t *circle=(gui_geometry_circle_t *)cp_element;
            glayer->x=circle->center.x>=circle->radius?circle->center.x-circle->radius:0;
            glayer->y=circle->center.y>=circle->radius?circle->center.y-circle->radius:0;
            glayer->w=circle->center.x+circle->radius-glayer->x+1;
            glayer->h=circle->center.y+circle->radius-glayer->y+1;
            break;
        }

//        case GUI_GEOMETRY_ARC:  /* 不支持的类型 */
//        default:
//        {
//
//        }
    }
//    OS_LOG("x=%d,y=%d,w=%d,h=%d,type=%d\r\n",glayer->x,glayer->y,glayer->w,glayer->h,type);

    glayer->type=LAYER_TYPE_GEMETRY;
    glayer->alignment=LAYER_ALIGN_TOP_LEFT;
    glayer->fixed=LAYER_DISABLE_FIXED;
    glayer->transparent=LAYER_DISABLE_TRANSP;
    glayer->visible=LAYER_VISIBLE;
    glayer->event_cnt=0;

    glayer->front_color=FRONT_COLOR;
    glayer->back_color=BACK_COLOR;
    glayer->trans_color=TRANSP_COLOR;

    glayer->angle=0;
    glayer->rotate_center_x=0;
    glayer->rotate_center_y=0;

    glayer->attr=geom_attr;

    return (gui_layer_hdl_t*)glayer;

geom_err:
    switch(type)
    {
        case GUI_GEOMETRY_POLYGON:
        {
            gui_geometry_polygon_t *polygon=(gui_geometry_polygon_t *)cp_element;
            OS_FREE(polygon->points);
            OS_FREE(polygon);
            break;
        }

        case GUI_GEOMETRY_CIRCLE:
        case GUI_GEOMETRY_POINT:
        case GUI_GEOMETRY_LINE:
        case GUI_GEOMETRY_ARC:
        default:
        {
            OS_FREE(cp_element);
            break;
        }
    }
    return NULL;
}

/*----------------------------------------------------------------------------------------------*/
/**
@brief : 创建柱状图 图层

@param :
-x  图层x在窗口的位置
-y  图层y在窗口的位置
-num  柱子数
-pillars 柱子属性@ gui_pillar_t

@retval:
-   hdl 成功，0 失败
*/
gui_layer_hdl_t gui_create_histogram_layer(uint16_t x,uint16_t y,uint16_t num,const gui_pillar_t *pillars)
{
    gui_layer_t *glayer;
    gui_histogram_layer_attr_t *histogram_attr;
    gui_pillar_t *cp_pillars;

    if(!pillars || num==0)
    {
        return NULL;
    }

    cp_pillars=(gui_pillar_t *)OS_MALLOC(sizeof(gui_pillar_t)*num);
    if(!cp_pillars)
    {
        return NULL;
    }
    memcpy(cp_pillars,pillars,sizeof(gui_pillar_t)*num);

    histogram_attr=(gui_histogram_layer_attr_t *)OS_MALLOC(sizeof(gui_histogram_layer_attr_t));
    if(!histogram_attr)
    {
        OS_FREE(cp_pillars);
        return NULL;
    }
    histogram_attr->num=num;
    histogram_attr->pillars=cp_pillars;

    glayer=(gui_layer_t *)OS_MALLOC(sizeof(gui_layer_t));
    if(!glayer)
    {
        OS_FREE(cp_pillars);
        OS_FREE(histogram_attr);
        return NULL;
    }

    //图层属性
    {
        uint16_t max_x,max_y;
        max_x=pillars[0].x+pillars[0].w;
        max_y=pillars[0].y+pillars[0].h;
        for(int i=1;i<num;i++)
        {
            if(max_x<pillars[i].x+pillars[i].w)
                max_x=pillars[i].x+pillars[i].w;
            if(max_y<pillars[i].y+pillars[i].h)
                max_y=pillars[i].y+pillars[i].h;
        }
        glayer->x=x;
        glayer->y=y;
        glayer->w=max_x;
        glayer->h=max_y;
    }
//    OS_LOG("x=%d,y=%d,w=%d,h=%d\r\n",glayer->x,glayer->y,glayer->w,glayer->h);

    glayer->type=LAYER_TYPE_HISTOGRAM;
    glayer->alignment=LAYER_ALIGN_TOP_LEFT;
    glayer->fixed=LAYER_DISABLE_FIXED;
    glayer->transparent=LAYER_DISABLE_TRANSP;
    glayer->visible=LAYER_VISIBLE;
    glayer->event_cnt=0;

    glayer->front_color=FRONT_COLOR;
    glayer->back_color=BACK_COLOR;
    glayer->trans_color=TRANSP_COLOR;

    glayer->angle=0;
    glayer->rotate_center_x=0;
    glayer->rotate_center_y=0;

    glayer->attr=histogram_attr;

    return (gui_layer_hdl_t*)glayer;
}

/*----------------------------------------------------------------------------------------------*/
/**
@brief : 设置图层在窗口的位置

@param :
-hdl    图层句柄
-x      图层所在窗口内的x偏移
-y      图层所在窗口内的y偏移

@retval:
-0 成功，-1 失败
*/
int gui_layer_set_position(gui_layer_hdl_t hdl,uint16_t x,uint16_t y)
{
    if(!hdl)
        return -1;
    gui_layer_t *layer=(gui_layer_t *)hdl;

    gui_lock();
    layer->x = x;
    layer->y = y;
    gui_unlock();

    return 0;
}

/**
@brief : 设置图层宽高

@param :
-hdl    图层句柄
-w      图层宽，如果等于0表示不修改
-h      图层高，如果等于0表示不修改

@retval:
-0 成功，-1 失败
*/
int gui_layer_set_size(gui_layer_hdl_t hdl,uint16_t w,uint16_t h)
{
    if(!hdl)
        return -1;
    gui_layer_t *layer=(gui_layer_t *)hdl;

    gui_lock();

    //目前像素图层的像素宽高使用了图层宽高，为避免出错所以不允许重新设置宽高
    //TODO：fix
    if(layer->type!=LAYER_TYPE_PIXEL)
    {
        if(w>0)
            layer->w = w;
        if(h>0)
            layer->h = h;
    }

    gui_unlock();

    return 0;
}

/**
@brief : 设置图层固定属性：图层固定在显示窗口的某个位置，不随窗口滚动而移动

@param :
-hdl    图层句柄
-fixed  ref@ gui_layer_fixed_t

@retval:
-0 成功，-1 失败
*/
int gui_layer_set_fixed(gui_layer_hdl_t hdl,gui_layer_fixed_t fixed)
{
    if(!hdl)
        return -1;
    gui_layer_t *layer=(gui_layer_t *)hdl;

    gui_lock();
    layer->fixed = fixed;
    gui_unlock();

    return 0;
}

/**
@brief : 设置图层透明

@param :
-hdl    图层句柄
-transp  ref@ gui_layer_transparent_t

@retval:
-0 成功，-1 失败
*/
int gui_layer_set_transparent(gui_layer_hdl_t hdl,gui_layer_transparent_t transp)
{
    if(!hdl)
        return -1;
    gui_layer_t *layer=(gui_layer_t *)hdl;

    gui_lock();
    layer->transparent = transp;
    gui_unlock();

    return 0;
}

/**
@brief : 设置图层可见性

@param :
-hdl    图层句柄
-visible  ref@ gui_layer_visible_t

@retval:
-0 成功，-1 失败
*/
int gui_layer_set_visible(gui_layer_hdl_t hdl,gui_layer_visible_t visible)
{
    if(!hdl)
        return -1;
    gui_layer_t *layer=(gui_layer_t *)hdl;

    gui_lock();
    layer->visible = visible;
    gui_unlock();

    return 0;
}

/**
@brief : 设置图层 前景颜色

@param :
-hdl    图层句柄
-color  要设置的颜色

@retval:
-0 成功，-1 失败
*/
int gui_layer_set_front_color(gui_layer_hdl_t hdl,uint16_t color)
{
    if(!hdl)
        return -1;
    gui_layer_t *layer=(gui_layer_t *)hdl;

    gui_lock();
    layer->front_color = color;
    gui_unlock();

    return 0;
}

/**
@brief : 设置图层 背景颜色

@param :
-hdl    图层句柄
-color  要设置的颜色

@retval:
-0 成功，-1 失败
*/
int gui_layer_set_back_color(gui_layer_hdl_t hdl,uint16_t color)
{
    if(!hdl)
        return -1;
    gui_layer_t *layer=(gui_layer_t *)hdl;

    gui_lock();
    layer->back_color = color;
    gui_unlock();

    return 0;
}

/**
@brief : 设置图层 透明颜色

@param :
-hdl    图层句柄
-color  要设置的颜色

@retval:
-0 成功，-1 失败
*/
int gui_layer_set_trans_color(gui_layer_hdl_t hdl,uint16_t color)
{
    if(!hdl)
        return -1;
    gui_layer_t *layer=(gui_layer_t *)hdl;

    gui_lock();
    layer->trans_color = color;
    gui_unlock();

    return 0;
}

/**
@brief : 设置图层在旋转角度

           屏幕如果是偶数的宽高， 旋转中心一定要取4个点的最右下角的点（图像处理的地方做了旋转修正，外部不需要修正）
    —— ——
    | | |
    —— ——
    | |x|
    —— ——

            说明：
                屏幕如果是偶数的宽高，那么旋转中心必须取4个点，旋转之后的图像才能和原来重合。
                如果只取一个点，具体偏移情况为（取旋转中心为4个点中的右下角的点）：
                    第1象限，图像不旋转，图像不偏移
                    第2象限，图像旋转90，图像偏移x+1，所以旋转中心x-1
                    第3象限，图像旋转180，图像偏移 x+1,y+1，旋转中心x-1,y-1
                    第4象限，图像旋转270，图像偏移 y+1，旋转中心y-1

@param :
-hdl        图层句柄
-angle      旋转角
-rot_x      旋转中心x坐标
-rot_y      旋转中心y坐标

@retval:
-0 成功，-1 失败
*/
int gui_layer_set_angle(gui_layer_hdl_t hdl,uint16_t angle,uint16_t rotate_center_x,uint16_t rotate_center_y)
{
    if(!hdl)
        return -1;
    gui_layer_t *layer=(gui_layer_t *)hdl;

    gui_lock();

    switch(layer->type)
    {
        case LAYER_TYPE_PIXEL:
        case LAYER_TYPE_BITMAP:
        {
            layer->angle=angle%360;
            layer->rotate_center_x=rotate_center_x;
            layer->rotate_center_y=rotate_center_y;
            break;
        }
        default:
        {
            break;
        }
    }

    gui_unlock();

    return 0;
}

/**
@brief : 添加图层事件

@param :
-hdl    图层句柄
-event  ref@ gui_event_hdl_t

@retval:
-0 成功，-1 失败
*/
static int __gui_layer_add_event(gui_layer_hdl_t hdl,gui_event_hdl_t event)
{
    if(!hdl || !event)
        return -1;

    gui_layer_t *layer=(gui_layer_t *)hdl;
    gui_event_hdl_t *new_events;

    gui_lock();
    new_events = OS_MALLOC(sizeof(gui_event_hdl_t) * (layer->event_cnt + 1));
    if (NULL == new_events)
    {
        gui_unlock();
        return -1;
    }

    if (layer->event_cnt>0)
    {
        memcpy(new_events, layer->events, sizeof(gui_event_hdl_t)*(layer->event_cnt));
        OS_FREE(layer->events);
    }
    new_events[layer->event_cnt] = event;

    layer->event_cnt++;
    layer->events=new_events;

    gui_unlock();
    return 0;
}

/**
@brief : 订阅图层事件

@param :
-hdl        图层句柄
-evt_type   事件类型 ref@ gui_event_type_t
-cb         ref@ gui_event_call_back_t

@retval:
-0 成功，-1 失败
*/
int gui_layer_subscribe_event(gui_layer_hdl_t hdl,
    gui_event_type_t evt_type,
    gui_event_call_back_t cb)
{
    if(!hdl || evt_type==GUI_EVT_NONE || !cb)
        return -1;

    gui_event_t *new_event;

    gui_lock();
    new_event = OS_MALLOC(sizeof(gui_event_t));
    if (NULL == new_event)
    {
        gui_unlock();
        return -1;
    }

    new_event->cb=cb;
    new_event->type=evt_type;

    if(__gui_layer_add_event(hdl,(gui_event_hdl_t)new_event))
    {
        OS_FREE(new_event);
        gui_unlock();
        return -1;
    }

    gui_unlock();
    return 0;
}

/**
@brief : 删除图层，释放内存

@param :
-hdl        图层句柄

@retval:
-0 成功，-1 失败
*/
int gui_layer_destroy(gui_layer_hdl_t hdl)
{
    if(!hdl)
        return -1;
    gui_layer_t *layer=(gui_layer_t *)hdl;
    int i;

    gui_lock();

    //事件
    if(layer->event_cnt>0)
    {
        for(i=0;i<layer->event_cnt;i++)
        {
            OS_FREE(layer->events[i]);
        }
        OS_FREE(layer->events);
    }

    //属性
    switch(layer->type)
    {
        case LAYER_TYPE_BITMAP:
        case LAYER_TYPE_PIXEL:
        {
            OS_FREE(layer->attr);
            break;
        }

        case LAYER_TYPE_TEXT:
        {
            gui_text_layer_attr_t *txt=(gui_text_layer_attr_t*)layer->attr;
            if(txt->text_code)
                OS_FREE(txt->text_code);
            OS_FREE(layer->attr);
            break;
        }

        case LAYER_TYPE_GEMETRY:
        {
            gui_geometry_layer_attr_t *geometry=(gui_geometry_layer_attr_t*)layer->attr;
            if(geometry->element)
                OS_FREE(geometry->element);
            OS_FREE(layer->attr);
            break;
        }

        case LAYER_TYPE_HISTOGRAM:
        {
            gui_histogram_layer_attr_t *_attr=(gui_histogram_layer_attr_t*)layer->attr;
            if(_attr->pillars)
                OS_FREE(_attr->pillars);
            OS_FREE(layer->attr);
            break;
        }

        //TODO：其他图层根据具体情况释放空间
    }

    OS_FREE(layer);

    gui_unlock();
    return 0;
}
