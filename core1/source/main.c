/**
 ****************************************************************************************
 *
 * @file main.c
 *
 *   ����LPC55S69 ˫��MCUƽ̨�Ķ�ý�����ϵͳ��3.2�紥��������ϵͳ����jace GUI��jace FS ��
 *   ��Ч��Codeʹ��ϵͳ����Ч���ǳ�������
 *
 *   ��ϵͳ������������ϵƹ��ľʹ�ã��ɿ��Ƶƹ�Ч����
 *
 * JUST FOR :
 *      NXP MCU��ս��
 *
 *
 *      ���ߣ�jace
 *      email:734136032@qq.com
 *
 *      ������jace GUI��jace FS Ϊjace���˷�������ʱ����Դ��������Ȥ�����뱾��ȡ����ϵ��
 *      ��Ȼ���˹��̵�lib��Դ����ڲ���������ͬ������������ʹ�á�
 *
 *
 *  date:2019.12.31
 *
 *      ף����¿��֣�
 *
 *
 ****************************************************************************************
 */
#include "board.h"
#include "mcmgr.h"

#include "fsl_common.h"
#include "pin_mux.h"
#include "fsl_gpio.h"
#include "lcd_hw_drv.h"
#include "pin_mux.h"
#include "hs_spi.h"
#include "fsl_power.h"

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
    uint32_t startupData;
    mcmgr_status_t status;

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
        /*
         * �ȴ�Core0��֪ͨ��Ȼ�����¼�
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

        POWER_EnterSleep();
    }
}
