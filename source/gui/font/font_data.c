/*
 * font_convert.c
 *
 *  Created on: 2019年8月13日
 *      Author: jace
 */

#include "console.h"
#include "crc16.h"
#include "string.h"
#include "osal.h"
#include "jacefs.h"
#include "res_type.h"
#include <font_data.h>

#define FONT_INVALID_OFFSET (0xffffffff)

//获取GB2312的数据偏移
uint32_t font_gb2312_offset(uint16_t gb2312,uint16_t bytes_per_word)
{
    uint16_t qh = (gb2312&0xff00)>>8;
    uint16_t ql = (gb2312&0x00ff);
    uint32_t offset=FONT_INVALID_OFFSET;

    if(qh>=0xa1/*0xB0*/&&qh<=0xF7&&ql>=0xA1&&ql<=0xFE){
        offset = (uint32_t)(((qh - 0xa1/*0xB0*/) * 94 + ql - 0xA1)*bytes_per_word);
    }

    return offset;
}

//从文件系统中获取中文点阵(gb2312)
//返回 0 成功，-1 失败
int font_gb2312_data(uint16_t gb2312,const font_info_t *font,uint8_t *data)
{
    //测试用
//    const uint8_t c_qi[]={
//        //U+51C4(凄)
//        0x00,0x00,0x00,0x00,0x60,0x00,0x00,0x40,0x00,0x00,0x40,0x00,0x08,0xF8,0x1F,0x10,
//        0x46,0x00,0x20,0x40,0x0C,0x00,0xFC,0x07,0x80,0x40,0x04,0x40,0x40,0x7E,0xC0,0xFF,
//        0x05,0x40,0x40,0x04,0x20,0xC0,0x07,0x20,0x3C,0x00,0x20,0x30,0x00,0x1C,0xD0,0x7F,
//        0xD8,0x2F,0x01,0x10,0x88,0x01,0x10,0x8C,0x00,0x00,0x70,0x00,0x00,0xA0,0x03,0x00,
//        0x18,0x0C,0x00,0x06,0x00,0xC0,0x01,0x00
//    };
//    memcpy(data,c_qi,font->font_attr.bytes_per_word);
//    return 0;

    uint32_t offset;

    if(!font || !data)
    {
        return -1;
    }

    offset=font_gb2312_offset(gb2312,font->font_attr.bytes_per_word);
    if(offset==FONT_INVALID_OFFSET)
    {
        return -1;
    }

    if(jacefs_read(font->file_id,font->app_id,data,
            font->font_attr.bytes_per_word,
            font->offset+FONT_DATA_HDR_SIZE+offset )
        !=font->font_attr.bytes_per_word)
    {
        return -1;
    }

    return 0;
}

//从文件系统中获取英文点阵(ascii)
//返回 0 成功，-1 失败
int font_ascii_data(uint16_t c,const font_info_t *font,uint8_t *data)
{
    //测试用
//    const uint8_t c5[]={
//        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF8,0x07,0x00,0xFC,
//        0x03,0x00,0x04,0x00,0x00,0x04,0x00,0x00,0x04,0x00,0x00,0x04,0x00,0x00,0xE4,0x00,
//        0x00,0x9C,0x03,0x00,0x04,0x03,0x00,0x00,0x06,0x00,0x00,0x06,0x00,0x00,0x06,0x00,
//        0x04,0x06,0x00,0x0E,0x06,0x00,0x06,0x02,0x00,0x04,0x03,0x00,0xF8,0x01,0x00,0x00,
//        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
//    };
//    memcpy(data,c5,font->font_attr.bytes_per_word);

    uint32_t offset;

    if(!font || !data || c<FONT_ASCII_MIN)
    {
        return -1;
    }

    offset=c-FONT_ASCII_MIN;

    if(jacefs_read(font->file_id,font->app_id,data,
            font->font_attr.bytes_per_word,
            font->offset+FONT_DATA_HDR_SIZE+offset*font->font_attr.bytes_per_word )
        !=font->font_attr.bytes_per_word)
    {
        return -1;
    }

    return 0;
}

//获取文字个数 -- text中是gb2312和英文混合
uint16_t font_gb2312_text_len(const char* text)
{
    uint16_t words=0;
    for(int i=0;i<strlen(text);)
    {
        words++;
        if((uint8_t)text[i] < FONT_ASCII_LIMIT)
        {
            i++;
        }
        else
        {
            i+=2;
        }
    }

    return words;
}

//把中英混合的编码全部转为gb2312
//-- text中是gb2312和英文混合，gb2312为输出
uint16_t font_gb2312_text_fill(const char* text,uint16_t *gb2312)
{
    uint16_t words=0;
    for(int i=0;i<strlen(text);)
    {
        if((uint8_t)text[i]<FONT_ASCII_LIMIT)
        {
            gb2312[words]=text[i];
            i++;
        }
        else
        {
            gb2312[words]=((uint8_t)text[i]<<8)|((uint8_t)text[i+1]);
            i+=2;
        }
        words++;
    }

    return words;
}




