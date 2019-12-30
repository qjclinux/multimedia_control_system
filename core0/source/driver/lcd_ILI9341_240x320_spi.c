/*
 * lcd_hw_init.c
 *
 *  Created on: 2019年6月25日
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
#include "lcd_hw_drv.h"
#include "hs_spi.h"

#define LCD_RST_PIN (21)
#define LCD_RST_PORT (1)

#define LCD_DC_PIN 24//5//(20)
#define LCD_DC_PORT (1)

#define LCD_CS_PIN (1)
#define LCD_CS_PORT (1)

#define LCD_PEN_CS_PIN (27)
#define LCD_PEN_CS_PORT (0)

#define lcd_cs_low GPIO_PinWrite(GPIO, LCD_CS_PORT, LCD_CS_PIN, 0)
#define lcd_cs_high GPIO_PinWrite(GPIO, LCD_CS_PORT, LCD_CS_PIN, 1)

#define lcd_dc_low GPIO_PinWrite(GPIO, LCD_DC_PORT, LCD_DC_PIN, 0)
#define lcd_dc_high GPIO_PinWrite(GPIO, LCD_DC_PORT, LCD_DC_PIN, 1)

static void lcd_io_init()
{
    /*
     * RST PIO1_21
     */
    const uint32_t lcd_rst = (IOCON_PIO_FUNC0 |IOCON_PIO_MODE_PULLUP |IOCON_PIO_SLEW_STANDARD |
                                IOCON_PIO_INV_DI |IOCON_PIO_DIGITAL_EN |IOCON_PIO_OPENDRAIN_DI);
    IOCON_PinMuxSet(IOCON, LCD_RST_PORT, LCD_RST_PIN, lcd_rst);

    /*
     * DC PIO1_20
     */
    const uint32_t lcd_dc = (IOCON_PIO_FUNC0 |IOCON_PIO_MODE_PULLUP |IOCON_PIO_SLEW_STANDARD |
                                IOCON_PIO_INV_DI |IOCON_PIO_DIGITAL_EN |IOCON_PIO_OPENDRAIN_DI);
    IOCON_PinMuxSet(IOCON, LCD_DC_PORT, LCD_DC_PIN, lcd_dc);

    /*
     * PEN_CS PIO0_27
     */
    const uint32_t lcd_cs = (IOCON_PIO_FUNC0 |IOCON_PIO_MODE_PULLUP |IOCON_PIO_SLEW_STANDARD |
                                IOCON_PIO_INV_DI |IOCON_PIO_DIGITAL_EN |IOCON_PIO_OPENDRAIN_DI);
    IOCON_PinMuxSet(IOCON, LCD_CS_PORT, LCD_CS_PIN, lcd_cs);

    /*
     * PEN_CS PIO1_11
     */
    const uint32_t pen_cs = (IOCON_PIO_FUNC0 |IOCON_PIO_MODE_PULLUP |IOCON_PIO_SLEW_STANDARD |
                                IOCON_PIO_INV_DI |IOCON_PIO_DIGITAL_EN |IOCON_PIO_OPENDRAIN_DI);
    IOCON_PinMuxSet(IOCON, LCD_PEN_CS_PORT, LCD_PEN_CS_PIN, pen_cs);

    gpio_pin_config_t pin_config = {
        kGPIO_DigitalOutput,
        1,
    };
    GPIO_PortInit(GPIO, 0);
    GPIO_PortInit(GPIO, 1);

    GPIO_PinInit(GPIO, LCD_RST_PORT, LCD_RST_PIN, &pin_config);
    GPIO_PinInit(GPIO, LCD_DC_PORT, LCD_DC_PIN, &pin_config);
    GPIO_PinInit(GPIO, LCD_CS_PORT, LCD_CS_PIN, &pin_config);
    GPIO_PinInit(GPIO, LCD_PEN_CS_PORT, LCD_PEN_CS_PIN, &pin_config);

    GPIO_PinWrite(GPIO, LCD_RST_PORT, LCD_RST_PIN, 1);
    GPIO_PinWrite(GPIO, LCD_DC_PORT, LCD_DC_PIN, 1);
    GPIO_PinWrite(GPIO, LCD_CS_PORT, LCD_CS_PIN, 1);
    GPIO_PinWrite(GPIO, LCD_PEN_CS_PORT, LCD_PEN_CS_PIN, 1);
}

void lcd_write_cmd(uint8_t cmd)
{
    lcd_dc_low;
    spi_req_lock();
    lcd_cs_low;
    spi_write_data(&cmd,1);
    lcd_cs_high;
    spi_req_unlock();
    lcd_dc_high;
}

void lcd_write_data(uint8_t dat)
{
    lcd_dc_high;
    spi_req_lock();
    lcd_cs_low;
    spi_write_data(&dat,1);
    lcd_cs_high;
    spi_req_unlock();
}

/*
 * 这是 150M 主频，用逻辑分析仪测量出来的延时
 */
static void lcd_delay(uint32_t ms)
{
    for(int i=0;i<ms;i++)
    {
        for(int j=0;j<16666;j++)
        {

        }
    }
}


static void lcd_param_init()
{
//    while(1)
    {
        GPIO_PinWrite(GPIO, LCD_RST_PORT, LCD_RST_PIN, 1);
        lcd_delay(1);
        GPIO_PinWrite(GPIO, LCD_RST_PORT, LCD_RST_PIN, 0);
        lcd_delay(1);
    }
    GPIO_PinWrite(GPIO, LCD_RST_PORT, LCD_RST_PIN, 1);
    lcd_delay(120);

#if 1

    // VCI=2.8V

    //************* Start Initial Sequence **********//
    lcd_write_cmd(0xCF);
    lcd_write_data(0x00);
    lcd_write_data(0xC1);
    lcd_write_data(0X30);
    lcd_write_cmd(0xED);
    lcd_write_data(0x64);
    lcd_write_data(0x03);
    lcd_write_data(0X12);
    lcd_write_data(0X81);
    lcd_write_cmd(0xE8);
    lcd_write_data(0x85);
    lcd_write_data(0x00);
    lcd_write_data(0x79);
    lcd_write_cmd(0xCB);
    lcd_write_data(0x39);
    lcd_write_data(0x2C);
    lcd_write_data(0x00);
    lcd_write_data(0x34);
    lcd_write_data(0x02);
    lcd_write_cmd(0xF7);
    lcd_write_data(0x20);
    lcd_write_cmd(0xEA);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_cmd(0xC0); //Power control
    lcd_write_data(0x1D); //VRH[5:0]
    lcd_write_cmd(0xC1); //Power control
    lcd_write_data(0x12); //SAP[2:0];BT[3:0]
    lcd_write_cmd(0xC5); //VCM control
    lcd_write_data(0x33);
    lcd_write_data(0x3F);
    lcd_write_cmd(0xC7); //VCM control
    lcd_write_data(0x92);
    lcd_write_cmd(0x3A); // Memory Access Control
    lcd_write_data(0x55);
    lcd_write_cmd(0x36); // Memory Access Control
    lcd_write_data(0x08);
    lcd_write_cmd(0xB1);
    lcd_write_data(0x00);
    lcd_write_data(0x12);
    lcd_write_cmd(0xB6); // Display Function Control
    lcd_write_data(0x0A);
    lcd_write_data(0xA2);

    lcd_write_cmd(0x44);
    lcd_write_data(0x02);

    lcd_write_cmd(0xF2); // 3Gamma Function Disable
    lcd_write_data(0x00);
    lcd_write_cmd(0x26); //Gamma curve selected
    lcd_write_data(0x01);
    lcd_write_cmd(0xE0); //Set Gamma
    lcd_write_data(0x0F);
    lcd_write_data(0x22);
    lcd_write_data(0x1C);
    lcd_write_data(0x1B);
    lcd_write_data(0x08);
    lcd_write_data(0x0F);
    lcd_write_data(0x48);
    lcd_write_data(0xB8);
    lcd_write_data(0x34);
    lcd_write_data(0x05);
    lcd_write_data(0x0C);
    lcd_write_data(0x09);
    lcd_write_data(0x0F);
    lcd_write_data(0x07);
    lcd_write_data(0x00);
    lcd_write_cmd(0XE1); //Set Gamma
    lcd_write_data(0x00);
    lcd_write_data(0x23);
    lcd_write_data(0x24);
    lcd_write_data(0x07);
    lcd_write_data(0x10);
    lcd_write_data(0x07);
    lcd_write_data(0x38);
    lcd_write_data(0x47);
    lcd_write_data(0x4B);
    lcd_write_data(0x0A);
    lcd_write_data(0x13);
    lcd_write_data(0x06);
    lcd_write_data(0x30);
    lcd_write_data(0x38);
    lcd_write_data(0x0F);
    lcd_write_cmd(0x11); //Exit Sleep
    lcd_delay(120);
    lcd_write_cmd(0x29); //Display on

#endif

}


static uint16_t *lcd_fb=NULL;
static uint16_t lcd_rex=LCD_RES_X,lcd_rey=LCD_RES_Y;

void lcd_set_window(uint16_t start_x,uint16_t end_x,uint16_t start_y,uint16_t end_y)
{
#if 1
    uint8_t data[4];

    lcd_write_cmd(0x2A);
    data[0]=start_x>>8;
    data[1]=start_x&0xff;
    data[2]=end_x>>8;
    data[3]=end_x&0xff;
    lcd_write_data(data[0]);
    lcd_write_data(data[1]);
    lcd_write_data(data[2]);
    lcd_write_data(data[3]);

    lcd_write_cmd(0x2B);
    data[0]=start_y>>8;
    data[1]=start_y&0xff;
    data[2]=end_y>>8;
    data[3]=end_y&0xff;
    lcd_write_data(data[0]);
    lcd_write_data(data[1]);
    lcd_write_data(data[2]);
    lcd_write_data(data[3]);

    lcd_write_cmd(0x2C);
#endif
}

void lcd_set_fb(uint32_t fb_addr,uint16_t resx,uint16_t resy)
{
    lcd_fb=(uint16_t *)fb_addr;
    lcd_rex=resx;
    lcd_rey=resy;

}

void lcd_flush(bool wait_to_finish)
{
    lcd_dc_high;
    spi_req_lock();
    lcd_cs_low;
    for(int i=0;i<lcd_rey/2;i++)
    {
        spi_write_data(((uint8_t*)lcd_fb)+lcd_rex*2*2*i,lcd_rex*2*2);
    }
    lcd_cs_high;
    spi_req_unlock();
}

void lcd_hw_init(uint16_t resx,uint16_t resy)
{

    lcd_io_init();

    lcd_param_init();

}


void lcd_off()
{
//    lcd_write_cmd(0x10);
//    lcd_write_cmd(0x28);
}

void lcd_on()
{
//    lcd_write_cmd(0x11);
//    lcd_write_cmd(0x29);
}

