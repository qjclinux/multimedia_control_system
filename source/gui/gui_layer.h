/*
 * gui_layer.h
 *
 *  Created on: 2019年7月11日
 *      Author: jace
 */

#ifndef WEAR_SRC_GUI_GUI_LAYER_H_
#define WEAR_SRC_GUI_GUI_LAYER_H_

#include "gui_event.h"
#include "font_mgr.h"

/*
 * 图层类型
 */
typedef enum
{
    LAYER_TYPE_BITMAP,  //位图
    LAYER_TYPE_PIXEL,   //像素
    LAYER_TYPE_TEXT,    //文本
    LAYER_TYPE_GEMETRY, //几何
    LAYER_TYPE_HISTOGRAM,
}gui_layer_type_t;

/*
 * 像素类型
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
 *  图片图层
 */
typedef struct
{
    uint16_t app_id;    //图片可以引用其他安装包的内容
    uint16_t file_id;

    uint16_t type;      //图片类型,ref @gui_pixel_type_t
    uint16_t res_id;    //图片资源ID
    uint32_t size;      //图片大小（字节）
    uint32_t offset;    //图片数据在安装包的偏移位置

    uint32_t phy_addr;  //数据在flash的真实物理地址，用于加速读取

    uint16_t w;         //像素高度
    uint16_t h;         //像素宽度

}gui_bmp_layer_attr_t;

//----------------------------------------------------------------
/*
 * 像素图层 数据
 */
typedef struct
{
    uint16_t type;          //像素类型 @ref gui_pixel_type_t
//    uint16_t w;             //像素高度
//    uint16_t h;             //像素宽度

    const void *data;   //像素数据
}gui_pixel_layer_attr_t;

//----------------------------------------------------------------

/* 文本图层 */
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
    uint8_t text_encoding;      /* 文字类型 @ */
    uint8_t size;
    const font_info_t *fonts[CHARACTER_SET_MAX];

//    const char *text;       //文字
    uint16_t text_len;
    uint16_t *text_code;
}gui_text_layer_attr_t;


//----------------------------------------------------------------
/*
 * 几何图形
 *
 * TODO：
 *      考虑使用像内塞尔曲线那样来描述图形--使用点和线来描述图形
 */

typedef enum{
    GUI_GEOMETRY_POINT,
    GUI_GEOMETRY_LINE,
    GUI_GEOMETRY_POLYGON,
    GUI_GEOMETRY_CIRCLE,
    GUI_GEOMETRY_ARC,
}gui_geometry_t;

typedef enum{
    GEOM_FILL_OUT_LINE, //填充边框
    GEOM_FILL_AREA,     //填充区域
}gui_geometry_fill_t;

typedef struct {            //点
    uint16_t x;
    uint16_t y;
}point_t;

typedef struct {            //线
    point_t points[2];
}gui_geometry_line_t;

typedef struct {            //多边形：三角，四边，五角，六角。。。
    uint16_t point_cnt;     /* 多边形顶点个数 */
    point_t *points;        /* 多边形顶点：现在是按点下标顺序连接 */
}gui_geometry_polygon_t;

typedef struct {            //圆
    uint16_t radius;       /* 半径 */
    point_t center;        /* 圆心 */
}gui_geometry_circle_t;

typedef struct
{
    uint16_t type;
    uint16_t fill:1;        //ref @gui_geometry_fill_t
    uint16_t reserved:16-1;
    void *element;          //不同的 type 对应不同的元素
}gui_geometry_layer_attr_t;

//----------------------------------------------------------------
/*
 * 柱状图
 */
typedef struct
{
    uint16_t x;     /* 这个是在图层内的坐标，不是窗口的坐标 */
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
 * 图层
 */

typedef enum{
    LAYER_ALIGN_TOP_LEFT,   //左上
    LAYER_ALIGN_TOP_RIGHT,  //右上
    LAYER_ALIGN_TOP_CENTER, //上居中

    LAYER_ALIGN_CENTER, //居中

    LAYER_ALIGN_BOTTOM_LEFT,   //左下
    LAYER_ALIGN_BOTTOM_RIGHT,  //右下
    LAYER_ALIGN_BOTTOM_CENTER, //下居中

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
    uint16_t type;          /* 图层类型 ref@gui_layer_type_t*/
    uint16_t x, y;          /* 在窗口内的显示起始位置 */
    uint16_t w, h;          /* 图层大小：图层可超出窗口范围，但超出部分不显示 */

    uint16_t alignment:4;   /* 对齐方式 ref@gui_layer_alignment_t */
    uint16_t fixed:1;       /* 固定功能：固定在窗口显示区域  ref@gui_layer_fixed_t */
    uint16_t transparent:1; /* 背景透明 ref@gui_layer_transparent_t ( 像素图像的黑色/位图图层黑色/文本图层黑色  不显示)*/
    uint16_t visible:1;     /* 图层可见 ref@gui_layer_visible_t */
    uint16_t reserved:16-7;

    uint16_t front_color;   /* 前景色 (只对于  像素图层的单色像素-rgb1/文本图层/几何图层  有效，位图图层 无用)*/
    uint16_t back_color;    /* 背景色 (只对于  像素图层的单色像素-rgb1/文本图层  有效，位图图层/几何图层 无用)*/
    uint16_t trans_color;   /* 透明色 (只对于  像素图层、位图图层  有效，如果打开了透明-alignment，那么颜色和trans_color相同的将不显示)*/

    /*
     * 旋转功能目前只支持 图像图层、像素图层（rgb565）
     */
    uint16_t angle;            //旋转角度
    uint16_t rotate_center_x;  //旋转中心x（相对值，图层内的偏移值）
    uint16_t rotate_center_y;  //旋转中心y（相对值，图层内的偏移值）

    void *attr;             /* 图层属性*/
    uint8_t event_cnt;      /* 图层事件数 */
    gui_event_hdl_t *events;/* 图层事件数据 */
}gui_layer_t;

typedef void * gui_layer_hdl_t;

//----------------------------------------------------------------

#define RGB888_TO_RGB565(x) ( ((((x)>>19)&0x1f)<<11) | ((((x)>>10)&0x3f)<<5) | (((x)>>3) &0x1f) )

//图片图层使用接口
gui_bmp_layer_attr_t *gui_layer_get_bmp_attr(uint16_t res_id,uint16_t app_id,uint16_t file_id);
gui_layer_hdl_t gui_create_bmp_layer_inner(uint16_t res_id,uint16_t x,uint16_t y,uint16_t app_id,uint16_t file_id);

#define gui_create_bmp_layerk(res_id,x,y) \
    gui_create_bmp_layer_inner((res_id),(x),(y),SYS_APP_ID_RES,APP_INST_PKG_FILE_ID)

gui_layer_hdl_t gui_create_bmp_layer(uint16_t res_id,uint16_t x,uint16_t y);
int gui_layer_set_bmp(gui_layer_hdl_t hdl,uint16_t res_id,bool resize);

//像素图层
gui_layer_hdl_t gui_create_pixel_layer(uint16_t x,uint16_t y,uint16_t w,uint16_t h,
                                    gui_pixel_type_t pix_type,const void *pix_data);

//文本图层
gui_layer_hdl_t gui_create_text_layer(uint16_t x,uint16_t y,uint16_t w,uint16_t h,
                                    uint8_t size,const char *text,text_encoding_t text_encoding);

//几何图层
gui_layer_hdl_t gui_create_geometry_layer(gui_geometry_t type,
                                const void *element,gui_geometry_fill_t fill);

//柱状图 图层
gui_layer_hdl_t gui_create_histogram_layer(uint16_t x,uint16_t y,uint16_t num,const gui_pillar_t *pillars);

//图层通用接口
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
