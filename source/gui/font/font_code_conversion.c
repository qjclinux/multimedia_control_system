/*
 * font_code_conversion.c
 *
 *  Created on: 2019年11月5日
 *      Author: jace
 */

#include "stdint.h"
#include "string.h"
#include "osal.h"
#include "font_gb2312_unicode_map.h"
#include "font_code_conversion.h"

static int enc_get_utf8_size(const unsigned char pInput)
{
    unsigned char c = pInput;
    if(c< 0x80) return 0;
    if(c>=0x80 && c<0xC0) return -1;
    if(c>=0xC0 && c<0xE0) return 2;
    if(c>=0xE0 && c<0xF0) return 3;
    if(c>=0xF0 && c<0xF8) return 4;
    if(c>=0xF8 && c<0xFC) return 5;
    if(c>=0xFC) return 6;
    return 0;
}

static int enc_utf8_to_unicode_one(const unsigned char* pInput, unsigned long *Unic)
{
    if(pInput == 0 || Unic == 0)return 0;
    char b1, b2, b3;
    *Unic = 0x0;
    int utfbytes = enc_get_utf8_size(*pInput);
    unsigned char *pOutput = (unsigned char *) Unic;

    switch ( utfbytes )
    {
        case 0:
           *pOutput     = *pInput;
            utfbytes    += 1;
            break;
        case 2:
            b1 = *pInput;
            b2 = *(pInput + 1);
            if (((b2 & 0x80) != 0x80)||((b1&0xc0)!=0xc0))
               return 0;
            *pOutput     = (b1 << 6) + (b2 & 0x3F);
            *(pOutput+1) = (b1 >> 2) & 0x07;
            break;
        case 3:
            b1 = *pInput;
            b2 = *(pInput + 1);
            b3 = *(pInput + 2);
            if (((b1&0xc0)!=0xc0)|| ((b2 & 0x80) != 0x80) || ((b3 & 0x80) != 0x80) )
                return 0;
            *pOutput     = (b2 << 6) + (b3 & 0x3F);
            *(pOutput+1) = (b1 << 4) + ((b2 >> 2) & 0x0F);
            break;
        case 4:
        case 5:
        case 6:
            *(pOutput+1) = 0x20;
            break;
        default:
            return 0;
    }

    return utfbytes;
}


uint16_t *utf8_to_unicode(const uint8_t *utf8,uint16_t utf8_bytes,uint16_t *unicode_len)
{
    unsigned long temp_unicode=0;
    uint16_t utfbytes=0;
    uint16_t unicode_cnt=0;
    uint16_t *out_unicode;

    if( !utf8 || !utf8_bytes)
        return NULL;

    //由于 utf8 长度不限制，并且转换出来的unicode也是不确定，所以干脆分配与utf8相同长度的内容空间（最坏的情况是utf8全是3个字节的，此时分配unicode空间会浪费2/3）
    out_unicode=(uint16_t*)OS_MALLOC((utf8_bytes+1)*sizeof(uint16_t));

    if(out_unicode==NULL)
        return NULL;

    for(uint16_t index=0;index<utf8_bytes;)
    {
        utfbytes=enc_utf8_to_unicode_one(utf8+index,&temp_unicode);
        if(utfbytes>0)
        {
            out_unicode[unicode_cnt++]=temp_unicode;
            index+=utfbytes;
        }
        else
        {
            index++;
        }
    }
    out_unicode[unicode_cnt]=0;

    if(unicode_len)
        *unicode_len=unicode_cnt;

    return out_unicode;
}

/**-----------------------------------------------*/

uint16_t *uicode_to_gb2312(const uint16_t *unicode,uint16_t unicode_len,uint16_t *gb2312_len)
{
    if(!unicode || !unicode_len)
    {
        return NULL;
    }

    uint16_t gb2312_code_len=0;
    uint16_t *gb2312_code;
    gb2312_code=(uint16_t*)OS_MALLOC((unicode_len+1)*sizeof(uint16_t));

    if(!gb2312_code)
        return NULL;

    for(uint16_t unicode_idx=0;unicode_idx<unicode_len;unicode_idx++)
    {
        uint16_t tmp_code=unicode[unicode_idx];

        //部分unicode 可以转为 ascii
        for(uint8_t i=0;unicode_to_ascii_table[i][0]!=0;i++)
        {
            if(unicode_to_ascii_table[i][0]==tmp_code)
            {
                tmp_code=unicode_to_ascii_table[i][1];
            }
        }

        /*
         * unicode 分类参考：
         * https://www.cnblogs.com/chris-oil/p/8677309.html
         */

        //Basic Latin (ascii)
        if(tmp_code>0x00 && tmp_code<=0x007F)
        {
            //ascii 码保留原样
        }

        //Latin
//        else if(tmp_code>=0x0080 && tmp_code<=0x1ef9)
//        {
//
//        }

        //CJK Unified Ideographs
        else if(tmp_code>=0x4E00 && tmp_code<0xA000)
        {
            for(uint16_t i=0;gb2312_unicode_table[i][0]!=0;i++)
            {
                if(gb2312_unicode_table[i][1]==tmp_code)
                {
                    tmp_code=gb2312_unicode_table[i][0];
                }
            }
        }

        else
        {
            //其他字符，统一转换为 ‘?’
            tmp_code='?';
        }

        gb2312_code[gb2312_code_len++]=tmp_code;
    }

    if(gb2312_len)
        *gb2312_len=gb2312_code_len;

    return gb2312_code;
}






