/*
 * gui_layer.c
 *
 *  Created on: 2019��7��11��
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
@brief : ��ȡbmp��Դ������(�Ӱ�װ����ȡ)

@param :
-res_id ��ԴID
-app_id ��Դ����APP
-file_id ��װ������jacefs���ļ�ID

@retval:
-   bmp ��Դ�������ݣ�0 ʧ��
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

    //�ж���Դ�Ƿ�Ϊͼ������ת��ΪGUI�Ķ�Ӧ����
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

    //ͼƬ����
    bmp->w=pict_wh.width;
    bmp->h=pict_wh.height;
    bmp->offset=res_desc.offset+sizeof(inst_pkg_res_picture_t);
    bmp->res_id=res_desc.id;
    bmp->type=res_desc.type;
    bmp->size=res_desc.size-sizeof(inst_pkg_res_picture_t);
    bmp->app_id=app_id;
    bmp->file_id=file_id;

    //��ȡ�����ַ���Ա������Դ���ݶ�ȡ
    bmp->phy_addr=gui_res_get_phy_addr(bmp->app_id,bmp->file_id,bmp->offset);

    //TODO:��߶Բ��ϣ���Ҫ�Ӱ�װ���޸�
    bmp->w=bmp->size/bmp->h/GUI_BYTES_PER_PIX;

    return bmp;
}



/**
@brief : ����ͼƬͼ�㣬layer���Ĭ�Ϻ�ͼ����һ��
                        ע��
                            �ú���ֻ����ϵͳӦ��ʹ�ã��ⲿ��װ�Ĳ���ʹ��
@param :
-res_id ��ԴID
-x      ͼ�����ڴ����ڵ�xƫ��
-y      ͼ�����ڴ����ڵ�yƫ��
-app_id ��Դ����APP
-file_id ��װ������jacefs���ļ�ID

@retval:
-   hdl �ɹ���0 ʧ��
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

    //ͼ������
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
@brief : ����ͼƬͼ�㣬layer���Ĭ�Ϻ�ͼ����һ��
        ע�⣺�ú���bmpʹ�õ���ͼ��������APP�İ�װ���ڵ���Դ

@param :
-res_id ��ԴID
-x      ͼ�����ڴ����ڵ�xƫ��
-y      ͼ�����ڴ����ڵ�yƫ��

@retval:
-   hdl �ɹ���0 ʧ��
*/
gui_layer_hdl_t gui_create_bmp_layer(uint16_t res_id,uint16_t x,uint16_t y)
{
    uint16_t app_id;
    app_id=os_app_get_runing();
    return gui_create_bmp_layer_inner(res_id,x,y,app_id,APP_INST_PKG_FILE_ID);
}


/**
@brief : ��������ͼƬͼ���ͼ��
        ע�⣺�ú���bmpʹ�õ���ͼ��������APP�İ�װ���ڵ���Դ

@param :
-hdl    ͼ����
-res_id ��ԴID
-resize ==true �����¸���ͼ���С�޸�ͼ���С��==false ��ά��ԭ��С

@retval:
-0 �ɹ���-1 ʧ��
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

    //���������Դ��ͬ����ô������������
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

    //�޸�ͼ���С��BMPһ��
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
@brief : ��������ͼ��

@param :
-x      ͼ�����ڴ����ڵ�xƫ��
-y      ͼ�����ڴ����ڵ�yƫ��
-w      ͼ���Ҳ������ͼ��Ŀ�
-h      ͼ��ߣ�Ҳ������ͼ��ĸ�
-angle  ͼ����ת�Ƕȣ�Ҳ������ͼ����ת�Ƕ�
-pix_data ͼ����������

@retval:
-   hdl �ɹ���0 ʧ��
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

    //��������
    pix_attr->type=pix_type;
    pix_attr->data=pix_data;


    //ͼ������
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
@brief : �����ı�ͼ��
            ע��text��������ΪGB2312/GBK

@param :
-x      ͼ�����ڴ����ڵ�xƫ��
-y      ͼ�����ڴ����ڵ�yƫ��
-w      ͼ���Ҳ������ͼ��Ŀ�
-h      ͼ��ߣ�Ҳ������ͼ��ĸ�
-size   �ı��ֺ�
-text   Ҫ��ʾ���ı�

@retval:
-   hdl �ɹ���0 ʧ��
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

    //�ı�����
    text_attr->size=size;

    //�����ֿ�
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
            //�����������ֽ�һ�����֣����Գ���ֱ�Ӽ���
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
            //����� unicode �������ֽ�һ�����֣����Գ���ֱ�Ӽ���
            uint16_t text_len=0;
            const uint16_t *unicode=(const uint16_t *)text;
            while(unicode[text_len])
            {
                text_len++;
            }

            //����ֻ�������ֿ⣬����unicode ͳһתΪgb2312
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

    //�����ַ� ����
//    OS_LOG("text len=%d\r\n",text_attr->text_len);
//    for(int i=0;i<text_attr->text_len;i++)
//    {
//        OS_INFO(" %x",text_attr->text_code[i]);
//    }
//    OS_INFO("\r\n");


    //ͼ������
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
@brief : ��������ͼ��

@param :
-type  �������ͣ�ref @gui_geometry_t
-element ��ͬ��type�в�ͬ�����ͣ���Ӧ��ϵΪ��
            GUI_GEOMETRY_POINT      point_t,
            GUI_GEOMETRY_LINE       gui_geometry_line_t,
            GUI_GEOMETRY_POLYGON    gui_geometry_polygon_t,
            GUI_GEOMETRY_CIRCLE     gui_geometry_circle_t

                            ������ʱ��֧�֡�

-fill   ������ݣ�ref @gui_geometry_fill_t

@retval:
-   hdl �ɹ���0 ʧ��
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

        case GUI_GEOMETRY_ARC:  /* ��֧�ֵ����� */
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

    //��������
    geom_attr->fill=fill;
    geom_attr->type=type;
    geom_attr->element=cp_element;

    //ͼ������
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

//        case GUI_GEOMETRY_ARC:  /* ��֧�ֵ����� */
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
@brief : ������״ͼ ͼ��

@param :
-x  ͼ��x�ڴ��ڵ�λ��
-y  ͼ��y�ڴ��ڵ�λ��
-num  ������
-pillars ��������@ gui_pillar_t

@retval:
-   hdl �ɹ���0 ʧ��
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

    //ͼ������
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
@brief : ����ͼ���ڴ��ڵ�λ��

@param :
-hdl    ͼ����
-x      ͼ�����ڴ����ڵ�xƫ��
-y      ͼ�����ڴ����ڵ�yƫ��

@retval:
-0 �ɹ���-1 ʧ��
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
@brief : ����ͼ����

@param :
-hdl    ͼ����
-w      ͼ����������0��ʾ���޸�
-h      ͼ��ߣ��������0��ʾ���޸�

@retval:
-0 �ɹ���-1 ʧ��
*/
int gui_layer_set_size(gui_layer_hdl_t hdl,uint16_t w,uint16_t h)
{
    if(!hdl)
        return -1;
    gui_layer_t *layer=(gui_layer_t *)hdl;

    gui_lock();

    //Ŀǰ����ͼ������ؿ��ʹ����ͼ���ߣ�Ϊ����������Բ������������ÿ��
    //TODO��fix
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
@brief : ����ͼ��̶����ԣ�ͼ��̶�����ʾ���ڵ�ĳ��λ�ã����洰�ڹ������ƶ�

@param :
-hdl    ͼ����
-fixed  ref@ gui_layer_fixed_t

@retval:
-0 �ɹ���-1 ʧ��
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
@brief : ����ͼ��͸��

@param :
-hdl    ͼ����
-transp  ref@ gui_layer_transparent_t

@retval:
-0 �ɹ���-1 ʧ��
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
@brief : ����ͼ��ɼ���

@param :
-hdl    ͼ����
-visible  ref@ gui_layer_visible_t

@retval:
-0 �ɹ���-1 ʧ��
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
@brief : ����ͼ�� ǰ����ɫ

@param :
-hdl    ͼ����
-color  Ҫ���õ���ɫ

@retval:
-0 �ɹ���-1 ʧ��
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
@brief : ����ͼ�� ������ɫ

@param :
-hdl    ͼ����
-color  Ҫ���õ���ɫ

@retval:
-0 �ɹ���-1 ʧ��
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
@brief : ����ͼ�� ͸����ɫ

@param :
-hdl    ͼ����
-color  Ҫ���õ���ɫ

@retval:
-0 �ɹ���-1 ʧ��
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
@brief : ����ͼ������ת�Ƕ�

           ��Ļ�����ż���Ŀ�ߣ� ��ת����һ��Ҫȡ4����������½ǵĵ㣨ͼ����ĵط�������ת�������ⲿ����Ҫ������
    ���� ����
    | | |
    ���� ����
    | |x|
    ���� ����

            ˵����
                ��Ļ�����ż���Ŀ�ߣ���ô��ת���ı���ȡ4���㣬��ת֮���ͼ����ܺ�ԭ���غϡ�
                ���ֻȡһ���㣬����ƫ�����Ϊ��ȡ��ת����Ϊ4�����е����½ǵĵ㣩��
                    ��1���ޣ�ͼ����ת��ͼ��ƫ��
                    ��2���ޣ�ͼ����ת90��ͼ��ƫ��x+1��������ת����x-1
                    ��3���ޣ�ͼ����ת180��ͼ��ƫ�� x+1,y+1����ת����x-1,y-1
                    ��4���ޣ�ͼ����ת270��ͼ��ƫ�� y+1����ת����y-1

@param :
-hdl        ͼ����
-angle      ��ת��
-rot_x      ��ת����x����
-rot_y      ��ת����y����

@retval:
-0 �ɹ���-1 ʧ��
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
@brief : ���ͼ���¼�

@param :
-hdl    ͼ����
-event  ref@ gui_event_hdl_t

@retval:
-0 �ɹ���-1 ʧ��
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
@brief : ����ͼ���¼�

@param :
-hdl        ͼ����
-evt_type   �¼����� ref@ gui_event_type_t
-cb         ref@ gui_event_call_back_t

@retval:
-0 �ɹ���-1 ʧ��
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
@brief : ɾ��ͼ�㣬�ͷ��ڴ�

@param :
-hdl        ͼ����

@retval:
-0 �ɹ���-1 ʧ��
*/
int gui_layer_destroy(gui_layer_hdl_t hdl)
{
    if(!hdl)
        return -1;
    gui_layer_t *layer=(gui_layer_t *)hdl;
    int i;

    gui_lock();

    //�¼�
    if(layer->event_cnt>0)
    {
        for(i=0;i<layer->event_cnt;i++)
        {
            OS_FREE(layer->events[i]);
        }
        OS_FREE(layer->events);
    }

    //����
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

        //TODO������ͼ����ݾ�������ͷſռ�
    }

    OS_FREE(layer);

    gui_unlock();
    return 0;
}
