/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "board.h"
#include "mcmgr.h"

#include "fsl_common.h"
#include "pin_mux.h"
#include "fsl_gpio.h"
#include "lcd_hw_drv.h"
#include "pin_mux.h"
#include "hs_spi.h"


/*******************************************************************************
 * Definitions
 ******************************************************************************/
//#define LED_INIT() GPIO_PinInit(GPIO, BOARD_LED_RED_GPIO_PORT, BOARD_LED_RED_GPIO_PIN, &led_config);
//#define LED_TOGGLE() GPIO_PortToggle(GPIO, BOARD_LED_RED_GPIO_PORT, 1u << BOARD_LED_RED_GPIO_PIN);

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Function to create delay for Led blink.
 */
void delay(void)
{
    volatile uint32_t i = 0;
    for (i = 0; i < 1000000; ++i)
    {
        __asm("NOP"); /* delay */
    }
}

/*!
 * @brief Application-specific implementation of the SystemInitHook() weak function.
 */
void SystemInitHook(void)
{
    /* Initialize MCMGR - low level multicore management library. Call this
       function as close to the reset entry as possible to allow CoreUp event
       triggering. The SystemInitHook() weak function overloading is used in this
       application. */
    MCMGR_EarlyInit();
}

static void RPMsgRemoteReadyEventHandler(uint16_t eventData, void *context)
{
    uint16_t *data = (uint16_t *)context;

    *data = eventData;
}

/*!
 * @brief Main function
 */
int main(void)
{
    static uint16_t *gui_fb;
    uint32_t startupData
//    ,i
    ;
    mcmgr_status_t status;

    /* Define the init structure for the output LED pin*/
//    gpio_pin_config_t led_config = {
//        kGPIO_DigitalOutput,
//        0,
//    };

    BOARD_BootClockPLL150M();

    /* Initialize MCMGR, install generic event handlers */
    MCMGR_Init();

    volatile uint16_t RPMsgRemoteReadyEventData = 0;

    MCMGR_RegisterEvent(kMCMGR_RemoteApplicationEvent, RPMsgRemoteReadyEventHandler,
                            (void *)&RPMsgRemoteReadyEventData);

    /* Get the startup data */
    do
    {
        status = MCMGR_GetStartupData(&startupData);
    } while (status != kStatus_MCMGR_Success);

    /* Make a noticable delay after the reset */



    /* Init board hardware.*/
    /* enable clock for GPIO */
    CLOCK_EnableClock(kCLOCK_Gpio0);
    CLOCK_EnableClock(kCLOCK_Gpio1);

    /* Configure LED */
//    LED_INIT();


    CLOCK_AttachClk(kMAIN_CLK_to_HSLSPI);/* attach 50 MHz clock to HSLSPI */
    RESET_PeripheralReset(kHSLSPI_RST_SHIFT_RSTn);/* reset FLEXCOMM for SPI */
    hs_spi_init();

    gui_fb=(uint16_t*)(startupData);
    lcd_hw_init(LCD_RES_X,LCD_RES_Y);
    lcd_set_fb((uint32_t)gui_fb,LCD_RES_X,LCD_RES_Y);

    lcd_set_window(0, LCD_RES_X-1, 0, LCD_RES_Y-1);
    lcd_flush(true);

    while (1)
    {
//        delay();
//        LED_TOGGLE();

        /*
         * 等待Core0的通知，然后处理事件
         */
#define RPMSG_FLUSH_SCREEN_REQ  1
#define RPMSG_FLUSH_SCREEN_DONE 2

        if(RPMsgRemoteReadyEventData==RPMSG_FLUSH_SCREEN_REQ)
        {
            lcd_set_window(0, LCD_RES_X-1, 0, LCD_RES_Y-1);
            lcd_flush(true);

            MCMGR_TriggerEventForce(kMCMGR_RemoteApplicationEvent, RPMSG_FLUSH_SCREEN_DONE);
            RPMsgRemoteReadyEventData=0;
        }

    }
}
