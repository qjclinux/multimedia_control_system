/*
 * tp_xpt2046.c
 *
 *  Created on: 2019年12月23日
 *      Author: jace
 */

#include <string.h>
#include <stdbool.h>
#include "fsl_debug_console.h"
#include "fsl_device_registers.h"
#include "fsl_spi.h"
#include "fsl_spi_dma.h"
#include "fsl_dma.h"
#include "board.h"
#include "pin_mux.h"
#include "tp_xpt2046.h"
#include "hs_spi.h"
#include "console.h"


#define TP_CS_PIN (20)
#define TP_CS_PORT (1)

#define TP_MISO_PIN (19)
#define TP_MISO_PORT (0)

#define TP_MOSI_PIN (20)
#define TP_MOSI_PORT (0)

#define TP_CLK_PIN (21)
#define TP_CLK_PORT (0)

#define TP_INT_PIN (18)
#define TP_INT_PORT (1)

#define tp_cs_low GPIO_PinWrite(GPIO, TP_CS_PORT, TP_CS_PIN, 0)
#define tp_cs_high GPIO_PinWrite(GPIO, TP_CS_PORT, TP_CS_PIN, 1)

#define tp_clk_low GPIO_PinWrite(GPIO, TP_CLK_PORT, TP_CLK_PIN, 0)
#define tp_clk_high GPIO_PinWrite(GPIO, TP_CLK_PORT, TP_CLK_PIN, 1)

#define tp_mosi_low GPIO_PinWrite(GPIO, TP_MOSI_PORT, TP_MOSI_PIN, 0)
#define tp_mosi_high GPIO_PinWrite(GPIO, TP_MOSI_PORT, TP_MOSI_PIN, 1)

#define tp_miso_read GPIO_PinRead(GPIO, TP_MISO_PORT, TP_MISO_PIN)
#define tp_int_read GPIO_PinRead(GPIO, TP_INT_PORT, TP_INT_PIN)

static void tp_io_init()
{
    const uint32_t tp_io = (IOCON_PIO_FUNC0 |IOCON_PIO_MODE_PULLUP |IOCON_PIO_SLEW_STANDARD |
                                IOCON_PIO_INV_DI |IOCON_PIO_DIGITAL_EN |IOCON_PIO_OPENDRAIN_DI);

    IOCON_PinMuxSet(IOCON, TP_CS_PORT, TP_CS_PIN, tp_io);
    IOCON_PinMuxSet(IOCON, TP_MOSI_PORT, TP_MOSI_PIN, tp_io);
    IOCON_PinMuxSet(IOCON, TP_MISO_PORT, TP_MISO_PIN, tp_io);
    IOCON_PinMuxSet(IOCON, TP_CLK_PORT, TP_CLK_PIN, tp_io);
    IOCON_PinMuxSet(IOCON, TP_INT_PORT, TP_INT_PIN, tp_io);

    gpio_pin_config_t pin_config = {
        kGPIO_DigitalOutput,
        1,
    };
//    GPIO_PortInit(GPIO, 0);
//    GPIO_PortInit(GPIO, 1);

    GPIO_PinInit(GPIO, TP_CS_PORT, TP_CS_PIN, &pin_config);
    GPIO_PinInit(GPIO, TP_MOSI_PORT, TP_MOSI_PIN, &pin_config);
    GPIO_PinInit(GPIO, TP_CLK_PORT, TP_CLK_PIN, &pin_config);

    pin_config.pinDirection=kGPIO_DigitalInput;
    GPIO_PinInit(GPIO, TP_MISO_PORT, TP_MISO_PIN, &pin_config);
    GPIO_PinInit(GPIO, TP_INT_PORT, TP_INT_PIN, &pin_config);

    tp_cs_high;
    tp_clk_high;
    tp_mosi_high;
}

bool tp_int_get()
{
    if(!tp_int_read)
        return true;
    else
        return false;
}
void tp_init()
{
    tp_io_init();
}

#define  CMD_RDX  0xD0   //触摸IC读坐标积存器
#define  CMD_RDY  0x90


static void lcd_delay_us(uint32_t us)
{
    for(int i=0;i<us;i++)
    {
        for(int j=0;j<17;j++)
        {

        }
    }
}

void TP_Write_Byte(uint8_t num)
{
    uint8_t count=0;
    for(count=0;count<8;count++)
    {
        if(num&0x80)
            tp_mosi_high;//TDIN=1;
        else
            tp_mosi_low;//TDIN=0;
        num<<=1;
        tp_clk_low;//TCLK=0;
        tp_clk_high;//TCLK=1;     //上升沿有效
    }
}

uint16_t TP_Read_AD(uint8_t CMD)
{
    uint8_t count=0;
    uint16_t Num=0;
    tp_clk_low;//TCLK=0;     //先拉低时钟
    tp_mosi_low;//TDIN=0;     //拉低数据线
    tp_cs_low;//TCS=0;      //选中触摸屏IC
    TP_Write_Byte(CMD);//发送命令字
    lcd_delay_us(6);//delay_us(6);//ADS7846的转换时间最长为6us
    tp_clk_low;//TCLK=0;
    lcd_delay_us(1);//delay_us(1);
    tp_clk_high;//TCLK=1;     //给1个时钟，清除BUSY
    tp_clk_low;//TCLK=0;
    for(count=0;count<16;count++)//读出16位数据,只有高12位有效
    {
        Num<<=1;
        tp_clk_low;//TCLK=0; //下降沿有效
        tp_clk_high;//TCLK=1;
        if(tp_miso_read/*DOUT*/)Num++;
    }
    Num>>=4;    //只有高12位有效.
    tp_cs_high;//TCS=1;      //释放片选
    return(Num);
}

#define READ_TIMES 5    //读取次数
#define LOST_VAL 1      //丢弃值
uint16_t TP_Read_XOY(uint8_t xy)
{
    uint16_t i, j;
    uint16_t buf[READ_TIMES];
    uint16_t sum=0;
    uint16_t temp;
    for(i=0;i<READ_TIMES;i++)buf[i]=TP_Read_AD(xy);
    for(i=0;i<READ_TIMES-1; i++)//排序
    {
        for(j=i+1;j<READ_TIMES;j++)
        {
            if(buf[i]>buf[j])//升序排列
            {
                temp=buf[i];
                buf[i]=buf[j];
                buf[j]=temp;
            }
        }
    }
    sum=0;
    for(i=LOST_VAL;i<READ_TIMES-LOST_VAL;i++)sum+=buf[i];
    temp=sum/(READ_TIMES-2*LOST_VAL);
    return temp;
}

uint8_t TP_Read_XY(uint16_t *x,uint16_t *y)
{
    uint16_t xtemp,ytemp;
    xtemp=TP_Read_XOY(CMD_RDX);
    ytemp=TP_Read_XOY(CMD_RDY);
    //if(xtemp<100||ytemp<100)return 0;//读数失败
    *x=xtemp;
    *y=ytemp;
    return 1;//读数成功
}

#define ERR_RANGE 50 //误差范围
uint8_t tp_read_xy(uint16_t *x,uint16_t *y)
{
    uint16_t x1,y1;
    uint16_t x2,y2;
    uint8_t flag;
    flag=TP_Read_XY(&x1,&y1);
    if(flag==0)return(0);
    flag=TP_Read_XY(&x2,&y2);
    if(flag==0)return(0);
    if(((x2<=x1&&x1<x2+ERR_RANGE)||(x1<=x2&&x2<x1+ERR_RANGE))//前后两次采样在+-50内
    &&((y2<=y1&&y1<y2+ERR_RANGE)||(y1<=y2&&y2<y1+ERR_RANGE)))
    {
        *x=(x1+x2)/2;
        *y=(y1+y2)/2;
        return 1;
    }else return 0;
}










