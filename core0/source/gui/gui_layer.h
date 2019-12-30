/*
 * gui_layer.h
 *
 *  Created on: 2019��7��11��
 *      Author: jace
 */

#ifndef WEAR_SRC_GUI_GUI_LAYER_H_
#define WEAR_SRC_GUI_GUI_LAYER_H_

#include "gui_event.h"
#include "font_mgr.h"

/*
 * ͼ������
 */
typedef enum
{
    LAYER_TYPE_BITMAP,  //λͼ
    LAYER_TYPE_PIXEL,   //����
    LAYER_TYPE_TEXT,    //�ı�
    LAYER_TYPE_GEMETRY, //����
    LAYER_TYPE_HISTOGRAM,
}gui_layer_type_t;

/*
 * ��������
 */
typedef enum{
    GUI_PIX_TYPE_UNKNOW ,//=RES_TYPE_UNKNOW,
    GUI_PIX_TYPE_RGB565 ,//=RES_TYPE_RGB565,
    GUI_PIX_TYPE_RGB888 ,//=RES_TYPE_RGB888,
    GUI_PIX_TYPE_RGB    ,//=RES_TYPE_RGB   ,
    GUI_PIX_TYPE_RGB3   ,//=RES_TYPE_RGB3  ,
    GUI_PIX_TYPE_RGB1   ,//=RES_TYPE_RGB1  ,
}gui_pixel_type_t;

//----------------------------------------------------------------
/*
 *  ͼƬͼ��
 */
typedef struct
{
    uint16_t app_id;    //ͼƬ��������������װ��������
    uint16_t file_id;

    uint16_t type;      //ͼƬ����,ref @gui_pixel_type_t
    uint16_t res_id;    //ͼƬ��ԴID
    uint32_t size;      //ͼƬ��С���ֽڣ�
    uint32_t offset;    //ͼƬ�����ڰ�װ����ƫ��λ��

    uint32_t phy_addr;  //������flash����ʵ�����ַ�����ڼ��ٶ�ȡ

    uint16_t w;         //���ظ߶�
    uint16_t h;         //���ؿ��

}gui_bmp_layer_attr_t;

//----------------------------------------------------------------
/*
 * ����ͼ�� ����
 */
typedef struct
{
    uint16_t type;          //�������� @ref gui_pixel_type_t
//    uint16_t w;             //���ظ߶�
//    uint16_t h;             //���ؿ��

    const void *data;   //��������
}gui_pixel_layer_attr_t;

//----------------------------------------------------------------

/* �ı�ͼ�� */
typedef enum{
    CHARACTER_SET_ASCII=0,
    CHARACTER_SET_GB2312,
    CHARACTER_SET_UNICODE,
    CHARACTER_SET_UTF8,
    CHARACTER_SET_MAX,
}character_set_t;

typedef enum{
    TEXT_ENCODING_NONE,
    TEXT_ENCODING_ASCII,
    TEXT_ENCODING_GB2312,
    TEXT_ENCODING_GB2312_2BYTES,
    TEXT_ENCODING_UNICODE,
    TEXT_ENCODING_UTF8,
}text_encoding_t;


typedef struct
{
    uint8_t text_encoding;      /* �������� @ */
    uint8_t size;
    const font_info_t *fonts[CHARACTER_SET_MAX];

//    const char *text;       //����
    uint16_t text_len;
    uint16_t *text_code;
}gui_text_layer_attr_t;


//----------------------------------------------------------------
/*
 * ����ͼ��
 *
 * TODO��
 *      ����ʹ������������������������ͼ��--ʹ�õ����������ͼ��
 */

typedef enum{
    GUI_GEOMETRY_POINT,
    GUI_GEOMETRY_LINE,
    GUI_GEOMETRY_POLYGON,
    GUI_GEOMETRY_CIRCLE,
    GUI_GEOMETRY_ARC,
}gui_geometry_t;

typedef enum{
    GEOM_FILL_OUT_LINE, //���߿�
    GEOM_FILL_AREA,     //�������
}gui_geometry_fill_t;

typedef struct {            //��
    uint16_t x;
    uint16_t y;
}point_t;

typedef struct {            //��
    point_t points[2];
}gui_geometry_line_t;

typedef struct {            //����Σ����ǣ��ıߣ���ǣ����ǡ�����
    uint16_t point_cnt;     /* ����ζ������ */
    point_t *points;        /* ����ζ��㣺�����ǰ����±�˳������ */
}gui_geometry_polygon_t;

typedef struct {            //Բ
    uint16_t radius;       /* �뾶 */
    point_t center;        /* Բ�� */
}gui_geometry_circle_t;

typedef struct
{
    uint16_t type;
    uint16_t fill:1;        //ref @gui_geometry_fill_t
    uint16_t reserved:16-1;
    void *element;          //��ͬ�� type ��Ӧ��ͬ��Ԫ��
}gui_geometry_layer_attr_t;

//----------------------------------------------------------------
/*
 * ��״ͼ
 */
typedef struct
{
    uint16_t x;     /* �������ͼ���ڵ����꣬���Ǵ��ڵ����� */
    uint16_t y;
    uint16_t w;
    uint16_t h;
    uint16_t color;
}gui_pillar_t;

typedef struct
{
    uint16_t num;
    gui_pillar_t *pillars;
}gui_histogram_layer_attr_t;


//----------------------------------------------------------------

/*
 * ͼ��
 */

typedef enum{
    LAYER_ALIGN_TOP_LEFT,   //����
    LAYER_ALIGN_TOP_RIGHT,  //����
    LAYER_ALIGN_TOP_CENTER, //�Ͼ���

    LAYER_ALIGN_CENTER, //����

    LAYER_ALIGN_BOTTOM_LEFT,   //����
    LAYER_ALIGN_BOTTOM_RIGHT,  //����
    LAYER_ALIGN_BOTTOM_CENTER, //�¾���

}gui_layer_alignment_t;

typedef enum{
    LAYER_DISABLE_FIXED,
    LAYER_ENABLE_FIXED
}gui_layer_fixed_t;

typedef enum{
    LAYER_DISABLE_TRANSP,
    LAYER_ENABLE_TRANSP
}gui_layer_transparent_t;

typedef enum{
    LAYER_INVISIBLE,
    LAYER_VISIBLE
}gui_layer_visible_t;

typedef struct {
    uint16_t type;          /* ͼ������ ref@gui_layer_type_t*/
    uint16_t x, y;          /* �ڴ����ڵ���ʾ��ʼλ�� */
    uint16_t w, h;          /* ͼ���С��ͼ��ɳ������ڷ�Χ�����������ֲ���ʾ */

    uint16_t alignment:4;   /* ���뷽ʽ ref@gui_layer_alignment_t */
    uint16_t fixed:1;       /* �̶����ܣ��̶��ڴ�����ʾ����  ref@gui_layer_fixed_t */
    uint16_t transparent:1; /* ����͸�� ref@gui_layer_transparent_t ( ����ͼ��ĺ�ɫ/λͼͼ���ɫ/�ı�ͼ���ɫ  ����ʾ)*/
    uint16_t visible:1;     /* ͼ��ɼ� ref@gui_layer_visible_t */
    uint16_t reserved:16-7;

    uint16_t front_color;   /* ǰ��ɫ (ֻ����  ����ͼ��ĵ�ɫ����-rgb1/�ı�ͼ��/����ͼ��  ��Ч��λͼͼ�� ����)*/
    uint16_t back_color;    /* ����ɫ (ֻ����  ����ͼ��ĵ�ɫ����-rgb1/�ı�ͼ��  ��Ч��λͼͼ��/����ͼ�� ����)*/
    uint16_t trans_color;   /* ͸��ɫ (ֻ����  ����ͼ�㡢λͼͼ��  ��Ч���������͸��-alignment����ô��ɫ��trans_color��ͬ�Ľ�����ʾ)*/

    /*
     * ��ת����Ŀǰֻ֧�� ͼ��ͼ�㡢����ͼ�㣨rgb565��
     */
    uint16_t angle;            //��ת�Ƕ�
    uint16_t rotate_center_x;  //��ת����x�����ֵ��ͼ���ڵ�ƫ��ֵ��
    uint16_t rotate_center_y;  //��ת����y�����ֵ��ͼ���ڵ�ƫ��ֵ��

    void *attr;             /* ͼ������*/
    uint8_t event_cnt;      /* ͼ���¼��� */
    gui_event_hdl_t *events;/* ͼ���¼����� */
}gui_layer_t;

typedef void * gui_layer_hdl_t;

//----------------------------------------------------------------

#define RGB888_TO_RGB565(x) ( ((((x)>>19)&0x1f)<<11) | ((((x)>>10)&0x3f)<<5) | (((x)>>3) &0x1f) )

//ͼƬͼ��ʹ�ýӿ�
gui_bmp_layer_attr_t *gui_layer_get_bmp_attr(uint16_t res_id,uint16_t app_id,uint16_t file_id);
gui_layer_hdl_t gui_create_bmp_layer_inner(uint16_t res_id,uint16_t x,uint16_t y,uint16_t app_id,uint16_t file_id);

#define gui_create_bmp_layerk(res_id,x,y) \
    gui_create_bmp_layer_inner((res_id),(x),(y),SYS_APP_ID_RES,APP_INST_PKG_FILE_ID)

gui_layer_hdl_t gui_create_bmp_layer(uint16_t res_id,uint16_t x,uint16_t y);
int gui_layer_set_bmp(gui_layer_hdl_t hdl,uint16_t res_id,bool resize);

//����ͼ��
gui_layer_hdl_t gui_create_pixel_layer(uint16_t x,uint16_t y,uint16_t w,uint16_t h,
                                    gui_pixel_type_t pix_type,const void *pix_data);

//�ı�ͼ��
gui_layer_hdl_t gui_create_text_layer(uint16_t x,uint16_t y,uint16_t w,uint16_t h,
                                    uint8_t size,const char *text,text_encoding_t text_encoding);

//����ͼ��
gui_layer_hdl_t gui_create_geometry_layer(gui_geometry_t type,
                                const void *element,gui_geometry_fill_t fill);

//��״ͼ ͼ��
gui_layer_hdl_t gui_create_histogram_layer(uint16_t x,uint16_t y,uint16_t num,const gui_pillar_t *pillars);

//ͼ��ͨ�ýӿ�
int gui_layer_set_position(gui_layer_hdl_t hdl,uint16_t x,uint16_t y);
int gui_layer_set_size(gui_layer_hdl_t hdl,uint16_t w,uint16_t h);
int gui_layer_set_fixed(gui_layer_hdl_t hdl,gui_layer_fixed_t fixed);
int gui_layer_set_transparent(gui_layer_hdl_t hdl,gui_layer_transparent_t transp);
int gui_layer_set_visible(gui_layer_hdl_t hdl,gui_layer_visible_t visible);
int gui_layer_set_front_color(gui_layer_hdl_t hdl,uint16_t color);
int gui_layer_set_back_color(gui_layer_hdl_t hdl,uint16_t color);
int gui_layer_set_trans_color(gui_layer_hdl_t hdl,uint16_t color);

int gui_layer_set_angle(gui_layer_hdl_t hdl,uint16_t angle,uint16_t rotate_center_x,uint16_t rotate_center_y);

int gui_layer_subscribe_event(gui_layer_hdl_t hdl,
    gui_event_type_t evt_type,
    gui_event_call_back_t cb);

int gui_layer_destroy(gui_layer_hdl_t hdl);


#endif /* WEAR_SRC_GUI_GUI_LAYER_H_ */
